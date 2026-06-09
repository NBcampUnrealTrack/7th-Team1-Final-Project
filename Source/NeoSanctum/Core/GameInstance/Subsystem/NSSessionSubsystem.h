// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Engine/EngineBaseTypes.h"
#include "NSSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNSOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNSOnJoinSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNSOnDestroySessionComplete, bool, bWasSuccessful);

// 세션 생성, 참가, 종료 담당 클래스
// 타이틀 화면에서의 연결 로직 처리용
UCLASS()
class NEOSANCTUM_API UNSSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 세션 생성(호스트용)
	UFUNCTION(BlueprintCallable, Category = "Session")
	void CreateSession();

	// 초대 주소로 직접 참가(참가하는 클라이언트용)
	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinSessionByAddress(const FString& Address);
	
	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FNSOnCreateSessionComplete OnCreateSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FNSOnJoinSessionComplete OnJoinSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FNSOnDestroySessionComplete OnDestroySessionComplete;

private:
	void QueueCreateSessionForNextTick();
	void StartCreateSession();
	void StartDestroySession();

	bool HasPendingNetGame() const;
	void CancelPendingJoinForHost();

	void ClearCreateSessionDelegate();
	void ClearDestroySessionDelegate();
	
	void OnCreateSessionCompleted(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionCompleted(FName SessionName, bool bWasSuccessful);

	IOnlineSessionPtr SessionInterface;

	FDelegateHandle CreateSessionDelegateHandle;
	FDelegateHandle DestroySessionDelegateHandle;

	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	
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
};
