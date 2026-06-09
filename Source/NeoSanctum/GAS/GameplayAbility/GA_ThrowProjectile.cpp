// Copyright 2026 One Team. All rights reserved.


#include "GA_ThrowProjectile.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "NeoSanctum/Combat/Projectile/NSThrowProjectileBase.h"
#include "NeoSanctum/Combat/Projectile/NSTurretSpawner.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_ThrowProjectile::UGA_ThrowProjectile()
{
	ActivationPolicy = ENSAbilityActivationPolicy::OnInputTriggered;
}

void UGA_ThrowProjectile::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (!AnimMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 몽타주 시작
	ThrowMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		TEXT("ThrowMontageTask"),
		AnimMontage,
		1.0f,
		NAME_None,
		false,
		1.0f,
		0.0f
	);

	if (!ThrowMontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartGameplayEventTasks();
	
	ThrowMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnThrowMontageCompleted);
	ThrowMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnThrowMontageInterrupted);
	ThrowMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnThrowMontageInterrupted);
	AddDeactivateHandIKTag();
	ThrowMontageTask->ReadyForActivation();
	
	if (ActorInfo->IsLocallyControlled())
	{
		// TODO : 던지기 궤적 + 탄착 지점 프리뷰
	}
}

void UGA_ThrowProjectile::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	if (ActorInfo && ActorInfo->IsLocallyControlled())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			
			ASC->CurrentMontageJumpToSection(ReleaseSectionName);
		}
	}
}

void UGA_ThrowProjectile::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// TODO : 프리뷰 종료
	DestroyHeldMesh();
	RemoveDeactivateHandIKTag();

	if (AttachProjectileEventTask)
	{
		AttachProjectileEventTask->EndTask();
		AttachProjectileEventTask = nullptr;
	}

	if (ThrowProjectileEventTask)
	{
		ThrowProjectileEventTask->EndTask();
		ThrowProjectileEventTask = nullptr;
	}

	// 몽타주 종료
	if (ThrowMontageTask)
	{
		ThrowMontageTask->EndTask();
		ThrowMontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_ThrowProjectile::OnThrowMontageCompleted()
{
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		false
	);
}

void UGA_ThrowProjectile::OnThrowMontageInterrupted()
{
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		true
	);
}

void UGA_ThrowProjectile::OnAttachProjectileEventReceived(FGameplayEventData Payload)
{
	AttachHeldMesh();
}

void UGA_ThrowProjectile::OnThrowProjectileEventReceived(FGameplayEventData Payload)
{
	DestroyHeldMesh();
	SpawnProjectile();
}

void UGA_ThrowProjectile::StartGameplayEventTasks()
{
	// AttachTag에서 설정한 EventTag 대기
	AttachProjectileEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		AttachTag,
		nullptr,
		false,
		false
	);
	
	// 바인딩
	if (AttachProjectileEventTask)
	{
		AttachProjectileEventTask->EventReceived.AddDynamic(this, &ThisClass::OnAttachProjectileEventReceived);
		AttachProjectileEventTask->ReadyForActivation();
	}
	
	// ReleaseTag에서 설정한 EventTag 대기
	ThrowProjectileEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		ReleaseTag,
		nullptr,
		false,
		false
	);
	
	// 바인딩
	if (ThrowProjectileEventTask)
	{
		ThrowProjectileEventTask->EventReceived.AddDynamic(this, &ThisClass::OnThrowProjectileEventReceived);
		ThrowProjectileEventTask->ReadyForActivation();
	}
}

void UGA_ThrowProjectile::AttachHeldMesh()
{
	if (HoldMeshComponent || !HoldStaticMesh)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->GetMesh())
	{
		return;
	}
	
	HoldMeshComponent = NewObject<UStaticMeshComponent>(Character);
	if (!HoldMeshComponent)
	{
		return;
	}
	
	// 메쉬 컴포넌트 설정하고 캐릭터의 Weapon_l 소켓에 부착
	HoldMeshComponent->SetStaticMesh(HoldStaticMesh);
	HoldMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HoldMeshComponent->SetGenerateOverlapEvents(false);
	HoldMeshComponent->RegisterComponent();
	HoldMeshComponent->AttachToComponent(
		Character->GetMesh(),
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		HoldAttachSocketName
	);
	HoldMeshComponent->SetRelativeTransform(HoldRelativeTransform);
}

void UGA_ThrowProjectile::DestroyHeldMesh()
{
	if (!HoldMeshComponent)
	{
		return;
	}
	
	// 부착해두었던 MeshComponent 제거
	HoldMeshComponent->DestroyComponent();
	HoldMeshComponent = nullptr;
}

void UGA_ThrowProjectile::SpawnProjectile()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	
	if (!AvatarActor || !AvatarActor->HasAuthority() || !World || !ProjectileClass)
	{
		return;
	}
	
	APawn* OwningPawn = Cast<APawn>(AvatarActor);
	AController* OwningController = OwningPawn ? OwningPawn->GetController() : nullptr;
	const FTransform SpawnTransform = GetProjectileSpawnTransform();
	const FVector ThrowDirection = GetProjectileThrowDirection();
	
	// SpawnActor를 통해 Projectile Actor 스폰
	ANSThrowProjectileBase* Projectile = World->SpawnActor<ANSThrowProjectileBase>(
		ProjectileClass,
		SpawnTransform
	);
	
	// Projectile Initialize
	if (Projectile)
	{
		Projectile->InitializeThrowActor(OwningPawn, OwningController, ThrowDirection);

		if (ProjectileAbilityConfig.ProjectileType == EProjectileType::TurretSpawner)
		{
			if (ANSTurretSpawner* TurretSpawner = Cast<ANSTurretSpawner>(Projectile))
			{
				TurretSpawner->InitializeTurretSpawner(ProjectileAbilityConfig.TurretSpawnerTypeConfig);
			}
		}
	}
}

FTransform UGA_ThrowProjectile::GetProjectileSpawnTransform() const
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && Character->GetMesh() && !ProjectileSpawnSocketName.IsNone())
	{
		return Character->GetMesh()->GetSocketTransform(ProjectileSpawnSocketName, RTS_World);
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	return AvatarActor ? AvatarActor->GetActorTransform() : FTransform::Identity;
}

FVector UGA_ThrowProjectile::GetProjectileThrowDirection() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return FVector::ZeroVector;
	}

	return AvatarActor->GetActorForwardVector();
}

void UGA_ThrowProjectile::AddDeactivateHandIKTag()
{
	if (bDeactivateHandIKTagAdded)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(NSGameplayTags::State_Deactivate_HandIK);
		bDeactivateHandIKTagAdded = true;
	}
}

void UGA_ThrowProjectile::RemoveDeactivateHandIKTag()
{
	if (!bDeactivateHandIKTagAdded)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(NSGameplayTags::State_Deactivate_HandIK);
	}

	bDeactivateHandIKTagAdded = false;
}
