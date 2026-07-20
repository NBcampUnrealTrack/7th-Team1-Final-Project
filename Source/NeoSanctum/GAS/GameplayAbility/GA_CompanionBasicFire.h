// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "GA_CompanionBasicFire.generated.h"


class UNSCompanionAttributeSet;
class ANSDroneProjectile;
class UGameplayEffect;

UCLASS(Abstract)
class NEOSANCTUM_API UGA_CompanionBasicFire : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_CompanionBasicFire();
	
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	
	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;
	
	// Getter
	FVector GetMuzzleSocketLocation(FName SocketName) const;
	const UNSCompanionAttributeSet* GetCompanionSet() const;
	AActor* GetCombatTarget() const;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	TSubclassOf<ANSDroneProjectile> ProjectileClass;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	FGameplayTag DamageSetTag;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	FGameplayTag CoolDownTag;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	TArray<FName> MuzzleSocketNames;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	FName EnemyTargetKey;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	FGameplayTag FireCueTag;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	bool bPredictiveAim = false;
	
	// @민재 : 타겟 움직임 예측 발사
	FVector ComputeAimDirection(const FVector& Muzzle, AActor* Target) const;
	
	// 변경: 드론 기준 1회 검증 (out 파라미터 제거)
	bool CanFireAt(AActor* Target) const;

	// 변경: 소켓 수(MuzzleCount)로 데미지 분할
	void FireProjectile(const FVector& Muzzle, const FVector& Dir, AActor* Target, int32 MuzzleCount);

	// 추가: 소켓별 발사 이펙트(GameplayCue) 실행
	void PlayFireCue(const FVector& Location, const FVector& Dir) const;
};
