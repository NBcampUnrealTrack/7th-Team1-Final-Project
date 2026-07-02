// Copyright 2026 One Team. All rights reserved.


#include "NSSessionSubsystem.h"

#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Kismet/GameplayStatics.h" 
#include "NeoSanctum/Core/Interface/NSGameInstanceInterface.h"
#include "NeoSanctum/Data/Config/NSLevelCatalog.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "Online/OnlineSessionNames.h"

// AppID 오염 방지용 키(후에 자체 AppID 발급받으면 제거 가능)
static const FName NS_SESSION_KEY = FName(TEXT("NS_GAMEKEY"));
// 초대코드용 키
static const FName NS_INVITE_CODE_KEY = FName(TEXT("NS_INVITECODE"));

// 초대 코드 발급용 함수
static FString GenerateInviteCode()
{
	// 헷갈리는 0,O,1,I 제외 (후에 수정 가능)
	static const FString Charset = TEXT("ABCDEFGHJKLMNPQRSTUVWXYZ23456789");
	FString Code;
	for (int32 i = 0; i < 8; ++i)
	{
		Code.AppendChar(Charset[FMath::RandRange(0, Charset.Len() - 1)]);
	}
	
	return Code;
}

void UNSSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (OnlineSubsystem)
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();
	}
	
	if (SessionInterface.IsValid())
	{
		SessionInviteAcceptedDelegateHandle =
			SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(
				FOnSessionUserInviteAcceptedDelegate::CreateUObject(
					this, &UNSSessionSubsystem::OnSessionUserInviteAccepted));
	}
	
	// 콜드 런치로 이미 수락 상태로 켜졌는지 확인용
	TryConsumePendingInvite();
	
	// 조인/트래블 실패 시 복구용
	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UNSSessionSubsystem::HandleNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &UNSSessionSubsystem::HandleTravelFailure);
	}
}

void UNSSessionSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnTravelFailure().RemoveAll(this);
	}

	ClearCreateSessionDelegate();
	ClearDestroySessionDelegate();
	
	if (SessionInterface.IsValid())
	{
		if (FindSessionsDelegateHandle.IsValid())
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(
				FindSessionsDelegateHandle);
		if (JoinSessionDelegateHandle.IsValid())
			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(
				JoinSessionDelegateHandle);
	}
	
	if (SessionInterface.IsValid() && SessionInviteAcceptedDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(
			   SessionInviteAcceptedDelegateHandle);
	}
	
	FindSessionsDelegateHandle.Reset();
	JoinSessionDelegateHandle.Reset();
	LastSessionSearch.Reset();
	LastSessionSettings.Reset();
	SessionInterface.Reset();
	SessionInviteAcceptedDelegateHandle.Reset();
	PendingInviteResult.Reset();

	bIsJoining = false;
	bIsCreatingSession = false;
	bIsDestroyingSession = false;
	bHostStartQueued = false;
	bSwitchingToHost = false;
	bCreateSessionAfterDestroy = false;
	bReturnToTitleAfterDestroy = false;
	bHasPendingInvite = false;

	Super::Deinitialize();
}

void UNSSessionSubsystem::StartGameToHub()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FString HubPackage;
	if (UNSLevelCatalog* Catalog = GetLevelCatalog(); Catalog && !Catalog->HubLevel.IsNull())
	{
		HubPackage = Catalog->HubLevel.ToSoftObjectPath().GetLongPackageName();
	}
	if (HubPackage.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("허브 레벨을 찾지 못해 게임 시작 불가"));
		return;
	}

	if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this))
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			UIManager->ShowTravelLoadingScreen(PC);
		}
	}

	// 세션은 열지않고 리슨 서버로만 오픈
	UGameplayStatics::OpenLevel(
		World,
		FName(*HubPackage),
		true,
		TEXT("listen"));
}

