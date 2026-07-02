// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSEnemyTargetComponent.generated.h"

/**
 * Enemy의 공격 대상 판정, 엄폐물 Trace, 조준 위치 계산을 담당하는 컴포넌트입니다.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSEnemyTargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSEnemyTargetComponent();

	// TargetActor까지 Trace하여 실제 공격할 Actor를 반환하는 함수
	AActor* ResolveAttackActor(AActor* TargetActor, bool& bOutHasDirectLineOfSight) const;

	// 대상이 체력 데이터를 갖고 있고 살아 있는지 확인하는 함수
	bool IsValidLivingTarget(const AActor* Target) const;

protected:
	// 파괴 가능한 엄폐물이 시야를 막으면 해당 엄폐물을 공격 대상으로 삼을지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target")
	bool bAttackDestructibleCover = true;

	// Bounds 기준 조준 위치를 위로 보정하는 비율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target", meta = (ClampMin = "0.0"))
	float CoverAimZRatio = 0.15f;

private:
	// 컴포넌트 Owner를 Pawn으로 반환하는 함수
	APawn* GetOwnerPawn() const;

	// Actor Bounds 기준 조준 위치를 계산하는 함수
	FVector GetAimLocation(const AActor* Actor) const;

	// 공격 대상 판정 Trace 시작 위치를 계산하는 함수
	FVector GetTraceStart() const;
};
