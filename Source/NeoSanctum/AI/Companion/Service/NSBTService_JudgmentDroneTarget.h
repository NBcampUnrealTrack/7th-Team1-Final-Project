// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "NeoSanctum/AI/Companion/State/NSCompanionTypes.h"
#include "NSBTService_JudgmentDroneTarget.generated.h"


class ANSCompanionDroneAI;

UCLASS()
class NEOSANCTUM_API UNSBTService_JudgmentDroneTarget : public UBTService
{
	GENERATED_BODY()
public:
	UNSBTService_JudgmentDroneTarget();
	
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	
protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	
protected:
	// @민재 : 블랙보드 키 변수 에디터 할당
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector StateKey;
	
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector MoveTargetKey;
	
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetDropIdKey;
	
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector EnemyActorKey;
	
	// @민재 : 재화 관련 변수
	
	UPROPERTY(EditAnywhere, Category="DroneAI|Currency")
	float CurrencyDetectionRadius = 1500.f;
	
	// @민재 : 전투 관련 변수
	UPROPERTY(EditAnywhere, Category = "DroneAI|Combat")
	TSubclassOf<AActor> EnemyClass;
	
	UPROPERTY(EditAnywhere, Category="DroneAI|Combat")
	float CombatDetectionRadius = 2000.f;
	
	UPROPERTY(EditAnywhere, Category="DroneAI|Combat")
	float EnemyDistance = 350.f;
	
	UPROPERTY(EditAnywhere, Category="DroneAI|Combat")
	FGameplayTag FireAbilityTag;
	
	UPROPERTY(EditAnywhere, Category="DroneAI|Combat")
	TArray<TEnumAsByte<EObjectTypeQuery>> EnemyObjectTypes;
	
	// @민재 : 오너 거리
	UPROPERTY(EditAnywhere, Category="DroneAI|Follow")
	FVector FollowOffset = FVector(-100.f, 0.f, 100.f);

private:
	ECompanionState EvaluateState(ANSCompanionDroneAI* CompanionPawn, UBlackboardComponent* BB) const;
	
	// @민재 : 감지범위구체
	AActor* FindNearestActor(
		AActor* InActor, 
		TSubclassOf<AActor> FilterClass, 
		float Radius,
		const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
		bool bRequireAliveEnemy = false) const;
	
	// @민재 : 적과의 거리 계산
	FVector ComputeStandoffPosition(const AActor* Drone, const AActor* Enemy) const;
	
	// @민재 : 발사 어빌리티 활성화 시도
	void TryActivateFire(const ANSCompanionDroneAI* Drone) const;
	
};