void UNSSessionSubsystem::CreateSession()
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	// 이미 세션이 존재하면 재생성하지 않음
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		UE_LOG(LogTemp, Warning, TEXT("이미 세션이 존재하므로 재생성 무시"));
		// 기존 코드를 UI에 다시 알려줌 (세션 생성 버튼 눌러도 코드 변경 X)
		const FString ExistingCode = GetCurrentInviteCode();
		if (!ExistingCode.IsEmpty())
		{
			OnInviteCodeReady.Broadcast(ExistingCode);
		}
		
			return;
	}
	
	// 이미 Host 생성/삭제/예약 과정이 진행 중이면 연타 요청 무시
	if (bIsCreatingSession ||
		bIsDestroyingSession ||
		bHostStartQueued ||
		bSwitchingToHost)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Host 세션 작업이 이미 진행 중이므로 중복 요청 무시"));
		return;
	}

	// 같은 프레임의 추가 클릭 차단
	bSwitchingToHost = true;
	bHostStartQueued = true;
	bCreateSessionAfterDestroy = false;

	// Join 중이라면 타임아웃을 기다리지 않고 바로 취소
	if (bIsJoining || HasPendingNetGame())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("진행 중인 Join 연결을 취소하고 Host 전환 시작"));

		CancelPendingJoinForHost();
	}

	// 다음 틱에서 세션 생성을 시작
	QueueCreateSessionForNextTick();
}

void UNSSessionSubsystem::JoinSessionByCode(const FString& InviteCode)
{
	const FString TrimmedCode = InviteCode.TrimStartAndEnd();
	if (TrimmedCode.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("초대 코드가 비어 있음"));
		OnJoinSessionComplete.Broadcast(false);
		
		return;
	}

	if (!SessionInterface.IsValid())
	{
		OnJoinSessionComplete.Broadcast(false);
		
		return;
	}
	
	// 검색 중복 방지와 조인 진행 중 방지
	if (bIsSearchingForCode || bIsJoining)
	{
		UE_LOG(LogTemp, Warning, TEXT("이미 검색 or 조인 중"));
		return;
	}
	
	// Host 작업 중 거부
	if (bIsCreatingSession || bIsDestroyingSession || bHostStartQueued || bSwitchingToHost)
	{
		UE_LOG(LogTemp, Warning, TEXT("Host 세션 작업 중이므로 Join 요청 거부"));
		return;
	}
	
	const ULocalPlayer* LP = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr;
	if (!LP || !LP->GetPreferredUniqueNetId().IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("로컬 플레이어/NetId 없음"));
		OnJoinSessionComplete.Broadcast(false);
		
		return;
	}
	
	bIsSearchingForCode = true; 
	PendingJoinCode = TrimmedCode.ToUpper();

	LastSessionSearch = MakeShared<FOnlineSessionSearch>();
	LastSessionSearch->bIsLanQuery = false;
	LastSessionSearch->MaxSearchResults = 50;
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	FindSessionsDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(
			this,
			&UNSSessionSubsystem::OnFindSessionsForCodeCompleted));

	SessionInterface->FindSessions(
		*LP->GetPreferredUniqueNetId(),
		LastSessionSearch.ToSharedRef());
}

void UNSSessionSubsystem::OnFindSessionsForCodeCompleted(bool bWasSuccessful)
{
	if (FindSessionsDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
		FindSessionsDelegateHandle.Reset();
	}
	
	// 검색 끝났으니 초기화
	bIsSearchingForCode = false; 

	if (!bWasSuccessful || !LastSessionSearch.IsValid())
	{
		OnJoinSessionComplete.Broadcast(false);
		
		return;
	}

	for (const FOnlineSessionSearchResult& Result : LastSessionSearch->SearchResults)
	{
		FString FoundCode;
		if (Result.Session.SessionSettings.Get(NS_INVITE_CODE_KEY, FoundCode)
			&& FoundCode.ToUpper() == PendingJoinCode)
		{
			// JoinResolvedSession으로 진행
			JoinResolvedSession(Result);
			
			return;
		}
	}

	// 일치 코드 없음
	UE_LOG(LogTemp, Warning, TEXT("초대 코드에 해당하는 세션 없음: %s"), *PendingJoinCode);
	OnJoinSessionComplete.Broadcast(false);
}

void UNSSessionSubsystem::OnReadFriendsListCompleted(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName,
	const FString& ErrorStr)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("친구 목록 읽기 실패: %s"), *ErrorStr);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("친구 목록 읽기 성공"));
	// UI가 GetCachedFriends로 꺼내감
	OnFriendsListUpdated.Broadcast();
}

