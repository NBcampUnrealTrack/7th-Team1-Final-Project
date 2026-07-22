// Copyright 2026 One Team. All rights reserved.

#include "GA_EnemyDeath.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/System/Component/NSDissolveComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_EnemyDeath::UGA_EnemyDeath()
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;

	bServerRespectsRemoteAbilityCancellation = false;
	bRetriggerInstancedAbility = false;

	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::State_Dead);
	SetAssetTags(AssetTags);

	CancelAbilitiesWithTag.AddTag(NSGameplayTags::State_Enemy_Combat);
	CancelAbilitiesWithTag.AddTag(NSGameplayTags::Ability_Enemy_Attack);
	CancelAbilitiesWithTag.AddTag(NSGameplayTags::Ability_Enemy_HitReaction);

	BlockAbilitiesWithTag.AddTag(NSGameplayTags::State_Enemy_Combat);
	BlockAbilitiesWithTag.AddTag(NSGameplayTags::Ability_Enemy_Attack);
	BlockAbilitiesWithTag.AddTag(NSGameplayTags::Ability_Enemy_HitReaction);

	ActivationOwnedTags.AddTag(NSGameplayTags::State_Dead);
}

void UGA_EnemyDeath::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	USkeletalMeshComponent* MeshComponent = GetDeathMesh();

	if (!AvatarActor || !MeshComponent)
	{
		FinishDeathAbility(true);
		return;
	}

	DisableDeathCollision(AvatarActor, MeshComponent);

	const float StartDelay = FMath::Max(DeathPresentationStartDelay, 0.0f);
	if (StartDelay > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			DeathPresentationStartTimerHandle,
			this,
			&ThisClass::StartDeathPresentation,
			StartDelay,
			false);

		return;
	}

	StartDeathPresentation();
}

void UGA_EnemyDeath::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathPresentationStartTimerHandle);
		World->GetTimerManager().ClearTimer(DeathFreezeTimerHandle);
	}

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UGA_EnemyDeath::OnDeathMontageCompleted()
{
	if (!IsActive())
	{
		return;
	}

	if (USkeletalMeshComponent* MeshComponent = GetDeathMesh())
	{
		MeshComponent->bPauseAnims = true;
	}

	StartDissolve();
	FinishDeathAbility(false);
}

void UGA_EnemyDeath::OnDeathMontageInterrupted()
{
	if (!IsActive())
	{
		return;
	}

	ApplyRagdoll(GetDeathMesh());
	StartDissolve();
	FinishDeathAbility(true);
}

USkeletalMeshComponent* UGA_EnemyDeath::GetDeathMesh() const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return nullptr;
	}

	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(AvatarActor);
	if (EnemyAgent)
	{
		if (USkeletalMeshComponent* EnemyMesh = EnemyAgent->GetEnemyMesh())
		{
			return EnemyMesh;
		}
	}

	return AvatarActor->FindComponentByClass<USkeletalMeshComponent>();
}

void UGA_EnemyDeath::DisableDeathCollision(
	AActor* AvatarActor,
	USkeletalMeshComponent* MeshComponent) const
{
	if (!AvatarActor)
	{
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(AvatarActor))
	{
		if (UCapsuleComponent* CapsuleComponent = Character->GetCapsuleComponent())
		{
			CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			CapsuleComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		}

		if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
			MovementComponent->DisableMovement();
		}

		return;
	}

	UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(AvatarActor->GetRootComponent());
	if (RootPrimitive && RootPrimitive != MeshComponent)
	{
		RootPrimitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RootPrimitive->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
}

bool UGA_EnemyDeath::ApplyRagdoll(USkeletalMeshComponent* MeshComponent) const
{
	if (!MeshComponent || !MeshComponent->GetPhysicsAsset())
	{
		return false;
	}

	MeshComponent->bPauseAnims = false;
	MeshComponent->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetAllBodiesSimulatePhysics(true);
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->WakeAllRigidBodies();

	return MeshComponent->IsAnySimulatingPhysics();
}

void UGA_EnemyDeath::StartDissolve() const
{
	if (!bStartDissolve)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return;
	}

	if (UNSDissolveComponent* DissolveComponent =
		AvatarActor->FindComponentByClass<UNSDissolveComponent>())
	{
		DissolveComponent->StartDissolve(bDestroyAfterDissolve);
	}
}

void UGA_EnemyDeath::FinishDeathAbility(bool bWasCancelled)
{
	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		true,
		bWasCancelled);
}


void UGA_EnemyDeath::FreezeDeathPose()
{
	USkeletalMeshComponent* MeshComponent = GetDeathMesh();
	if (!MeshComponent || !DeathMontage)
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	const float FreezeTime = FMath::Max(
		DeathMontage->GetPlayLength() - 0.03f,
		0.0f);

	AnimInstance->Montage_SetPosition(DeathMontage, FreezeTime);
	AnimInstance->Montage_Pause(DeathMontage);

	MeshComponent->bPauseAnims = true;
}

void UGA_EnemyDeath::StartDeathPresentation()
{
	USkeletalMeshComponent* MeshComponent = GetDeathMesh();

	if (!MeshComponent)
	{
		FinishDeathAbility(true);
		return;
	}

	if (!DeathMontage)
	{
		const bool bRagdollStarted = ApplyRagdoll(MeshComponent);

		if (!bRagdollStarted)
		{
			MeshComponent->bPauseAnims = true;
			MeshComponent->SetComponentTickEnabled(false);
		}

		StartDissolve();
		FinishDeathAbility(false);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			TEXT("DeathMontageTask"),
			DeathMontage,
			FMath::Max(MontagePlayRate, 0.01f),
			StartSectionName,
			false,
			1.0f,
			0.0f);

	if (!MontageTask)
	{
		ApplyRagdoll(MeshComponent);
		StartDissolve();
		FinishDeathAbility(true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnDeathMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnDeathMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnDeathMontageInterrupted);
	MontageTask->ReadyForActivation();

	const float FreezeDelay =
		DeathMontage->GetPlayLength() / FMath::Max(MontagePlayRate, 0.01f) - 0.03f;

	GetWorld()->GetTimerManager().SetTimer(
		DeathFreezeTimerHandle,
		this,
		&ThisClass::FreezeDeathPose,
		FMath::Max(FreezeDelay, 0.01f),
		false);
}
