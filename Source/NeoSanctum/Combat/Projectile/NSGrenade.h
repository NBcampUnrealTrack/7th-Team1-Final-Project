// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSThrowProjectileBase.h"
#include "NeoSanctum/GAS/GameplayAbility/GA_ThrowProjectile.h"
#include "NSGrenade.generated.h"

UCLASS()
class NEOSANCTUM_API ANSGrenade : public ANSThrowProjectileBase
{
	GENERATED_BODY()

public:
	ANSGrenade();

	// GA_ThrowProjectile에서 Explosive 타입 설정을 주입받아 런타임 폭발 동작을 구성
	void InitializeGrenade(const FNSExplosiveTypeConfig& InConfig);

protected:
	virtual void BeginPlay() override;
	
private:
	UFUNCTION()
	void OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	void Explode();

	// 실제 폭발 처리를 수행하며, Location은 데미지 중심점, Normal은 Cue 방향, TraceStart는 차폐 판정 시작점으로 사용
	void ExplodeAt(
		const FVector& ExplosionLocation,
		const FVector& ExplosionNormal,
		const FVector& OcclusionTraceStart
	);

	// 폭발 사운드/VFX를 GameplayCue로 실행
	void ExecuteExplosionCue(const FVector& ExplosionLocation, const FVector& ExplosionNormal);

	void ReportExplosionNoise(const FVector& ExplosionLocation) const;
	
	// 폭발 반경 안의 Pawn 후보를 중복 없이 수집
	void FindExplosionTargetActors(const FVector& ExplosionLocation, TArray<AActor*>& OutTargetActors) const;

	// 폭발 지점과 대상 사이에 WorldStatic 차폐물이 있는 대상을 제거
	void FilterOccludedExplosionTargets(const FVector& TraceStart, TArray<AActor*>& TargetActors) const;
	bool IsExplosionTargetOccluded(const FVector& TraceStart, const AActor* TargetActor) const;

	// 차폐 필터를 통과한 대상에게 GameplayEffect 데미지를 적용
	void ApplyExplosionDamage(const FVector& ExplosionLocation, const TArray<AActor*>& TargetActors);

	// CombatStat.ExplosionRadius 런타임 payload에서 폭발 반경을 조회
	float GetExplosionRadius() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Grenade")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(VisibleAnywhere, Category = "Grenade")
	FGameplayTag ExplosionCueTag;

	UPROPERTY(VisibleAnywhere, Category = "Grenade")
	float FuseTime = 2.0f;

	UPROPERTY(VisibleAnywhere, Category = "Grenade")
	bool bExplodeOnImpact = false;

	UPROPERTY(VisibleAnywhere, Category = "Grenade")
	float LifeSpanAfterThrow = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Grenade|Explosion")
	bool bExcludeOccludedExplosionTargets = true;

	UPROPERTY(EditDefaultsOnly, Category = "Grenade|Explosion")
	float ExplosionOcclusionTraceStartOffset = 5.0f;

	// 폭발 반경 디버그 구체 표시
	UPROPERTY(EditDefaultsOnly, Category = "Grenade|Debug")
	bool bDrawDebugExplosion = true;

	// 폭발 지점에서 대상까지의 차폐 판정 라인 표시
	UPROPERTY(EditDefaultsOnly, Category = "Grenade|Debug")
	bool bDrawDebugExplosionOcclusion = true;

	// 타이머와 충돌 이벤트가 겹쳐도 폭발을 딱 한 번만 수행하기 위한 플래그
	bool bExploded = false;

	FTimerHandle FuseTimerHandle;
};