void UNSSessionSubsystem::DestroySession()
{
	if (bIsCreatingSession ||
			bIsDestroyingSession ||
			bHostStartQueued ||
			bSwitchingToHost)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("다른 세션 작업이 진행 중이므로 DestroySession 요청 거부"));

		OnDestroySessionComplete.Broadcast(false);
		return;
	}

	bCreateSessionAfterDestroy = false;
	StartDestroySession();
}

void UNSSessionSubsystem::LeaveSessionToTitle()
{
	// 진행 중 플래그 리셋 (다른 세션 작업 상태 정리)
	bIsJoining = false;
	bHostStartQueued = false;
	bSwitchingToHost = false;
	bCreateSessionAfterDestroy = false;

	// 펜딩 조인이 남아 있으면 취소
	if (HasPendingNetGame())
	{
		CancelPendingJoinForHost();
	}

	// 내가 GameSession을 들고 있으면 Destroy 완료 후 타이틀로
	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession))
	{
		bReturnToTitleAfterDestroy = true;
		StartDestroySession();
		
		return;
	}

	// 보유 세션 없으면 즉시 타이틀로 이동
	ReturnToTitle();
}

void UNSSessionSubsystem::RegisterPlayerInSession(const FUniqueNetIdRepl& PlayerId)
{
	if (!SessionInterface.IsValid() || !PlayerId.IsValid())
	{
		return;
	}
	
	if (!SessionInterface->GetNamedSession(NAME_GameSession))
	{
		return;
	}
	
	SessionInterface->RegisterPlayer(
		NAME_GameSession, 
		*PlayerId.GetUniqueNetId(),
		false);
}

void UNSSessionSubsystem::UnregisterPlayerInSession(const FUniqueNetIdRepl& PlayerId)
{
	if (!SessionInterface.IsValid() || !PlayerId.IsValid())
	{
		return;
	}
	
	if (!SessionInterface->GetNamedSession(NAME_GameSession))
	{
		return;
	}
	
	SessionInterface->UnregisterPlayer(
		NAME_GameSession,
		*PlayerId.GetUniqueNetId());
}

FString UNSSessionSubsystem::GetCurrentInviteCode() const
{
	// 실제 세션이 없으면 코드값 무효값
	if (!SessionInterface.IsValid() ||
		!SessionInterface->GetNamedSession(NAME_GameSession))
	{
		return FString();
	}
	
	return CurrentInviteCode;
}

void UNSSessionSubsystem::InviteFriendToSession(const FString& FriendNetIdString)
{
	if (!SessionInterface.IsValid() || FriendNetIdString.IsEmpty())
	{
		return;
	}

	// 세션이 이미 있으면 바로 초대
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		SendInviteToFriendInternal(FriendNetIdString);
		return;
	}

	// 세션이 없으면 보류 목록에 추가하고 세션 생성 우선시
	PendingInviteFriendNetIds.AddUnique(FriendNetIdString);
	if (!bIsCreatingSession && !bHostStartQueued)
	{
		CreateSession();
	}
}

void UNSSessionSubsystem::StartRunSession()
{
	// 클라는 세션 상태 제어 안 함
	UWorld* World = GetWorld();
	if (World && World->GetNetMode() == NM_Client)
	{
		return; 
	}
	
	if (!SessionInterface.IsValid())
	{
		return;
	}

	// 세션이 존재할 때만
	FNamedOnlineSession* Session = SessionInterface->GetNamedSession(NAME_GameSession);
	if (!Session)
	{
		UE_LOG(LogTemp, Log, TEXT("[Session] 세션 없음(솔로 플레이) → StartSession 스킵"));
		return;
	}

	// 이미 InProgress면 중복 호출 방지
	if (Session->SessionState == EOnlineSessionState::InProgress)
	{
		return;
	}

	const bool bStarted = SessionInterface->StartSession(NAME_GameSession);
	UE_LOG(LogTemp, Log, TEXT("[Session] StartSession 요청=%d (인런 진입, 참가 잠금)"), bStarted ? 1 : 0);
}

