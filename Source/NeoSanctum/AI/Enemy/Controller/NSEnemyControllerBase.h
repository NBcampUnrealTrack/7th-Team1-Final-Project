// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NSEnemyControllerBase.generated.h"

class UBlackboardComponent;
class UStateTreeAIComponent;
class UAbilitySystemComponent;
class UNSFlyingLocomotionComponent;
class UNSEnemyData;
class UNSEnemyPhaseComponent;
class INSEnemyAgent;
struct FNSEnemyAttackRow;

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.01
 * 
 * 클래스 개요 : 일반 몬스터, 보스, 드론 Controller가 공유하는 Enemy AI 기반 Controller입니다.
*/
UCLASS(Abstract)
class NEOSANCTUM_API ANSEnemyControllerBase : public AAIController
{
	GENERATED_BODY()

public:
	ANSEnemyControllerBase();

	virtual void Tick(float DeltaTime) override;

	// 공격 Ability가 실제로 시작됐을 때 Enemy Controller 공통 공격 상태를 기록하는 함수
	virtual void NotifyAttackStarted();

	// 공격 Ability 종료, 실패, 취소 이후 Enemy Controller 공통 공격 상태를 정리하는 함수
	virtual void NotifyAttackFinished();

protected:
	// EnemyData의 BrainType에 맞는 AI Brain을 시작하는 함수
	void StartEnemyBrain(const UNSEnemyData* EnemyData);

	// 현재 실행 중인 AI Brain을 정지하는 함수
	void StopEnemyBrain(const FString& Reason);

	// 현재 Possess 중인 Pawn을 Enemy 공통 Interface로 반환하는 함수
	const INSEnemyAgent* GetControlledEnemyAgent() const;

	// 현재 Possess 중인 Enemy의 DataAsset을 반환하는 함수
	const UNSEnemyData* GetControlledEnemyData() const;

	// 현재 Possess 중인 Enemy의 ASC를 반환하는 함수
	UAbilitySystemComponent* GetControlledEnemyASC() const;

	// 현재 Possess 중인 Enemy가 피격 경직 상태인지 반환하는 함수
	bool IsControlledEnemyHitReacting() const;

	// 현재 몬스터의 Health / MaxHealth 기준 체력 비율을 반환하는 함수
	float GetControlledEnemyHealthRatio() const;

	// 현재 체력 비율 기준으로 Enemy Phase 변경 여부를 확인하는 함수
	void UpdateEnemyPhase();

	// Phase 전환 중 공격 패턴 선택을 막고 있는지 확인하는 함수
	bool IsPhasePatternLocked() const;

	// 현재 Possess 중인 Enemy의 Phase Component를 반환하는 함수
	UNSEnemyPhaseComponent* GetEnemyPhaseComponent() const;

	// Phase Component 상태를 Blackboard에 반영하는 함수
	void SyncPhaseBlackboard(const UNSEnemyPhaseComponent* PhaseComponent);

	// 공통 Blackboard 기본값을 초기화하는 함수
	void InitCommonBlackboardState();

	// 공통 Blackboard 상태를 현재 Enemy 상태와 동기화하는 함수
	void SyncCommonStateBlackboard();

	// 현재 Possess 중인 Enemy의 CurrentAttackRow를 초기화하는 함수
	void ClearControlledAttackRow();

	// bCanAttack Blackboard 값을 변경하는 함수
	void SetCanAttackBB(bool bCanAttack);

	// bIsAttacking Blackboard 값을 변경하는 함수
	void SetIsAttackingBB(bool bIsAttacking);

	// bIsHitReacting Blackboard 값을 변경하는 함수
	void SetHitReactingBB(bool bHitReacting);

	// bIsAttacking Blackboard 값을 반환하는 함수
	bool IsAttackingBB() const;

	// bIsHitReacting Blackboard 값을 반환하는 함수
	bool IsHitReactingBB() const;

	// TargetActor Blackboard 값을 변경하는 함수
	void SetTargetActorBB(AActor* TargetActor);

	// AttackActor Blackboard 값을 변경하는 함수
	void SetAttackActorBB(AActor* AttackActor);

	// TargetActor Blackboard 값을 반환하는 함수
	AActor* GetTargetActorBB() const;

	// AttackActor Blackboard 값을 반환하는 함수
	AActor* GetAttackActorBB() const;

	// TargetLastKnownLocation Blackboard 값을 변경하는 함수
	void SetTargetLastKnownLocationBB(const FVector& LastKnownLocation);

	// bHasTargetLineOfSight Blackboard 값을 변경하는 함수
	void SetHasTargetLineOfSightBB(bool bHasLineOfSight);

	// 공통 타깃 관련 Blackboard 값을 초기화하는 함수
	void ClearCommonTargetBB(bool bClearCanAttack);

protected:
	// StateTree 방식 Enemy가 사용할 AI Brain Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	// 현재 Brain이 사용하는 Blackboard Component 캐시
	UPROPERTY(Transient)
	TObjectPtr<UBlackboardComponent> CachedBBComp;

	// Phase Lock 상태를 저장할 Blackboard 키 이름
	FName PhasePatternLockedKey = TEXT("bPhasePatternLocked");

	// 현재 PhaseId를 저장할 Blackboard 키 이름
	FName CurrentPhaseIdKey = TEXT("CurrentPhaseId");
	
	// ---@민재 : 비행 컴포넌트---
	// 현재 Possess 중인 폰의 비행 로코모션을 반환 (비행체가 아니면 nullptr)
	UNSFlyingLocomotionComponent* GetControlledFlyingLocomotion() const;

	// 비행 로코모션의 회전 타겟 지정 (비행체 아니면 no-op). nullptr이면 velocity 방향 회전으로 복귀
	void SyncFlyingRotationTarget(AActor* Target);
	// ---
	
	
	// 현재 공격 실행 중인지 저장할 Blackboard 키 이름
	FName IsAttackingKey = TEXT("bIsAttacking");
};
