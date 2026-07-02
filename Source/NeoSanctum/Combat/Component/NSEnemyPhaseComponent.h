// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NSEnemyPhaseComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
struct FAbilityEndedData;
struct FNSEnemyPhaseRow;
class UNSEnemyData;

/**
 * Enemy의 Phase 상태, PhaseTag, TransitionGA, PatternLock을 관리하는 공통 컴포넌트입니다.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSEnemyPhaseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSEnemyPhaseComponent();

	// 현재 체력 비율을 기준으로 Phase 변경 여부를 갱신하는 함수
	const FNSEnemyPhaseRow* UpdatePhase(float HealthRatio);

	// Phase 런타임 상태를 초기화하는 함수
	void ResetPhaseState();

	// 현재 Phase 전환 중 패턴이 잠겨 있는지 반환하는 함수
	bool IsPatternLocked() const { return bPatternLocked; }

	// 현재 PhaseId를 반환하는 함수
	FName GetCurrentPhaseId() const { return CurrentPhaseId; }

	// 현재 PhaseTag를 반환하는 함수
	FGameplayTag GetCurrentPhaseTag() const { return CurrentPhaseTag; }

	// 현재 Phase Row를 반환하는 함수
	const FNSEnemyPhaseRow* GetCurrentPhaseRow() const { return CurrentPhaseRow; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Owner의 EnemyData를 반환하는 함수
	const UNSEnemyData* GetEnemyData() const;

	// Owner의 ASC를 반환하는 함수
	UAbilitySystemComponent* GetOwnerASC() const;

	// 새 Phase에 진입할 때 PhaseTag와 TransitionGA를 적용하는 함수
	void EnterPhase(const FNSEnemyPhaseRow& NewPhaseRow, bool bPlayTransition);

	// Phase Transition GA 종료 시 PatternLock을 해제하는 함수
	void OnTransitionAbilityEnded(const FAbilityEndedData& AbilityEndedData);

private:
	// 현재 적용된 Phase Row
	const FNSEnemyPhaseRow* CurrentPhaseRow = nullptr;

	// 현재 적용된 PhaseId
	FName CurrentPhaseId = NAME_None;

	// 현재 적용된 PhaseTag
	FGameplayTag CurrentPhaseTag;

	// 최초 Phase 초기화가 완료됐는지 여부
	bool bPhaseInitialized = false;

	// Phase 전환 중 일반 패턴 선택을 막는지 여부
	bool bPatternLocked = false;

	// 현재 실행 중인 Phase Transition GA
	TSubclassOf<UGameplayAbility> CurrentTransitionGA;
};