void UNSSessionSubsystem::EndRunSession()
{
	// 클라는 세션 상태 제어 안 함
	UWorld* World = GetWorld();
	if (World && World->GetNetMode() == NM_Client)
	{
		return; 
	}
	
	if (!SessionInterface.IsValid())
	{
		return;
	}

	FNamedOnlineSession* Session = SessionInterface->GetNamedSession(NAME_GameSession);
	if (!Session)
	{
		return;
	}

	// InProgress 상태일 때만 End (Pending 상태면 이미 열려있음)
	if (Session->SessionState != EOnlineSessionState::InProgress)
	{
		return;
	}

	const bool bEnded = SessionInterface->EndSession(NAME_GameSession);
	UE_LOG(LogTemp, Log, TEXT("[Session] EndSession 요청=%d (허브 복귀, 참가 허용)"), bEnded ? 1 : 0);
}

void UNSSessionSubsystem::OnCreateSessionCompleted(FName SessionName, bool bWasSuccessful)
{
	ClearCreateSessionDelegate();
	bIsCreatingSession = false;
	bSwitchingToHost = false;

	if (!bWasSuccessful)
	{
		OnCreateSessionComplete.Broadcast(false);
		// 세션 생성에 실패하면 보류 초대 목록 클리어
		PendingInviteFriendNetIds.Reset(); 
		
		return;
	}
	
	OnCreateSessionComplete.Broadcast(true);
	
	// 초대 코드 발급하면 UI로 전달
	const FString InviteCode = GetCurrentInviteCode();
	if (!InviteCode.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("초대 코드 발급: %s"), *InviteCode);
		OnInviteCodeReady.Broadcast(InviteCode);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("세션 ID를 얻지 못해 초대 코드 발급 실패"));
	}
	
	// 세션 생성 중 쌓인 보류 친구들 전부 초대
	if (PendingInviteFriendNetIds.Num() > 0)
	{
		TArray<FString> ToInvite = MoveTemp(PendingInviteFriendNetIds);
		PendingInviteFriendNetIds.Reset();
		for (const FString& FriendId : ToInvite)
			SendInviteToFriendInternal(FriendId);
	}
}

void UNSSessionSubsystem::OnDestroySessionCompleted(FName SessionName, bool bWasSuccessful)
{
	ClearDestroySessionDelegate();
	bIsDestroyingSession = false;
	
	// 저장되어있는 초대 코드 정리
	CurrentInviteCode.Empty();
	
	// 초대로 인한 Destroy였다면 여기서 조인
	if (bJoinInviteAfterDestroy && PendingInviteResult.IsValid())
	{
		bJoinInviteAfterDestroy = false;
		// Destroy 실패 시엔 조인하지 않고 보류 초대 정리
		if (!bWasSuccessful || !PendingInviteResult.IsValid())
		{
			bHasPendingInvite = false;
			PendingInviteResult.Reset();
			OnDestroySessionComplete.Broadcast(bWasSuccessful);
			// 조인 실패 신호 (UI용)
			OnJoinSessionComplete.Broadcast(false);
			return;
		}
		
		bHasPendingInvite = false;
		const FOnlineSessionSearchResult Result = *PendingInviteResult;
		PendingInviteResult.Reset();
		JoinResolvedSession(Result);
		
		return;
	}
	
	// Destroy 끝났으면 타이틀 화면으로
	if (bReturnToTitleAfterDestroy)
	{
		bReturnToTitleAfterDestroy = false;
		// 이탈 우선
		bCreateSessionAfterDestroy = false; 
		OnDestroySessionComplete.Broadcast(bWasSuccessful);
		ReturnToTitle();
		
		return;
	}

	const bool bShouldCreateAfterDestroy =
		bCreateSessionAfterDestroy;

	bCreateSessionAfterDestroy = false;

	OnDestroySessionComplete.Broadcast(bWasSuccessful);

	if (!bShouldCreateAfterDestroy)
	{
		return;
	}

	if (!bWasSuccessful)
	{
		bSwitchingToHost = false;
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	bHostStartQueued = true;
	QueueCreateSessionForNextTick();
}

void UNSSessionSubsystem::JoinResolvedSession(const FOnlineSessionSearchResult& SearchResult)
{
	if (!SessionInterface.IsValid() || bIsJoining)
	{
		OnJoinSessionComplete.Broadcast(false);
		
		return;
	}
	if (bIsCreatingSession||
		bIsDestroyingSession ||
		bHostStartQueued || 
		bSwitchingToHost)
	{
		UE_LOG(LogTemp, Warning, TEXT("Host 작업 중이므로 Join 거부"));
		
		return;
	}

	const ULocalPlayer* LocalPlayer =
		GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr;
	if (!LocalPlayer || !LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		OnJoinSessionComplete.Broadcast(false);
		
		return;
	}

	bIsJoining = true;
	JoinSessionDelegateHandle =
		SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
			FOnJoinSessionCompleteDelegate::CreateUObject(
				this, &UNSSessionSubsystem::OnJoinSessionCompleted));

	SessionInterface->JoinSession(
		*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SearchResult);

}

