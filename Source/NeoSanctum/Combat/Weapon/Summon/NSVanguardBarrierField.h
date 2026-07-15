// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSBarrierBase.h"
#include "NeoSanctum/GAS/GameplayAbility/GA_ThrowProjectile.h"
#include "NSVanguardBarrierField.generated.h"

class USphereComponent;
class UGameplayEffect;

/**
 * Vanguard 투척 배리어 필드 Actor
 * 체력과 지속시간을 가진 구형 방어막 및 주기적으로 피해를 주는 액터
 */
UCLASS()
class NEOSANCTUM_API ANSVanguardBarrierField : public ANSBarrierBase
{
	GENERATED_BODY()

public:
	ANSVanguardBarrierField();

	// GA_ThrowProjectile의 ShieldField 타입 설정에 기반해서 Initialize
	void InitializeBarrierField(
		const FNSShieldFieldTypeConfig& InConfig,
		APawn* InOwningPawn,
		AController* InOwningController,
		float InRadius,
		float InDuration,
		float InDamageInterval,
		const TArray<FNSSetByCallerMagnitude>& InSetByCallerMagnitudes
	);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void ApplyCollisionRadius(float Radius) override;

private:
	// 주기 피해 타이머 시작
	void StartDamageTimer();
	// 주기 피해 처리
	void ApplyPeriodicDamage();
	// 피해 반경 안의 대상 후보 수집
	void FindDamageTargetActors(TArray<AActor*>& OutTargetActors) const;
	// 대상에게 GameplayEffect 데미지 적용
	void ApplyDamageToTargets(const TArray<AActor*>& TargetActors);

private:
	// 배리어 충돌 및 방어막 범위 표현에 사용하는 구형 Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BarrierField|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> SphereBarrierCollisionComponent;

	// 지속 피해 GameplayEffect
	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 지속 피해 GE에 전달할 SetByCaller payload
	UPROPERTY(Transient)
	TArray<FNSSetByCallerMagnitude> DamageSetByCallerMagnitudes;

	// 기본 지속 피해 간격
	float DamageInterval = 0.5f;

	FTimerHandle DamageTimerHandle;
};
