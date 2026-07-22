// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "NeoSanctum/Core/GameFlow/NSSessionType.h"
#include "Engine/EngineBaseTypes.h"
#include "steam/steam_api.h"
#include "NSSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNSOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNSOnJoinSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNSOnDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNSOnInviteCodeReady, const FString&, InviteCode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNSOnFriendsListUpdated);

class UWorld;

// 세션 생성, 참가, 종료 담당 클래스
// 타이틀 화면에서의 연결 로직 처리용
UCLASS()
class NEOSANCTUM_API UNSSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// 리슨 서버, 세션 생성 분리용 함수(리슨 서버로 여는 함수)
	UFUNCTION(BlueprintCallable, Category = "Session")
	void StartGameToHub();

	// 세션 생성
	UFUNCTION(BlueprintCallable, Category = "Session")
	void CreateSession();

	// 초대 주소로 직접 참가(참가하는 클라이언트용)
	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinSessionByCode(const FString& InviteCode);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();
	
	// 능동적 세션 정리 함수 (일시정지 메뉴 "메인메뉴" 버튼)
	UFUNCTION(BlueprintCallable, Category = "Session")
	void LeaveSessionToTitle();
	
	// 인런 진입 시 세션을 InProgress로 전환해 중간 참가를 차단
	void StartRunSession();
	
	// GameMode가 호출하는 세션 인원 등록,해제용 함수
	void RegisterPlayerInSession(const FUniqueNetIdRepl& PlayerId);
	void UnregisterPlayerInSession(const FUniqueNetIdRepl& PlayerId);
	
	// 현재 세션의 로비 ID를 초대 코드 문자열로 반환 (없으면 빈 문자열)
	UFUNCTION(BlueprintCallable, Category = "Session")
	FString GetCurrentInviteCode() const;
	
	// 친구를 세션에 초대 (세션 없으면 생성 후 초대)
	UFUNCTION(BlueprintCallable, Category = "Session|Friends")
	void InviteFriendToSession(const FString& FriendNetIdString);

	// 허브 복귀 시 세션을 다시 열어 참가를 허용
	void EndRunSession();

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FNSOnCreateSessionComplete OnCreateSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FNSOnJoinSessionComplete OnJoinSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FNSOnDestroySessionComplete OnDestroySessionComplete;
	
	UPROPERTY(BlueprintAssignable, Category = "Session")
	FNSOnInviteCodeReady OnInviteCodeReady;
	
	// 친구 목록관련 함수 (후에 시간이 남는다면 세션서브시스템에서 분리)
	// 스팀 친구 목록 비동기 읽기 요청
	UFUNCTION(BlueprintCallable, Category = "Session|Friends")
	void RequestFriendsList();

	// 캐시된 친구 목록을 UI 표시용 구조체로 반환
	void GetCachedFriends(TArray<FNSFriendInfo>& OutFriends) const;

	// 친구 목록 읽기 완료 시 브로드캐스트
	UPROPERTY(BlueprintAssignable, Category = "Session|Friends")
	FNSOnFriendsListUpdated OnFriendsListUpdated;