void UNSSessionSubsystem::OnJoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface.IsValid() && JoinSessionDelegateHandle.IsValid())
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
	JoinSessionDelegateHandle.Reset();
	bIsJoining = false;

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinSession 실패: %d"), (int32)Result);
		OnJoinSessionComplete.Broadcast(false);
		ReturnToTitle();
		
		return;
	}

	FString ConnectString;
	if (!SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString))
	{
		UE_LOG(LogTemp, Error, TEXT("연결 문자열 해석 실패"));
		OnJoinSessionComplete.Broadcast(false);
		ReturnToTitle();
		
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("ConnectString: %s"), *ConnectString);

	APlayerController* PC =
		GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (!PC)
	{
		OnJoinSessionComplete.Broadcast(false);
		ReturnToTitle();
		return;
	}

	if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this))
		UIManager->ShowTravelLoadingScreen(PC);

	OnJoinSessionComplete.Broadcast(true);
	PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
}

void UNSSessionSubsystem::OnSessionUserInviteAccepted(
	const bool bWasSuccessful, 
	const int32 ControllerId,
	FUniqueNetIdPtr UserId,
	const FOnlineSessionSearchResult& InviteResult)
{
	if (!bWasSuccessful || !InviteResult.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("초대 수락 실패 또는 결과 무효"));
		
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("초대 수락됨 → 조인 시도"));

	// 지금 바로 조인 가능한 상태가 아니면 보관 후 지연 처리
	if (bIsCreatingSession || bIsDestroyingSession || bHostStartQueued
		|| bSwitchingToHost || bIsJoining)
	{
		PendingInviteResult = MakeShared<FOnlineSessionSearchResult>(InviteResult);
		bHasPendingInvite = true;
		return;
	}

	JoinResolvedSession(InviteResult);
}

void UNSSessionSubsystem::SendInviteToFriendInternal(const FString& FriendNetIdString)
{
	const ULocalPlayer* LP = 
		GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr;
	if (!LP || !LP->GetPreferredUniqueNetId().IsValid())
	{
		return;
	}

	IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld());
	IOnlineIdentityPtr Identity = OSS ? OSS->GetIdentityInterface() : nullptr;
	if (!Identity.IsValid())
	{
		return;
	}

	FUniqueNetIdPtr FriendId = Identity->CreateUniquePlayerId(FriendNetIdString);
	if (!FriendId.IsValid())
	{
		return;
	}

	const bool bSent = SessionInterface->SendSessionInviteToFriend(
		*LP->GetPreferredUniqueNetId(), NAME_GameSession, *FriendId);
	UE_LOG(LogTemp, Log, TEXT("세션 초대 전송(%s): %s"),
		bSent ? TEXT("성공") : TEXT("실패"), *FriendNetIdString);
}

void UNSSessionSubsystem::HandleNetworkFailure(
	UWorld* World,
	UNetDriver* NetDriver,
	ENetworkFailure::Type FailureType,
	const FString& ErrorString)
{
	const bool bIsServer = World && World->GetNetMode() != NM_Client;

	if (bIsServer)
	{
		switch (FailureType)
		{
			// 특정 클라 연결의 실패하면 호스트는 게임 유지, 그 연결만 엔진이 정리
		case ENetworkFailure::ConnectionTimeout:
		case ENetworkFailure::ConnectionLost:
		case ENetworkFailure::FailureReceived:
			return;

		default:
			// 호스트의 자신의 에러는 진행
			break;
		}
	}
	
	const bool bIsPendingDriverFailure =
		NetDriver &&
		NetDriver->NetDriverName == FName(TEXT("PendingNetDriver"));

	if (bSwitchingToHost && bIsPendingDriverFailure)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Host 전환을 위해 Pending Join 취소 이벤트 무시: %s"),
			*ErrorString);

		bIsJoining = false;
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("네트워크 실패 - 타이틀로 복귀: %s"),
		*ErrorString);

	bIsJoining = false;
	OnJoinSessionComplete.Broadcast(false);
	
	// 연결 실패시 풀백 함수 사용
	ReturnToTitle();
}

