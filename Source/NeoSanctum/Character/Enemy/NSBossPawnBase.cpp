// Copyright 2026 One Team. All rights reserved.

#include "NSBossPawnBase.h"

#include "Components/CapsuleComponent.h"
#include "NeoSanctum/Combat/Component/NSBossModeComponent.h"
#include "NeoSanctum/Combat/Component/NSBossTargetComponent.h"
#include "GameFramework/Controller.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NeoSanctum/Character/Animation/NSBossAnimInstance.h"
#include "NeoSanctum/Collision/NSCollisionProfiles.h"
#include "NeoSanctum/Combat/Component/Artillery/NSBossArtilleryComponent.h"
#include "NeoSanctum/Combat/Cosmetic/NSEnemyCosmeticComponent.h"
#include "NeoSanctum/System/Component/NSDissolveComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"

ANSBossPawnBase::ANSBossPawnBase()
{
	BossModeComponent = CreateDefaultSubobject<UNSBossModeComponent>(TEXT("BossModeComponent"));
	BossTargetComponent = CreateDefaultSubobject<UNSBossTargetComponent>(TEXT("BossTargetComponent"));
	CosmeticComponent = CreateDefaultSubobject<UNSEnemyCosmeticComponent>(TEXT("CosmeticComponent"));
	BossArtilleryComponent = CreateDefaultSubobject<UNSBossArtilleryComponent>(TEXT("BossArtilleryComponent"));

	if (UCapsuleComponent* BossCollisionComponent = GetCollisionComponent())
	{
		BossCollisionComponent->SetCollisionProfileName(NSCollisionProfiles::EnemyBoss);
	}

	DeathExplosionCosmeticEventTag = NSGameplayTags::Cosmetic_Enemy_TitanWalker_Death_Explosion;
}

void ANSBossPawnBase::BeginPlay()
{
	Super::BeginPlay();

	if (BossModeComponent)
	{
		BossModeComponent->InitializeMode();
	}
}

void ANSBossPawnBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathDissolveTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void ANSBossPawnBase::HandleDeathStarted()
{
	Super::HandleDeathStarted();

	if (!HasAuthority())
	{
		return;
	}

	SendDeathExplosionCosmeticEvent();

	if (bStartDissolveAfterDeathExplosion)
	{
		GetWorld()->GetTimerManager().SetTimer(
			DeathDissolveTimerHandle,
			this,
			&ThisClass::StartDelayedDeathDissolve,
			DeathDissolveDelay,
			false);
	}
}


void ANSBossPawnBase::ApplyDeadState()
{
	Super::ApplyDeadState();

	if (BossTargetComponent)
	{
		BossTargetComponent->ResetTargets();
	}
}

void ANSBossPawnBase::HandleHitReactionStateChanged(bool bHitReacting)
{
	Super::HandleHitReactionStateChanged(bHitReacting);

	if (!bHitReacting || !bFaceTargetOnHitReaction || !HasAuthority())
	{
		return;
	}

	FaceCurrentTargetForHitReaction();
}

void ANSBossPawnBase::FaceCurrentTargetForHitReaction()
{
	if (USkeletalMeshComponent* MeshComponent = GetEnemyMesh())
	{
		if (UNSBossAnimInstance* BossAnimInstance =
			Cast<UNSBossAnimInstance>(MeshComponent->GetAnimInstance()))
		{
			BossAnimInstance->ResetCombatAimImmediate();
		}
	}

	const AActor* TargetActor = ThreatComponent
		                            ? ThreatComponent->GetCurrentTarget()
		                            : nullptr;

	if (!IsValid(TargetActor))
	{
		return;
	}

	FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.0f;

	if (!ToTarget.Normalize())
	{
		return;
	}

	FRotator TargetRotation = ToTarget.Rotation();
	TargetRotation.Pitch = 0.0f;
	TargetRotation.Roll = 0.0f;

	SetActorRotation(TargetRotation);

	if (AController* OwnerController = GetController())
	{
		OwnerController->SetControlRotation(TargetRotation);
	}
}

FName ANSBossPawnBase::GetAliveCollisionProfileName() const
{
	return NSCollisionProfiles::EnemyBoss;
}


void ANSBossPawnBase::SendDeathExplosionCosmeticEvent()
{
	if (!bPlayDeathExplosionCosmetic || !CosmeticComponent)
	{
		return;
	}

	FNSCosmeticEventNetData EventData;
	EventData.EventTag = DeathExplosionCosmeticEventTag.IsValid()
		                     ? DeathExplosionCosmeticEventTag
		                     : NSGameplayTags::Cosmetic_Enemy_TitanWalker_Death_Explosion;
	EventData.Phase = ENSCosmeticEventPhase::OneShot;
	EventData.Location = GetActorLocation();
	EventData.Direction = GetActorForwardVector();
	EventData.Duration = DeathDissolveDelay;

	CosmeticComponent->SendCosmeticEvent(EventData, true);
}

void ANSBossPawnBase::StartDelayedDeathDissolve()
{
	if (DissolveComponent)
	{
		DissolveComponent->StartDissolve(bDestroyAfterDeathDissolve);
	}
}
