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
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	
private:
	// AbortTask와 워치독 타임아웃이 공유하는 정리 로직 (RecoverTimer clear, 델리게이트 해제, 어빌리티 취소, BB/캐시 초기화)
	void CleanupAndCancelAbility(UBehaviorTreeComponent& OwnerComp);
	
	// 발동한 공격 GA 종료 콜백 (ASC->OnAbilityEnded에 바인딩)
	void OnAttackAbilityEnded(const FAbilityEndedData& AbilityEndedData);
	
	// RecoverTime 대기가 끝나면 태스크를 최종 완료 처리하는 함수
	void FinishAfterRecover();

	// 컨트롤러 종류(보스/드론)에 맞춰 공격 Row 선택을 위임
	const FNSEnemyAttackRow* SelectAttackRow(AAIController* AIController) const;

	// 컨트롤러 종류에 맞춰 공격 사용 기록(쿨다운) 위임
	void NotifyAttackUsed(AAIController* AIController, const FNSEnemyAttackRow& AttackRow) const;

private:
	// 종료 콜백에서 태스크를 마무리하기 위한 OwnerComp 캐시
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	// 발동한 공격 GA 클래스 (종료 판별용)
	TSubclassOf<UGameplayAbility> CachedAttackAbilityClass;

	// 이번 공격 종료 후 다음 패턴 전 대기할 RecoverTime(초)
	float CachedRecoverTime = 0.f;

	// RecoverTime 대기 타이머 핸들
	FTimerHandle RecoverTimerHandle;
	
	// 공격 진행 상태 블랙보드 키
	FName IsAttackingKey = TEXT("bIsAttacking");
	
	// 패턴 최대 실행 허용 시간(초). 이 시간 내 EndAbility가 안 오면 자동 취소+Failed 처리
	UPROPERTY(EditAnywhere, Category="FlyingExecuteAbility")
	float MaxExecutionTime = 15.f;

	float ElapsedTime = 0.f;
};
