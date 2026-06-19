// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyAttackBase.h"
#include "GA_EnemyAttackMelee.generated.h"

/**
 * 몬스터의 근접 공격 처리
 */
UCLASS()
class NEOSANCTUM_API UGA_EnemyAttackMelee : public UGA_EnemyAttackBase
{
	GENERATED_BODY()

public:
	UGA_EnemyAttackMelee();

protected:
	virtual void InitializeAttack() override;
	virtual void HandleAttackEvent(
		const FGameplayEventData& Payload) override;
	
	float AttackTraceDistance = 100.0f;
	float AttackTraceRadius = 8.0f;

private:
	TSet<uint32> DamagedTraceWindowIds;
	
	// 디버그 설정
	void DrawAttackTraceDebug(
		const FVector& Start,
		const FVector& End,
		bool bHit,
		const FHitResult& HitResult) const;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Debug")
	bool bDrawAttackTraceDebug = false;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Debug", meta = (ClampMin = "0.0"))
	float AttackTraceDebugDuration = 1.0f;
};
