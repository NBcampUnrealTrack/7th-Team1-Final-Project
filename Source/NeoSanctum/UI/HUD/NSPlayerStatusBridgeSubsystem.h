// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NSPlayerStatusBridgeSubsystem.generated.h"

class ANSPlayerState;
class UAbilitySystemComponent;

struct FNSPlayerStatusQueryMessage;
struct FNSPlayerStatusViewData;
struct FOnAttributeChangeData;

/**
 * 추적중인 팀원 한명의 PlayerState, ASC 및 Attribute 델리게이트 정보
 */
struct FNSPlayerStatusBinding
{
	TWeakObjectPtr<ANSPlayerState> PlayerState;
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
	
	FDelegateHandle HealthChangedHandle;
	FDelegateHandle MaxHealthChangedHandle;
	FDelegateHandle ShieldChangedHandle;
	FDelegateHandle MaxShieldChangedHandle;
	//TODO: 경험치 Attribute 변경 델리게이트 핸들 추가
};

/**
 * 복제된 팀원 PlayerState/GAS값을 읽어 GMS메시지로 UI에 전달
 */
UCLASS()
class NEOSANCTUM_API UNSPlayerStatusBridgeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	//팀원 상태 QueryGMS 채널을 구독
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	//GMS 및 Attribute 델리게이트와 타이머 정의
	virtual void Deinitialize() override;
	
private:
	//UI에서 전송한 팀원 상태 조회 요청 처리
	void HandleQueryMessage(FGameplayTag Channel, const FNSPlayerStatusQueryMessage& Message);
	
	//최초 Query수신시 팀원 추적 타이머 시작
	void EnsureTrackingStarted();
	
	//GameState의 현재 팀원 목록과 추적 목록 동기화
	void RefreshTrackedPlayers();
	
	//새 팀원의 ASC Attribute변경 이벤트 구독
	void AddTrackedPlayer(ANSPlayerState* PlayerState);
	
	//퇴장한 팀원의 ASC이벤트 구족 해제 및 UI알림
	void RemoveTrackedPlayer(int32 PlayerId, bool bBroadcastRemoval);
	
	//모든 팀원의 ASC이벤트 구독 해제
	void UnbindAllPlayers();
	
	//팀원의 체력 또는 쉴드가 변경 되었을때 호출
	void HandleAttributeChanged(const FOnAttributeChangeData& ChangeData, int32 PlayerId);
	
	//PlayerState와 ASC값을 UI용 ViewData로 변환
	bool BuildViewData(const ANSPlayerState* PlayerState, FNSPlayerStatusViewData& OutData) const;
	
	//특정 팀원의 최신 상태를 GMS로 전달
	void BroadcastPlayerChanged(int32 PlayerId);
	
	// TODO: Experience/RequiredExperience Attribute 실제 값 조회
	//경험치 AttributeSet 구현 전후를 분리하기위한 조회 함수
	void ReadExperienceAttributes(
		const UAbilitySystemComponent* AbilitySystem,
		float& OutCurrentExperience,
		float& OutRequiredExperience) const;
	
private:
	//팀원 상태 조회 요청 리스너
	FGameplayMessageListenerHandle QueryListenerHandle;
	
	//팀원 접속 및 퇴장 확인용 저빈도 타이머
	FTimerHandle RosterRefreshTimerHandle;
	
	//PlayerId를 기준으로 추적중인 팀원과 델리게이트 관리
	TMap<int32, FNSPlayerStatusBinding> TrackedPlayers;
};
