// Copyright 2026 One Team. All rights reserved.

#include "GA_EnemyAttackMachineGun.h"

#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Components/PrimitiveComponent.h"
#include "NeoSanctum/AI/Enemy/Controller/NSBossAIController.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Character/Animation/NSTitanWalkerAnimInstance.h"
#include "NeoSanctum/Combat/Component/NSEnemyPartComponent.h"
#include "NeoSanctum/Combat/Cosmetic/NSEnemyCosmeticComponent.h"
#include "NeoSanctum/Combat/Projectile/NSProjectileManagerComponent.h"
#include "NeoSanctum/Combat/Projectile/NSProjectileTypes.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSoundSubsystem.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_EnemyAttackMachineGun::UGA_EnemyAttackMachineGun()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Enemy_TitanWalker_MachineGun);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(NSGameplayTags::State_Enemy_Combat);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}


void UGA_EnemyAttackMachineGun::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	UGameplayAbility::ActivateAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	InitializeAttack();

	if (!CachedAttackRow)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TArray<FTransform> MuzzleTransforms;
	GetCurrentMuzzleTransforms(MuzzleTransforms);

	if (MuzzleTransforms.IsEmpty())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartPreAimOrBurst();
}

void UGA_EnemyAttackMachineGun::InitializeAttack()
{
	CachedAttackRow = GetCurrentAttackRow();

	FiredCount = 0;
	NextMuzzleIndex = 0;
	bBurstStarted = false;
}

void UGA_EnemyAttackMachineGun::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ClearBurstTimer();

	CachedAttackRow = nullptr;
	FiredCount = 0;
	NextMuzzleIndex = 0;
	bBurstStarted = false;
	PreAimStartTime = 0.0f;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UGA_EnemyAttackMachineGun::GetCurrentMuzzleTransforms(
	TArray<FTransform>& OutTransforms) const
{
	OutTransforms.Reset();

	if (!CachedAttackRow)
	{
		return;
	}

	const UNSEnemyPartComponent* PartComponent = GetEnemyPartComponent();
	if (!PartComponent)
	{
		return;
	}

	PartComponent->GetMuzzleTransformsByAttackId(
		CachedAttackRow->AttackId,
		OutTransforms);
}

const FNSEnemyAttackRow* UGA_EnemyAttackMachineGun::GetCurrentAttackRow() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(AvatarActor);

	return EnemyAgent
		       ? EnemyAgent->GetCurrentAttackRow()
		       : nullptr;
}

UNSEnemyPartComponent* UGA_EnemyAttackMachineGun::GetEnemyPartComponent() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();

	return AvatarActor
		       ? AvatarActor->FindComponentByClass<UNSEnemyPartComponent>()
		       : nullptr;
}

ANSBossAIController* UGA_EnemyAttackMachineGun::GetBossController() const
{
	const APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!AvatarPawn)
	{
		return nullptr;
	}

	return Cast<ANSBossAIController>(AvatarPawn->GetController());
}

UNSProjectileManagerComponent* UGA_EnemyAttackMachineGun::GetProjectileManager() const
{
	const UWorld* World = GetWorld();
	const ANSRunGameState* RunGameState = World
		                                      ? World->GetGameState<ANSRunGameState>()
		                                      : nullptr;

	return RunGameState
		       ? RunGameState->GetProjectileManagerComponent()
		       : nullptr;
}

AActor* UGA_EnemyAttackMachineGun::ResolveAttackActor() const
{
	ANSBossAIController* BossController = GetBossController();
	if (!BossController)
	{
		return nullptr;
	}

	if (AActor* AttackActor = BossController->GetCurrentAttackActor())
	{
		return AttackActor;
	}

	return BossController->GetCurrentTargetActor();
}

FVector UGA_EnemyAttackMachineGun::ResolveAimPoint(
	const FNSEnemyAttackRow& AttackRow,
	const FTransform& MuzzleTransform,
	const AActor* AttackActor) const
{
	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	const FVector MuzzleForward = MuzzleTransform.GetRotation().GetForwardVector();

	if (AttackRow.AimMode == ENSEnemyAimMode::Forward || !IsValid(AttackActor))
	{
		return MuzzleLocation + MuzzleForward * AttackRow.Condition.MaxRange;
	}

	FVector TargetLocation = AttackActor->GetActorLocation();

	if (const UPrimitiveComponent* PrimitiveComponent =
		Cast<UPrimitiveComponent>(AttackActor->GetRootComponent()))
	{
		TargetLocation = PrimitiveComponent->Bounds.Origin;
	}

	if (AttackRow.AimMode == ENSEnemyAimMode::Predict)
	{
		const float ProjectileSpeed = FMath::Max(AttackRow.ProjectileData.Speed, 1.0f);
		const float Distance = FVector::Dist(MuzzleLocation, TargetLocation);
		const float TravelTime = Distance / ProjectileSpeed;

		TargetLocation += AttackActor->GetVelocity() * TravelTime;
	}

	if (AttackRow.AimMode == ENSEnemyAimMode::Ground)
	{
		TargetLocation.Z = AttackActor->GetActorLocation().Z;
	}

	return TargetLocation;
}

