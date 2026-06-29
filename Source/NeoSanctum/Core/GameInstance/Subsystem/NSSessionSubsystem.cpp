// Copyright 2026 One Team. All rights reserved.


#include "NSSessionSubsystem.h"

#include "NSDataSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h" 
#include "NeoSanctum/Core/Interface/NSGameInstanceInterface.h"
#include "NeoSanctum/Data/Config/NSLevelCatalog.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"


void UNSSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (OnlineSubsystem)
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();
	}
	
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

	LastSessionSettings.Reset();
	SessionInterface.Reset();

	bIsJoining = false;
	bIsCreatingSession = false;
	bIsDestroyingSession = false;
	bHostStartQueued = false;
	bSwitchingToHost = false;
	bCreateSessionAfterDestroy = false;
	bReturnToTitleAfterDestroy = false;

	Super::Deinitialize();
}

void UNSSessionSubsystem::CreateSession()
{
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

void UNSSessionSubsystem::JoinSessionByAddress(const FString& Address)
{
	const FString TrimmedAddress = Address.TrimStartAndEnd();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("JoinSessionByAddress 호출: %s"),
		*TrimmedAddress);

	if (TrimmedAddress.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("참가 주소가 비어 있음"));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	if (bIsJoining)
	{
		UE_LOG(LogTemp, Warning, TEXT("이미 Join 시도 중"));
		return;
	}

	// Host 생성/삭제 중 ClientTravel이 시작되는 것을 막음
	if (bIsCreatingSession ||
		bIsDestroyingSession ||
		bHostStartQueued ||
		bSwitchingToHost)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Host 세션 작업 중이므로 Join 요청 거부"));
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	APlayerController* PlayerController =
		GameInstance ? GameInstance->GetFirstLocalPlayerController() : nullptr;

	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerController 없음"));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	bIsJoining = true;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("ClientTravel 호출: %s"),
		*TrimmedAddress);

	if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this))
	{
		UIManager->ShowTravelLoadingScreen(PlayerController);
	}

	PlayerController->ClientTravel(
		TrimmedAddress,
		ETravelType::TRAVEL_Absolute);
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

void UNSSessionSubsystem::OnCreateSessionCompleted(FName SessionName, bool bWasSuccessful)
{
	ClearCreateSessionDelegate();
	bIsCreatingSession = false;

	if (!bWasSuccessful)
	{
		bSwitchingToHost = false;
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	if (HasPendingNetGame())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("세션 생성 완료 후 PendingNetGame 감지됨 ServerTravel 중단"));

		bSwitchingToHost = false;
		OnCreateSessionComplete.Broadcast(false);
		return;
	}
	
	StartHostTravelToHub();
}

void UNSSessionSubsystem::StartHostTravelToHub()
{
	UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem)
	{
		bSwitchingToHost = false;
		OnCreateSessionComplete.Broadcast(false);
		return;
	}
	
	if (!DataSubsystem->IsCommonReady())
	{
		// Hub 진입 전에 캐릭터 기본 데이터 같은 공통 데이터가 준비되도록 기다림.
		DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &ThisClass::HandleHostCommonDataReady);
		DataSubsystem->OnCommonDataReady.AddDynamic(this, &ThisClass::HandleHostCommonDataReady);
		
		DataSubsystem->LoadCommonData();
		return;
	}
	
	if (!DataSubsystem->IsOutGameReady())
	{
		// 거점 진입 직후 거점/파트/진행 데이터가 비지 않도록 OutGame 데이터 준비 후 Travel.
		DataSubsystem->OnOutGameDataReady.RemoveDynamic(this, &ThisClass::HandleHostOutGameDataReady);
		DataSubsystem->OnOutGameDataReady.AddDynamic(this, &ThisClass::HandleHostOutGameDataReady);
		
		DataSubsystem->LoadOutGameData();
		return;
	}
	
	TravelToHubAfterDataReady();
}

void UNSSessionSubsystem::TravelToHubAfterDataReady()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		bSwitchingToHost = false;
		OnCreateSessionComplete.Broadcast(false);
		return;
	}
	
	FString HubPackage;
	if (UNSLevelCatalog* Catalog = GetLevelCatalog(); Catalog && !Catalog->HubLevel.IsNull())
	{
		HubPackage = Catalog->HubLevel.ToSoftObjectPath().GetLongPackageName();
	}
	
	if (HubPackage.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("카탈로그에서 거점 레벨을 찾지 못해서 호스트 트래블 중단."));
		bSwitchingToHost = false;
		OnCreateSessionComplete.Broadcast(false);
		return;
	}
	
	if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this))
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (APlayerController* LocalPlayerController = GameInstance->GetFirstLocalPlayerController())
			{
				UIManager->ShowTravelLoadingScreen(LocalPlayerController);
			}
		}
	}
	
	const bool bTravelStarted = World->ServerTravel(HubPackage + TEXT("?listen"));
	bSwitchingToHost = false;
	OnCreateSessionComplete.Broadcast(bTravelStarted);
}

void UNSSessionSubsystem::HandleHostCommonDataReady()
{
	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &ThisClass::HandleHostCommonDataReady);
	}
	
	StartHostTravelToHub();
}

void UNSSessionSubsystem::HandleHostOutGameDataReady()
{
	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->OnOutGameDataReady.RemoveDynamic(this, &ThisClass::HandleHostOutGameDataReady);
	}
	
	StartHostTravelToHub();
}

void UNSSessionSubsystem::OnDestroySessionCompleted(FName SessionName, bool bWasSuccessful)
{
	ClearDestroySessionDelegate();
	bIsDestroyingSession = false;
	
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

void UNSSessionSubsystem::HandleNetworkFailure(
	UWorld* World,
	UNetDriver* NetDriver,
	ENetworkFailure::Type FailureType,
	const FString& ErrorString)
{
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

	LastSessionSettings = MakeShared<FOnlineSessionSettings>();
	LastSessionSettings->bIsLANMatch = true;
	LastSessionSettings->NumPublicConnections = 4;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bUsesPresence = false;

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
