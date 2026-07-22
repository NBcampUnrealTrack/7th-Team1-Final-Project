// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSBossAbilityExecutorComponent.generated.h"

class ANSBossAIController;
class UAbilitySystemComponent;
class UGameplayAbility;
struct FAbilityEndedData;
struct FNSEnemyAttackRow;

UENUM(BlueprintType)
enum class ENSBossAbilityExecutionState : uint8
{
	// 실행 요청이 없고 대기 중인 상태
	Idle UMETA(DisplayName = "Idle"),

	// 공격 Ability가 활성화되어 실행 중인 상태
	Running UMETA(DisplayName = "Running"),

	// 공격 Ability는 끝났고 다음 패턴으로 넘어가기 전 RecoverTime을 기다리는 상태
	Recovering UMETA(DisplayName = "Recovering"),

	// 공격 Ability와 RecoverTime이 정상적으로 끝난 상태
	Succeeded UMETA(DisplayName = "Succeeded"),

	// 공격 선택, Ability 실행, 필수 데이터 검증 중 하나가 실패한 상태
	Failed UMETA(DisplayName = "Failed"),

	// StateTree 종료, 외부 중단, Ability 취소로 공격이 중단된 상태
	Cancelled UMETA(DisplayName = "Cancelled")
};

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.05
 * 
 * 클래스 개요 : Boss StateTree가 요청한 공격 Ability의 실행 생명주기를 관리하는 컴포넌트
 * ASC Ability 실행, OnAbilityEnded Delegate 등록/해제, RecoverTime, 취소, 공격 종료 정리를 담당
*/
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSBossAbilityExecutorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSBossAbilityExecutorComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// FixedAttackId 또는 자동 선택으로 Boss 공격 Ability 실행을 요청하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Ability")
	bool RequestAttack(FName FixedAttackId = NAME_None);

	// 현재 실행 중인 Boss 공격 Ability를 취소하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Ability")
	void CancelCurrentAttack();

	// 실행 상태와 캐시를 초기화하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Ability")
	void ResetExecutor();

	// 현재 Ability 실행 또는 RecoverTime 대기 중인지 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Ability")
	bool IsExecuting() const;
	
	// 현재 AbilityClass가 ASC에서 활성 상태인지 확인하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Ability")
	bool IsCurrentAbilityActive() const;

	// 현재 실행 상태를 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Ability")
	ENSBossAbilityExecutionState GetExecutionState() const { return ExecutionState; }

	// 마지막 실행이 성공으로 종료됐는지 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Ability")
	bool WasLastAttackSuccessful() const { return ExecutionState == ENSBossAbilityExecutionState::Succeeded; }

	// 마지막 실행이 실패 또는 취소로 종료됐는지 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Ability")
	bool WasLastAttackFailed() const;

	// 현재 또는 마지막으로 실행한 AttackId를 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Ability")
	FName GetCurrentAttackId() const { return CurrentAttackId; }

private:
	// Owner의 BossAIController를 반환하는 함수
	ANSBossAIController* GetBossController() const;

	// Owner의 ASC를 반환하는 함수
	UAbilitySystemComponent* GetOwnerASC() const;

	// FixedAttackId 또는 자동 선택으로 실행할 AttackRow를 반환하는 함수
	const FNSEnemyAttackRow* SelectAttackRow(ANSBossAIController* BossController, FName FixedAttackId) const;

	// 지정 AttackRow의 Ability 실행을 시작하는 함수
	bool ActivateAttackAbility(
		ANSBossAIController* BossController,
		UAbilitySystemComponent* ASC,
		const FNSEnemyAttackRow& AttackRow);

	// ASC Ability 종료 시 호출되는 함수
	void OnAttackAbilityEnded(const FAbilityEndedData& AbilityEndedData);

	// Ability 종료 후 RecoverTime 대기를 시작하는 함수
	void StartRecover(float InRecoverTime);

	// RecoverTime 종료 시 공격을 성공 상태로 마무리하는 함수
	void CompleteRecover();

	// 공격 실행 결과를 확정하고 Controller 공격 상태를 정리하는 함수
	void FinishAttack(ENSBossAbilityExecutionState FinishState);

	// ASC의 AbilityEnded Delegate에서 이 컴포넌트를 해제하는 함수
	void UnbindAbilityEndedDelegate();

	// 다음 공격 실행 전 런타임 캐시를 초기화하는 함수
	void ResetRuntimeForNewAttack();

	// 공격 종료 후 UObject 참조 캐시를 정리하는 함수
	void ClearRuntimeCache();

private:
	// 현재 공격 실행 상태
	UPROPERTY(Transient)
	ENSBossAbilityExecutionState ExecutionState = ENSBossAbilityExecutionState::Idle;

	// 현재 또는 마지막으로 실행한 AttackId
	UPROPERTY(Transient)
	FName CurrentAttackId = NAME_None;

	// 현재 실행을 요청한 FixedAttackId
	UPROPERTY(Transient)
	FName RequestedFixedAttackId = NAME_None;

	// 현재 공격 종료 후 대기할 RecoverTime
	UPROPERTY(Transient)
	float RecoverTime = 0.0f;

	// 현재 공격 상태 정리를 Controller에 알렸는지 여부
	UPROPERTY(Transient)
	bool bAttackFinishNotified = false;

	// 현재 실행 중인 Ability Class
	UPROPERTY(Transient)
	TSubclassOf<UGameplayAbility> CurrentAbilityClass;

	// 현재 실행에 사용 중인 BossAIController
	UPROPERTY(Transient)
	TWeakObjectPtr<ANSBossAIController> CachedBossController;

	// 현재 실행에 사용 중인 ASC
	UPROPERTY(Transient)
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	// RecoverTime 완료 타이머 핸들
	FTimerHandle RecoverTimerHandle;
};
