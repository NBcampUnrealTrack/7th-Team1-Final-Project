// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GA_ThrowProjectile.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UStaticMesh;
class UStaticMeshComponent;
class ANSThrowProjectileBase;

/**
 * Projectile을 포물선 형태로 던지기 위한 GA
 * Input이 Hold되는 동안 궤적 + 탄착지점 프리뷰를 보여주고 
 * Input이 Release 되는 순간 프리뷰를 제거하고 Projectile을 스폰
 */
UCLASS(Abstract, Blueprintable)
class NEOSANCTUM_API UGA_ThrowProjectile : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_ThrowProjectile();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

protected:
	UFUNCTION()
	void OnThrowMontageCompleted();

	UFUNCTION()
	void OnThrowMontageInterrupted();

	UFUNCTION()
	void OnAttachProjectileEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnThrowProjectileEventReceived(FGameplayEventData Payload);

protected:
	void StartGameplayEventTasks();
	void AttachHeldMesh();
	void DestroyHeldMesh();
	
	void SpawnProjectile();
	
protected:
	FTransform GetProjectileSpawnTransform() const;
	FVector GetProjectileThrowDirection() const;
	
	void AddDeactivateHandIKTag();
	void RemoveDeactivateHandIKTag();
	
protected:
	// 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Montage")
	TObjectPtr<UAnimMontage> AnimMontage;
	
	// Release 단계의 애니메이션 몽타주 섹션 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Montage")
	FName ReleaseSectionName = TEXT("Release");
	
	// Hold 단계에서 손에 메쉬를 붙히는 타이밍을 정해주는 Event Tag
	FGameplayTag AttachTag;
	
	// Release 단계에서 손에 붙어있던 메쉬를 제거하고 Projectile을 던지는 타이밍을 정해주는 Event Tag
	FGameplayTag ReleaseTag;
	
protected:
	// Hold 단계에서 손에 붙힐 메쉬
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Projectile")
	TObjectPtr<UStaticMesh> HoldStaticMesh;
	
	// Hold 단계에서 메쉬를 붙힐 소켓
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Projectile")
	FName HoldAttachSocketName = TEXT("Weapon_l");
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Projectile")
	FTransform HoldRelativeTransform = FTransform::Identity;

protected:
	// 발사할 Projectile 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Projectile")
	TSubclassOf<ANSThrowProjectileBase> ProjectileClass;

	// Projectile 발사 위치 소켓
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Projectile")
	FName ProjectileSpawnSocketName = TEXT("Weapon_l");

private:
	// 몽타주 재생 AbilityTask
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ThrowMontageTask;
	
	// 메쉬를 손에 붙히는 타이밍 GameplayEvent를 기다리는 Task	
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> AttachProjectileEventTask;
	
	// Projectile 액터를 발사하면서 손에 붙어있던 메쉬를 제거하는 타이밍 GameplayEvent를 기다리는 Task
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ThrowProjectileEventTask;
	
	// 캐릭터에 잠깐 붙히게 될 MeshComponent
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> HoldMeshComponent;

	bool bDeactivateHandIKTagAdded = false;
};
