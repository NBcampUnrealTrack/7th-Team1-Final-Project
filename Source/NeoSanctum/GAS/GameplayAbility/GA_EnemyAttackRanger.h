// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyAttackBase.h"
#include "GA_EnemyAttackRanger.generated.h"

/**
 * 몬스터의 원거리 공격 처리
 */
UCLASS()
class NEOSANCTUM_API UGA_EnemyAttackRanger : public UGA_EnemyAttackBase
{
	GENERATED_BODY()

public:
	UGA_EnemyAttackRanger();

protected:
	virtual void InitializeAttack() override;
	virtual void HandleAttackMontageCompleted() override;
	virtual void HandleAttackEvent(
		const FGameplayEventData& Payload) override;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Burst",
		meta = (ClampMin = "1"))
	int32 BurstCount = 3;

private:
	int32 CurrentShotCount = 0;
};