private:
	// CreateSession을 다음 틱에 시작하도록 예약
	void QueueCreateSessionForNextTick();
	// 실제 세션 생성 실행
	void StartCreateSession();
	// 실제 세션 제거 실행
	void StartDestroySession();

	// 진행 중인 Pending 네트워크 게임(조인 대기)이 있는지 확인
	bool HasPendingNetGame() const;
	// Pending Join을 취소하고 Host 생성으로 전환
	void CancelPendingJoinForHost();

	// CreateSession 엔진 델리게이트 핸들 해제
	void ClearCreateSessionDelegate();
	// DestroySession 엔진 델리게이트 핸들 해제
	void ClearDestroySessionDelegate();
	
	STEAM_CALLBACK(UNSSessionSubsystem, OnSteamAvatarLoaded, AvatarImageLoaded_t);

	// 세션 생성 완료 콜백 (성공 시 허브로 ServerTravel)
	void OnCreateSessionCompleted(FName SessionName, bool bWasSuccessful);
	// 세션 제거 완료 콜백
	void OnDestroySessionCompleted(FName SessionName, bool bWasSuccessful);
	
	// 모든 참가의 공통 진입점 
	void JoinResolvedSession(const FOnlineSessionSearchResult& SearchResult);
	
	// 세션 참가 완료 콜백 (연결 문자열 해석 후 ClientTravel)
	void OnJoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	
	void OnFindSessionsForCodeCompleted(bool bWasSuccessful);
	
	// ReadFriendsList 완료 콜백
	void OnReadFriendsListCompleted(
		int32 LocalUserNum, bool bWasSuccessful,
		const FString& ListName, const FString& ErrorStr);
	
	// 스팀 친구 초대 수락 콜백
	void OnSessionUserInviteAccepted(
	const bool bWasSuccessful,
	const int32 ControllerId,
	FUniqueNetIdPtr UserId,
	const FOnlineSessionSearchResult& InviteResult);
	
	// 실제 스팀 세션 초대 전송 (세션이 있는 상태에서 호출)
	void SendInviteToFriendInternal(const FString& FriendNetIdString);
	
#pragma region Data section
	
	UFUNCTION()
	void HandleHubOutGameDataReady();

	void TravelToHubAfterDataReady();
	void ClearPendingHubTravel();
	void AbortPendingHubTravel();

	bool bHubTravelPending = false;

	FString PendingHubPackageName;

	TWeakObjectPtr<UWorld> PendingHubSourceWorld;

#pragma endregion Data section

	IOnlineSessionPtr SessionInterface;

	FDelegateHandle CreateSessionDelegateHandle;
	FDelegateHandle DestroySessionDelegateHandle;
	
	FDelegateHandle FindSessionsDelegateHandle;
	FDelegateHandle JoinSessionDelegateHandle;
	
	FDelegateHandle SessionInviteAcceptedDelegateHandle;
	
	TSharedPtr<class FOnlineSessionSearch> LastSessionSearch;

	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	
	// 콜드 런치 대비: 초기화 전에 수락된 초대를 잠깐 보관
	TSharedPtr<FOnlineSessionSearchResult> PendingInviteResult;
	
	// 조인/트래블 실패 시 타이틀로 복귀시키는 핸들러
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	// 조인 시도 중복 방지
	bool bIsJoining = false;

	// CreateSession 비동기 요청 중복 방지
	bool bIsCreatingSession = false;

	// DestroySession 비동기 요청 중복 방지
	bool bIsDestroyingSession = false;

	// 다음 틱에 Host 생성을 시작하도록 예약했는지 여부
	bool bHostStartQueued = false;

	// Pending Join을 취소하고 Host로 전환하는 전체 과정인지 여부
	bool bSwitchingToHost = false;

	// 기존 세션 제거 완료 후 Host 세션을 다시 생성할지 여부
	bool bCreateSessionAfterDestroy = false;
	
	// Destroy 완료 후 타이틀로 복귀할지 여부
	bool bReturnToTitleAfterDestroy = false;
	
	// 초기화 전,처리 지연된 초대가 있는지 여부
	bool bHasPendingInvite = false;

	// Destroy 완료 후 보류된 초대로 조인할지 여부
	bool bJoinInviteAfterDestroy = false;
	
	bool bIsSearchingForCode = false;
	
	class UNSLevelCatalog* GetLevelCatalog() const;
	void ReturnToTitle();
	// 보류된 초대를 조인 가능한 시점에 처리
	void TryConsumePendingInvite();
	
	// 발급한 초대코드 저장용
	FString CurrentInviteCode;
	
	// 검색 시 대조할 코드 보관
	FString PendingJoinCode;

	// 참가할 세션 검색 결과에서 읽은 초대 코드. Join 성공 후 CurrentInviteCode로 확정한다.
	FString PendingResolvedInviteCode;
	
	// 세션 생성 완료 후 초대할 친구 목록 보관용(여러 사람에게 초대 연타 방지용)
	TArray<FString> PendingInviteFriendNetIds;
};
