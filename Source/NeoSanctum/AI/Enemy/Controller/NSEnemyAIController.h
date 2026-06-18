// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "Perception/AIPerceptionTypes.h" // FAIStimulus
#include "NSEnemyAIController.generated.h"

class UGameplayAbility;
class UNSEnemyData;
class ANSEnemyCharacterBase;
struct FNSEnemyAttackDefinition;

UCLASS()
class NEOSANCTUM_API ANSEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ANSEnemyAIController();
	virtual void Tick(float DeltaTime) override;

	/*
	 * AI Controller가 적을 필터링하기 위한 TeamId 조회
	 */
	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(static_cast<uint8>(ETeamId::Enemy));
	}

	// 타 진영을 보았을 때의 적대/우호 규칙 정의 수립
	virtual ETeamAttitude::Type GetTeamAttitudeTo(const AActor& Other) const;

	// 타겟과의 현재 거리/방향/시야 기준으로 사용 가능한 공격이 하나라도 있는지 확인
	bool CanUseAnyAttackByDistance();

	// 타겟과의 실시간 거리 기준으로 현재 사용할 공격 GA를 선택
	const FNSEnemyAttackDefinition* GetAttackDefinitionByDistance();
	
	// Blackboard에 저장된 현재 공격 대상을 반환
	AActor* GetCurrentTargetActor() const;

protected:
	// 빙의 시점에 에디터에서 할당된 BT 가동
	virtual void OnPossess(APawn* InPawn) override;

	// 시각 센서가 갱신될 때 블랙보드로 데이터를 전달할 콜백 함수
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
private:
	// 대상이 체력 데이터를 갖고 있는지, 살아 있는 유효한 타겟인지 검증
	bool IsValidLivingTarget(const AActor* Target) const;
	
	const FNSEnemyAttackDefinition* FindAttackDefinitionByDistance(bool bSelectWeightedAttack);

	bool CanUseAttackDefinition(
		const FNSEnemyAttackDefinition& AttackDefinition,
		const AActor* TargetActor,
		float Distance,
		bool bHasLineOfSight) const;

protected:
	// 시야/청각 설정 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	// 타겟을 정면으로 바라봤다고 판정할 최대 각도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Combat", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float AttackFacingAngleDegrees = 12.0f;

private:
	// 타겟 액터
	FName TargetActorKey = TEXT("TargetActor");

	UPROPERTY(Transient)
	TObjectPtr<UBlackboardComponent> CachedBBComp;
};
