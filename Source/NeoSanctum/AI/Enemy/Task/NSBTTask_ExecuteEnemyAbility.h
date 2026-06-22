// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "BehaviorTree/BTTaskNode.h"
#include "NSBTTask_ExecuteEnemyAbility.generated.h"

class UGameplayAbility;
class ANSEnemyAIController;

UCLASS()
class NEOSANCTUM_API UNSBTTask_ExecuteEnemyAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UNSBTTask_ExecuteEnemyAbility();

	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp, 
		uint8* NodeMemory) override;
	
	virtual EBTNodeResult::Type AbortTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
	
private:
	void OnAttackAbilityEnded(const FAbilityEndedData& AbilityEndedData);

	UPROPERTY(Transient)
	TObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
	
	UPROPERTY(Transient)
	TSubclassOf<UGameplayAbility> CachedAttackAbilityClass;
	
	// 현재 실행 중인 공격이 근접 공격 예약을 사용하는지 기록해, 공격 종료·실패·중단 시 예약 반환 여부
	bool bUsesMeleeReservation = false;
	
private:
	// 공격 중단 원인이 피격 경직이면 근접 예약을 유지해야 하는지 확인하는 함수
	bool ShouldPreserveMeleeReservation(const ANSEnemyAIController* AIController) const;
};
