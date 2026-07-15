// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSThrowProjectileBase.h"
#include "NeoSanctum/GAS/GameplayAbility/GA_ThrowProjectile.h"
#include "NSVanguardBarrierFieldProjectile.generated.h"

/**
 * Vanguard 배리어 필드 스포너 Actor
 * 첫 충돌 또는 최대 사거리 도달 지점에 배리어 필드를 설치하는 투사체
 */
UCLASS()
class NEOSANCTUM_API ANSVanguardBarrierFieldProjectile : public ANSThrowProjectileBase
{
	GENERATED_BODY()

public:
	ANSVanguardBarrierFieldProjectile();

	// GA_ThrowProjectile에서 ShieldField 타입 설정 주입
	void InitializeBarrierFieldProjectile(const FNSShieldFieldTypeConfig& InConfig);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void UpdateVisualRoll(float DeltaSeconds) const;

	UFUNCTION()
	void OnProjectileStopped(const FHitResult& ImpactResult);

	// 배리어 필드 설치
	void DeployField(const FVector& DeployLocation, const FVector& DeployNormal);

private:
	// CombatStat.Radius 기반 필드 반경 조회
	float GetFieldRadius() const;
	// CombatStat.Duration 기반 필드 지속시간 조회
	float GetFieldDuration() const;
	// CombatStat.DamageInterval 기반 주기 피해 간격 조회
	float GetDamageInterval() const;
	// CombatStat.SkillRange 기반 최대 이동 거리 조회
	float GetMaxTravelDistance() const;

private:
	// 필드 스폰과 피해 처리에 사용할 설정
	UPROPERTY(Transient)
	FNSShieldFieldTypeConfig ShieldFieldConfig;

	// 최대 이동 거리 판정용 시작 위치
	FVector StartLocation = FVector::ZeroVector;
	// 중복 설치 방지 플래그
	bool bFieldDeployed = false;

	// 투척 중 메쉬 Roll 회전 속도
	UPROPERTY(EditDefaultsOnly, Category = "BarrierFieldProjectile|Visual")
	float VisualRollSpeed = 1440.0f;
};