void UNSSessionSubsystem::HandleTravelFailure(
	UWorld* World,
	ETravelFailure::Type FailureType,
	const FString& ErrorString)
{
	UE_LOG(
			LogTemp,
			Warning,
			TEXT("트래블 실패 - 타이틀로 복귀: %s"),
			*ErrorString);

	bIsJoining = false;
	bIsCreatingSession = false;
	bIsDestroyingSession = false;
	bHostStartQueued = false;
	bSwitchingToHost = false;
	bCreateSessionAfterDestroy = false;

	// 트래블 실패시 풀백 함수 사용
	ReturnToTitle();
}

UNSLevelCatalog* UNSSessionSubsystem::GetLevelCatalog() const
{
	if (INSGameInstanceInterface* GameInstanceInterface =
		Cast<INSGameInstanceInterface>(GetGameInstance()))
	{
		return GameInstanceInterface->GetLevelCatalog();
	}
	
	return nullptr;
}

void UNSSessionSubsystem::ReturnToTitle()
{
	UWorld* World = GetWorld();
	if (!World) 
	{
		return;
	}

	if (UNSLevelCatalog* Catalog = GetLevelCatalog())
	{
		if (!Catalog->TitleLevel.IsNull())
		{
			UGameplayStatics::OpenLevelBySoftObjectPtr(
				World,
				Catalog->TitleLevel);
			return;
		}
	}
	
	// 복구용 로직
	UE_LOG(LogTemp, Warning, TEXT("카탈로그 TitleLevel 없음, 폴백 진행"));
	UGameplayStatics::OpenLevel(World, FName(TEXT("L_Title")));
}

void UNSSessionSubsystem::TryConsumePendingInvite()
{
	if (!bHasPendingInvite || !PendingInviteResult.IsValid())
	{
		return;
	}
	
	if (!SessionInterface.IsValid())
	{
		return;
	}

	// 아직 다른 작업이 남아있는 상태라면
	if (bIsCreatingSession || bIsDestroyingSession || bHostStartQueued
		|| bSwitchingToHost || bIsJoining)
	{
		return;
	}

	// 내가 이미 세션을 갖고 있으면 먼저 정리 후 조인
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		bJoinInviteAfterDestroy = true;
		DestroySession();
		
		return;
	}

	// 세션이 없으면 바로 조인
	const FOnlineSessionSearchResult Result = *PendingInviteResult;
	bHasPendingInvite = false;
	PendingInviteResult.Reset();
	JoinResolvedSession(Result);
}

bool UNSSessionSubsystem::HasPendingNetGame() const
{
	UWorld* World = GetWorld();
	if (!GEngine || !World)
	{
		return false;
	}

	const FWorldContext* WorldContext =
		GEngine->GetWorldContextFromWorld(World);

	return WorldContext &&
		WorldContext->PendingNetGame != nullptr;
}

void UNSSessionSubsystem::CancelPendingJoinForHost()
{
	// Join 상태 해제
	bIsJoining = false;

	UWorld* World = GetWorld();
	if (!GEngine || !World)
	{
		return;
	}

	FWorldContext* WorldContext =
		GEngine->GetWorldContextFromWorld(World);

	if (!WorldContext || !WorldContext->PendingNetGame)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("PendingNetGame 취소"));

	GEngine->CancelPending(
		World,
		nullptr);
}

void UNSSessionSubsystem::RequestFriendsList()
{
	IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld());
	IOnlineFriendsPtr Friends = OSS ? OSS->GetFriendsInterface() : nullptr;
	if (!Friends.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("친구 인터페이스 없음"));
		return;
	}

	Friends->ReadFriendsList(
		0,
		EFriendsLists::ToString(EFriendsLists::Default),
		FOnReadFriendsListComplete::CreateUObject(
			this, &UNSSessionSubsystem::OnReadFriendsListCompleted));
}

