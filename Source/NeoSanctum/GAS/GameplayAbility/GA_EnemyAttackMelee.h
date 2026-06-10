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
	virtual void PrepareForAttackMontage() override;
	virtual void HandleAttackEvent(
		const FGameplayEventData& Payload) override;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Melee")
	float AttackTraceDistance = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Melee")
	float AttackTraceRadius = 8.0f;

private:
	bool bHasHitThisAttack = false;
};
