// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "NSBTService_JudgmentDroneTarget.generated.h"

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
	FBlackboardKeySelector MoveTargetKey;
	
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetActorKey;
	
	// @민재 : 재화 관련 변수
	UPROPERTY(EditAnywhere, Category = "DroneAI|Currency")
	TSubclassOf<AActor> CurrencyClass;
	
	UPROPERTY(EditAnywhere, Category="DroneAI|Currency")
	float CurrencyDetectionRadius = 1500.f;
	
	// @민재 : 전투 관련 변수
	UPROPERTY(EditAnywhere, Category = "DroneAI|Combat")
	TSubclassOf<AActor> EnemyClass;
	
	UPROPERTY(EditAnywhere, Category="DroneAI|Combat")
	float CombatDetectionRadius = 2000.f;
	
	UPROPERTY(EditAnywhere, Category="DroneAI|Combat")
	float EnemyDistance = 350.f;
	
	// @민재 : 오너 거리
	UPROPERTY(EditAnywhere, Category="DroneAI|Follow")
	FVector FollowOffset = FVector(-100.f, 0.f, 100.f);
	
private:
	// @민재 : 감지범위구체
	AActor* FindNearestActor(AActor* InActor, TSubclassOf<AActor> FilterClass, float Radius) const;
	
};