void UNSSessionSubsystem::GetCachedFriends(TArray<FNSFriendInfo>& OutFriends) const
{
	OutFriends.Reset();

	IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld());
	IOnlineFriendsPtr Friends = OSS ? OSS->GetFriendsInterface() : nullptr;
	if (!Friends.IsValid())
	{
		return;
	}

	TArray<TSharedRef<FOnlineFriend>> FriendList;
	// ReadFriendsList로 캐시된 목록을 꺼냄 (같은 필터 이름 사용)
	if (!Friends->GetFriendsList(
		0,
		EFriendsLists::ToString(EFriendsLists::Default),
		FriendList))
	{
		return;
	}

	OutFriends.Reserve(FriendList.Num());
	for (const TSharedRef<FOnlineFriend>& Friend : FriendList)
	{
		FNSFriendInfo Info;
		Info.DisplayName = Friend->GetDisplayName();
		Info.NetIdString = Friend->GetUserId()->ToString();
		
		const FOnlineUserPresence& Presence = Friend->GetPresence();
		Info.bIsOnline = Presence.bIsOnline;

		OutFriends.Add(Info);
	}
	
	// 온라인 여부로 정렬
	OutFriends.Sort([](const FNSFriendInfo& A, const FNSFriendInfo& B)
	{
		if (A.bIsOnline != B.bIsOnline)
		{
			return A.bIsOnline; 
		}
		return A.DisplayName < B.DisplayName;
	});
}

void UNSSessionSubsystem::QueueCreateSessionForNextTick()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		bHostStartQueued = false;
		bSwitchingToHost = false;
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	World->GetTimerManager().SetTimerForNextTick(
		this,
		&UNSSessionSubsystem::StartCreateSession);
}

void UNSSessionSubsystem::StartCreateSession()
{
	// 예약된 Host 작업 실행됨
	bHostStartQueued = false;

	if (!bSwitchingToHost)
	{
		return;
	}

	if (bIsCreatingSession || bIsDestroyingSession)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("다른 세션 비동기 작업이 진행 중이므로 Host 생성 중단"));

		bSwitchingToHost = false;
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	// CancelPending 이후 다음 틱에도 PendingNetGame이 남아 있으면
	// ServerTravel을 실행하지 않고 실패 처리
	if (HasPendingNetGame())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("PendingNetGame이 아직 남아 있어 Host 세션 생성 중단"));

		bSwitchingToHost = false;
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	if (!SessionInterface.IsValid())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Online Session Interface가 유효하지 않음"));

		bSwitchingToHost = false;
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	// 기존 GameSession이 있다면 삭제 완료를 기다린 뒤 새 세션 만듦
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("기존 GameSession을 제거한 뒤 Host 세션 재생성"));

		bCreateSessionAfterDestroy = true;
		StartDestroySession();
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	const ULocalPlayer* LocalPlayer =
		GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;

	if (!LocalPlayer)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("LocalPlayer가 없어 Host 세션을 생성할 수 없음"));

		bSwitchingToHost = false;
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	const FUniqueNetIdRepl PreferredUniqueNetId =
		LocalPlayer->GetPreferredUniqueNetId();

	if (!PreferredUniqueNetId.IsValid())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("LocalPlayer의 UniqueNetId가 유효하지 않음"));

		bSwitchingToHost = false;
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	// 활성 온라인 서브시스템 판별(true면 steam 세션, false면 기존 null(LAN) 테스트 세션)
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	const bool bUsingSteam =
		OnlineSubsystem && OnlineSubsystem->GetSubsystemName() == STEAM_SUBSYSTEM;

	LastSessionSettings = MakeShared<FOnlineSessionSettings>();
	LastSessionSettings->NumPublicConnections = 4;
	LastSessionSettings->bAllowJoinInProgress = false;
	LastSessionSettings->bShouldAdvertise = true;

	if (bUsingSteam)
	{
		LastSessionSettings->bIsLANMatch = false;
		// 프레즌스 세션
		LastSessionSettings->bUsesPresence = true;    
		// 스팀 로비로 만듦
		LastSessionSettings->bUseLobbiesIfAvailable = true;  
		// 친구 목록의 게임 참여 허용
		LastSessionSettings->bAllowJoinViaPresence = true;   
		// 스팀 초대 허용
		LastSessionSettings->bAllowInvites = true;
		
		// 세션별 고유 코드 대조용
		CurrentInviteCode = GenerateInviteCode();
		LastSessionSettings->Set(
			NS_INVITE_CODE_KEY,
			CurrentInviteCode,
			EOnlineDataAdvertisementType::ViaOnlineService);
		// 같은 AppID 내에서 우리 게임만 걸러내기 위한 키
		LastSessionSettings->Set(
			NS_SESSION_KEY,
			FString(TEXT("NeoSanctum")),
			EOnlineDataAdvertisementType::ViaOnlineService);
	}
	else
	{
		// 기존 LAN 테스트 경로
		LastSessionSettings->bIsLANMatch = true;
		LastSessionSettings->bUsesPresence = false;
	}

	bIsCreatingSession = true;

	CreateSessionDelegateHandle =
		SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
			FOnCreateSessionCompleteDelegate::CreateUObject(
				this,
				&UNSSessionSubsystem::OnCreateSessionCompleted));

	const bool bCreateRequestStarted = SessionInterface->CreateSession(
		*PreferredUniqueNetId,
		NAME_GameSession,
		*LastSessionSettings);
	
	// 완료 델리게이트를 기다리지 않고 정리
	if (!bCreateRequestStarted)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("CreateSession 요청 시작 불가"));

		ClearCreateSessionDelegate();
		bIsCreatingSession = false;
		bSwitchingToHost = false;
		OnCreateSessionComplete.Broadcast(false);
	}

}