FVector UGA_EnemyAttackMachineGun::ResolveFireDirection(
	const FNSEnemyAttackRow& AttackRow,
	const FTransform& MuzzleTransform,
	const AActor* AttackActor) const
{
	const FVector MuzzleLocation = MuzzleTransform.GetLocation();

	FVector Direction =
		MuzzleTransform.GetRotation().GetForwardVector().GetSafeNormal();

	if (AttackRow.AimMode != ENSEnemyAimMode::Forward && IsValid(AttackActor))
	{
		const FVector AimPoint = ResolveAimPoint(
			AttackRow,
			MuzzleTransform,
			AttackActor);

		const FVector TargetDirection =
			(AimPoint - MuzzleLocation).GetSafeNormal();

		if (!TargetDirection.IsNearlyZero())
		{
			Direction = TargetDirection;
		}
	}

	if (Direction.IsNearlyZero())
	{
		Direction = FVector::ForwardVector;
	}

	return ApplySpread(Direction, AttackRow.ProjectileData.SpreadAngle);
}

FVector UGA_EnemyAttackMachineGun::ApplySpread(
	const FVector& Direction,
	float SpreadAngle) const
{
	const FVector NormalizedDirection = Direction.GetSafeNormal();

	if (SpreadAngle <= 0.0f)
	{
		return NormalizedDirection;
	}

	return FMath::VRandCone(
		NormalizedDirection,
		FMath::DegreesToRadians(SpreadAngle));
}

float UGA_EnemyAttackMachineGun::CalculateProjectileDamage(
	const FNSEnemyAttackRow& AttackRow) const
{
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return -1.0f;
	}

	const float SourceBaseDamage =
		SourceASC->GetNumericAttribute(UNSBaseAttributeSet::GetBaseDamageAttribute());

	return FMath::Max(SourceBaseDamage * AttackRow.DamageScale, 0.0f);
}

void UGA_EnemyAttackMachineGun::StartBurst()
{
	if (!CachedAttackRow || bBurstStarted)
	{
		CancelAttackAbility();
		return;
	}

	bBurstStarted = true;

	FireNextProjectile();

	if (!IsActive() || !CachedAttackRow)
	{
		return;
	}

	const int32 FireCount = FMath::Max(CachedAttackRow->ProjectileData.FireCount, 1);
	if (FiredCount >= FireCount)
	{
		return;
	}

	const float FireInterval = CachedAttackRow->ProjectileData.FireInterval;

	if (FireInterval <= 0.0f)
	{
		while (IsActive() && FiredCount < FireCount)
		{
			FireNextProjectile();
		}

		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			BurstTimerHandle,
			this,
			&ThisClass::FireNextProjectile,
			FireInterval,
			true);
	}
}

void UGA_EnemyAttackMachineGun::FireNextProjectile()
{
	if (!IsActive() || !CachedAttackRow)
	{
		ClearBurstTimer();
		return;
	}

	const int32 FireCount = FMath::Max(CachedAttackRow->ProjectileData.FireCount, 1);
	if (FiredCount >= FireCount)
	{
		ClearBurstTimer();
		FinishAttackAbility();
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	UNSProjectileManagerComponent* ProjectileManager = GetProjectileManager();
	if (!ProjectileManager)
	{
		ClearBurstTimer();
		CancelAttackAbility();
		return;
	}

	TArray<FTransform> MuzzleTransforms;
	GetCurrentMuzzleTransforms(MuzzleTransforms);

	if (MuzzleTransforms.IsEmpty())
	{
		ClearBurstTimer();
		CancelAttackAbility();
		return;
	}

	const FTransform& MuzzleTransform =
		MuzzleTransforms[NextMuzzleIndex % MuzzleTransforms.Num()];

	++NextMuzzleIndex;

	AActor* AttackActor = ResolveAttackActor();

	const FVector Direction = ResolveFireDirection(
		*CachedAttackRow,
		MuzzleTransform,
		AttackActor);

	FNSProjectileFireRequest Request;
	Request.StartLocation = MuzzleTransform.GetLocation();
	Request.Direction = Direction;
	Request.Speed = CachedAttackRow->ProjectileData.Speed;
	Request.MaxLifeTime = CachedAttackRow->ProjectileData.LifeTime;
	Request.Radius = CachedAttackRow->ProjectileData.Radius;
	Request.TraceChannel = ProjectileTraceChannel;
	Request.SourceActor = AvatarActor;
	Request.DamageEffectClass = DamageEffectClass;
	Request.Damage = CalculateProjectileDamage(*CachedAttackRow);

	ProjectileManager->FireProjectile(Request);
	PlayMachineGunFireSound(MuzzleTransform);
	
	if (UNSEnemyCosmeticComponent* CosmeticComponent =
	AvatarActor->FindComponentByClass<UNSEnemyCosmeticComponent>())
	{
		FNSCosmeticEventNetData EventData;
		EventData.EventTag = NSGameplayTags::Cosmetic_Enemy_TitanWalker_MachineGun_Fire;
		EventData.Phase = ENSCosmeticEventPhase::OneShot;
		EventData.Location = MuzzleTransform.GetLocation();
		EventData.Direction = Direction;
		EventData.Radius = CachedAttackRow->ProjectileData.Radius;

		CosmeticComponent->SendCosmeticEvent(EventData, false);
	}

	/*DrawDebugFire(
		*CachedAttackRow,
		Request.StartLocation,
		Direction);*/

	++FiredCount;

	if (FiredCount >= FireCount)
	{
		ClearBurstTimer();
		FinishAttackAbility();
	}
}

void UGA_EnemyAttackMachineGun::ClearBurstTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PreAimTimerHandle);
		World->GetTimerManager().ClearTimer(BurstTimerHandle);
	}
}

