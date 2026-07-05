// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "NSBTTask_FlyingExecuteAbility.generated.h"

struct FNSEnemyAttackRow;
struct FAbilityEndedData;
class UGameplayAbility;

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSBTTask_FlyingExecuteAbility : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UNSBTTask_FlyingExecuteAbility();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	// 발동한 공격 GA 종료 콜백 (ASC->OnAbilityEnded에 바인딩)
	void OnAttackAbilityEnded(const FAbilityEndedData& AbilityEndedData);

	// 컨트롤러 종류(보스/드론)에 맞춰 공격 Row 선택을 위임
	const FNSEnemyAttackRow* SelectAttackRow(AAIController* AIController) const;

	// 컨트롤러 종류에 맞춰 공격 사용 기록(쿨다운) 위임
	void NotifyAttackUsed(AAIController* AIController, const FNSEnemyAttackRow& AttackRow) const;

private:
	// 종료 콜백에서 태스크를 마무리하기 위한 OwnerComp 캐시
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	// 발동한 공격 GA 클래스 (종료 판별용)
	TSubclassOf<UGameplayAbility> CachedAttackAbilityClass;

	// 공격 진행 상태 블랙보드 키
	FName IsAttackingKey = TEXT("bIsAttacking");
};