void UNSSessionSubsystem::StartDestroySession()
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Online Session Interface가 유효하지 않아서 세션 제거 불가능"));

		const bool bWasRecreatingHost = bCreateSessionAfterDestroy;
		bCreateSessionAfterDestroy = false;
		bSwitchingToHost = false;

		OnDestroySessionComplete.Broadcast(false);
		
		if (bReturnToTitleAfterDestroy)
		{
			bReturnToTitleAfterDestroy = false;
			ReturnToTitle();
			
			return;
		}

		if (bWasRecreatingHost)
		{
			OnCreateSessionComplete.Broadcast(false);
		}
		return;
	}

	// 제거할 세션이 없으면 Destroy는 성공, Host 재생성 과정이라면 바로 다음 틱에 Create로 넘어감
	if (!SessionInterface->GetNamedSession(NAME_GameSession))
	{
		const bool bShouldCreateAfterDestroy = bCreateSessionAfterDestroy;
		bCreateSessionAfterDestroy = false;

		OnDestroySessionComplete.Broadcast(true);
		
		if (bReturnToTitleAfterDestroy) 
		{
			bReturnToTitleAfterDestroy = false;
			ReturnToTitle();
			
			return;
		}

		if (bShouldCreateAfterDestroy)
		{
			bHostStartQueued = true;
			QueueCreateSessionForNextTick();
		}
		return;
	}

	if (bIsDestroyingSession)
	{
		UE_LOG(LogTemp, Warning, TEXT("DestroySession 요청 이미 진행 중"));
		return;
	}

	bIsDestroyingSession = true;

	DestroySessionDelegateHandle =
		SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(
				this,
				&UNSSessionSubsystem::OnDestroySessionCompleted));

	const bool bDestroyRequestStarted =
		SessionInterface->DestroySession(NAME_GameSession);

	if (!bDestroyRequestStarted)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("DestroySession 시작하지 못함"));

		const bool bWasRecreatingHost = bCreateSessionAfterDestroy;

		ClearDestroySessionDelegate();
		bIsDestroyingSession = false;
		bCreateSessionAfterDestroy = false;
		bSwitchingToHost = false;

		OnDestroySessionComplete.Broadcast(false);

		if (bWasRecreatingHost)
		{
			OnCreateSessionComplete.Broadcast(false);
		}
	}
}

void UNSSessionSubsystem::ClearCreateSessionDelegate()
{
	if (SessionInterface.IsValid() &&
		CreateSessionDelegateHandle.IsValid())
	{
		SessionInterface
			->ClearOnCreateSessionCompleteDelegate_Handle(
				CreateSessionDelegateHandle);
	}

	CreateSessionDelegateHandle.Reset();
}

void UNSSessionSubsystem::ClearDestroySessionDelegate()
{
	if (SessionInterface.IsValid() &&
		DestroySessionDelegateHandle.IsValid())
	{
		SessionInterface
			->ClearOnDestroySessionCompleteDelegate_Handle(
				DestroySessionDelegateHandle);
	}

	DestroySessionDelegateHandle.Reset();
}