void UGA_EnemyAttackMachineGun::DrawDebugFire(
	const FNSEnemyAttackRow& AttackRow,
	const FVector& Start,
	const FVector& Direction) const
{
	if (!AttackRow.DebugData.bDrawDebug)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float DebugRange =
		FMath::Min(
			AttackRow.Condition.MaxRange,
			AttackRow.ProjectileData.Speed * AttackRow.ProjectileData.LifeTime);

	const FVector End = Start + Direction.GetSafeNormal() * DebugRange;

	DrawDebugLine(
		World,
		Start,
		End,
		FColor::Yellow,
		false,
		AttackRow.DebugData.DrawTime,
		0,
		2.0f);

	DrawDebugSphere(
		World,
		Start,
		AttackRow.ProjectileData.Radius,
		8,
		FColor::Orange,
		false,
		AttackRow.DebugData.DrawTime);
}

void UGA_EnemyAttackMachineGun::StartPreAimOrBurst()
{
	if (!CachedAttackRow)
	{
		CancelAttackAbility();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		StartBurst();
		return;
	}

	PreAimStartTime = World->GetTimeSeconds();
	TryStartBurstAfterPreAim();
}

void UGA_EnemyAttackMachineGun::TryStartBurstAfterPreAim()
{
	if (!IsActive() || !CachedAttackRow)
	{
		ClearBurstTimer();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		StartBurst();
		return;
	}

	const float ElapsedTime =
		FMath::Max(World->GetTimeSeconds() - PreAimStartTime, 0.0f);

	const float RequiredWarnTime =
		FMath::Max(CachedAttackRow->WarnTime, 0.0f);

	const float MaxWaitTime =
		FMath::Max(RequiredWarnTime, MaxPreAimWaitTime);

	const bool bWarnFinished = ElapsedTime >= RequiredWarnTime;
	const bool bAimReady = IsAimReadyForFire();
	const bool bTimedOut = ElapsedTime >= MaxWaitTime;

	if ((bWarnFinished && bAimReady) || bTimedOut)
	{
		World->GetTimerManager().ClearTimer(PreAimTimerHandle);
		StartBurst();
		return;
	}

	World->GetTimerManager().SetTimer(
		PreAimTimerHandle,
		this,
		&ThisClass::TryStartBurstAfterPreAim,
		FMath::Max(PreAimPollInterval, 0.01f),
		false);
}

bool UGA_EnemyAttackMachineGun::IsAimReadyForFire() const
{
	if (AimReadyToleranceDegrees <= 0.0f)
	{
		return true;
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(AvatarActor);
	const USkeletalMeshComponent* EnemyMesh =
		EnemyAgent ? EnemyAgent->GetEnemyMesh() : nullptr;

	const UNSTitanWalkerAnimInstance* TitanAnimInstance =
		EnemyMesh ? Cast<UNSTitanWalkerAnimInstance>(EnemyMesh->GetAnimInstance()) : nullptr;

	return !TitanAnimInstance ||
		TitanAnimInstance->IsAimAligned(AimReadyToleranceDegrees);
}

void UGA_EnemyAttackMachineGun::PlayMachineGunFireSound(const FTransform& MuzzleTransform) const
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer || MachineGunFireSoundID.IsNone())
	{
		return;
	}

	if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(this))
	{
		SoundSubsystem->PlaySoundAtLocation(MachineGunFireSoundID, MuzzleTransform.GetLocation());
	}
}
