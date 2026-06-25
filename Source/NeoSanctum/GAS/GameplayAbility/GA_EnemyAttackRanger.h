// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyAttackBase.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
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
	virtual void HandleAttackEvent(const FGameplayEventData& Payload) override;

	// 공격 Montage의 Fire 섹션을 반복할 횟수
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Burst", meta = (ClampMin = "1"))
	int32 BurstCount = 3;

	// 투사체 초당 이동 거리
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile", meta = (ClampMin = "1.0"))
	float ProjectileSpeed = 1000.0f;

	// 투사체 최대 수명 시간
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile", meta = (ClampMin = "0.01"))
	float ProjectileMaxLifeTime = 30.0f;
	
	// 투사체 충돌 반지름
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile", meta = (ClampMin = "0.1"))
	float ProjectileRadius = 10.0f;
	
	// 투사체 Trace Channel
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile")
	TEnumAsByte<ECollisionChannel> ProjectileTraceChannel = NSCollisionChannels::EnemyProjectile;

private:
	// 현재 발사 이벤트 수
	int32 CurrentShotCount = 0;
};
