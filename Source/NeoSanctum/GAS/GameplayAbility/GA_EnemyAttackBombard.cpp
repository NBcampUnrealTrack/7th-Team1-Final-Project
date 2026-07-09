// Copyright 2026 One Team. All rights reserved.

#include "GA_EnemyAttackBombard.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "DrawDebugHelpers.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "NeoSanctum/AI/Enemy/Controller/NSBossAIController.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPartComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSoundSubsystem.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSVFXSubsystem.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "Materials/MaterialInterface.h"
#include "NeoSanctum/Combat/Cosmetic/NSEnemyCosmeticComponent.h"
#include "NeoSanctum/Combat/Warning/NSAreaWarningPlaneActor.h"

UGA_EnemyAttackBombard::UGA_EnemyAttackBombard()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Enemy_TitanWalker_Bombard);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(NSGameplayTags::State_Enemy_Combat);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}

void UGA_EnemyAttackBombard::InitializeAttack()
{
	CachedAttackRow = GetCurrentAttackRow();

	bVolleyStarted = false;
	bMontageCompleted = false;
	FiredShotCount = 0;
	PendingImpactCount = 0;

	ShotTimerHandles.Reset();
	ImpactTimerHandles.Reset();
}

void UGA_EnemyAttackBombard::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ClearBombardTimers();

	CachedAttackRow = nullptr;
	bVolleyStarted = false;
	bMontageCompleted = false;
	FiredShotCount = 0;
	PendingImpactCount = 0;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UGA_EnemyAttackBombard::HandleAttackEvent(const FGameplayEventData& Payload)
{
	StartBombardVolley();
}

void UGA_EnemyAttackBombard::HandleAttackMontageCompleted()
{
	bMontageCompleted = true;

	if (!bVolleyStarted)
	{
		FinishAttackAbility();
		return;
	}

	TryFinishBombardAbility();
}

const FNSEnemyAttackRow* UGA_EnemyAttackBombard::GetCurrentAttackRow() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(AvatarActor);

	return EnemyAgent
		       ? EnemyAgent->GetCurrentAttackRow()
		       : nullptr;
}

UNSEnemyPartComponent* UGA_EnemyAttackBombard::GetEnemyPartComponent() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();

	return AvatarActor
		       ? AvatarActor->FindComponentByClass<UNSEnemyPartComponent>()
		       : nullptr;
}

ANSBossAIController* UGA_EnemyAttackBombard::GetBossController() const
{
	const APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!AvatarPawn)
	{
		return nullptr;
	}

	return Cast<ANSBossAIController>(AvatarPawn->GetController());
}

void UGA_EnemyAttackBombard::StartBombardVolley()
{
	if (!IsActive() || !CachedAttackRow || bVolleyStarted)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	bVolleyStarted = true;

	const int32 ShotCount = FMath::Max(CachedAttackRow->BombardData.ShotCount, 1);
	const float ShotInterval = FMath::Max(CachedAttackRow->BombardData.ShotInterval, 0.0f);

	UWorld* World = GetWorld();
	if (!World)
	{
		CancelAttackAbility();
		return;
	}

	for (int32 ShotIndex = 0; ShotIndex < ShotCount; ++ShotIndex)
	{
		const float Delay = ShotInterval * ShotIndex;

		if (Delay <= 0.0f)
		{
			FireBombardShot(ShotIndex);
			continue;
		}

		FTimerDelegate ShotDelegate;
		ShotDelegate.BindUObject(
			this,
			&ThisClass::FireBombardShot,
			ShotIndex);

		FTimerHandle ShotTimerHandle;
		World->GetTimerManager().SetTimer(
			ShotTimerHandle,
			ShotDelegate,
			Delay,
			false);

		ShotTimerHandles.Add(ShotTimerHandle);
	}
}

void UGA_EnemyAttackBombard::FireBombardShot(int32 ShotIndex)
{
	if (!IsActive() || !CachedAttackRow)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	AActor* TargetActor = ResolveBombardTarget(ShotIndex);
	const FVector ImpactLocation = ResolveImpactLocation(TargetActor);
	const FVector MuzzleLocation = ResolveMuzzleLocation(ShotIndex);

	SendBombardLaunchCosmeticEvent(MuzzleLocation, ImpactLocation);
	SendBombardWarningCosmeticEvent(ImpactLocation);

	/*DrawDebugBombardWarning(
		MuzzleLocation,
		ImpactLocation,
		*CachedAttackRow);*/

	++FiredShotCount;
	++PendingImpactCount;

	const float ImpactDelay = FMath::Max(CachedAttackRow->BombardData.ImpactDelay, 0.0f);

	if (ImpactDelay <= 0.0f)
	{
		ApplyBombardImpact(ImpactLocation);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		FTimerDelegate ImpactDelegate;
		ImpactDelegate.BindUObject(
			this,
			&ThisClass::ApplyBombardImpact,
			ImpactLocation);

		FTimerHandle ImpactTimerHandle;
		World->GetTimerManager().SetTimer(
			ImpactTimerHandle,
			ImpactDelegate,
			ImpactDelay,
			false);

		ImpactTimerHandles.Add(ImpactTimerHandle);
		return;
	}

	PendingImpactCount = FMath::Max(PendingImpactCount - 1, 0);
	TryFinishBombardAbility();
}

void UGA_EnemyAttackBombard::ApplyBombardImpact(FVector ImpactLocation)
{
	if (!CachedAttackRow)
	{
		PendingImpactCount = FMath::Max(PendingImpactCount - 1, 0);
		TryFinishBombardAbility();
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		PendingImpactCount = FMath::Max(PendingImpactCount - 1, 0);
		TryFinishBombardAbility();
		return;
	}

	/*DrawDebugBombardImpact(
		ImpactLocation,
		*CachedAttackRow);*/

	SendBombardImpactCosmeticEvent(ImpactLocation);

	if (UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (GameplayCueTag.IsValid())
		{
			FGameplayCueParameters CueParameters;
			CueParameters.Location = ImpactLocation;
			CueParameters.Normal = FVector::UpVector;
			SourceASC->ExecuteGameplayCue(GameplayCueTag, CueParameters);
		}
	}

	TSet<TObjectKey<AActor>> Targets;
	CollectTargetsAtImpact(
		ImpactLocation,
		Targets);

	for (const TObjectKey<AActor>& TargetKey : Targets)
	{
		AActor* TargetActor = TargetKey.ResolveObjectPtr();
		if (IsValid(TargetActor))
		{
			ApplyBombardDamageToTarget(
				TargetActor,
				ImpactLocation,
				*CachedAttackRow);
		}
	}

	PendingImpactCount = FMath::Max(PendingImpactCount - 1, 0);
	TryFinishBombardAbility();
}

AActor* UGA_EnemyAttackBombard::ResolveBombardTarget(int32 ShotIndex) const
{
	ANSBossAIController* BossController = GetBossController();
	if (!BossController)
	{
		return nullptr;
	}

	TArray<AActor*> AttackTargets;
	BossController->GetCurrentAttackTargets(AttackTargets);

	AttackTargets.RemoveAll(
		[](AActor* TargetActor)
		{
			return !IsValid(TargetActor);
		});

	if (!AttackTargets.IsEmpty())
	{
		return AttackTargets[ShotIndex % AttackTargets.Num()];
	}

	if (AActor* AttackActor = BossController->GetCurrentAttackActor())
	{
		return AttackActor;
	}

	return BossController->GetCurrentTargetActor();
}

FVector UGA_EnemyAttackBombard::ResolveImpactLocation(AActor* TargetActor) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();

	FVector BaseLocation = IsValid(TargetActor)
		                       ? GetTargetCheckLocation(TargetActor)
		                       : FVector::ZeroVector;

	if (BaseLocation.IsNearlyZero() && IsValid(AvatarActor) && CachedAttackRow)
	{
		BaseLocation =
			AvatarActor->GetActorLocation() +
			AvatarActor->GetActorForwardVector() * CachedAttackRow->Condition.MaxRange;
	}

	const float SpreadRadius = CachedAttackRow
		                           ? FMath::Max(CachedAttackRow->BombardData.SpreadRadius, 0.0f)
		                           : 0.0f;

	if (SpreadRadius > 0.0f)
	{
		const FVector2D Offset = FMath::RandPointInCircle(SpreadRadius);
		BaseLocation += FVector(Offset.X, Offset.Y, 0.0f);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return BaseLocation;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);

	if (IsValid(TargetActor))
	{
		QueryParams.AddIgnoredActor(TargetActor);
	}

	if (const UNSEnemyPartComponent* PartComponent = GetEnemyPartComponent())
	{
		TArray<AActor*> IgnoredPartActors;

		if (CachedAttackRow)
		{
			PartComponent->GetSpawnedPartActorsByAttackId(
				CachedAttackRow->AttackId,
				IgnoredPartActors);
		}

		for (AActor* IgnoredActor : IgnoredPartActors)
		{
			if (IsValid(IgnoredActor))
			{
				QueryParams.AddIgnoredActor(IgnoredActor);
			}
		}
	}

	const FVector TraceStart = BaseLocation + FVector::UpVector * GroundTraceHeight;
	const FVector TraceEnd = BaseLocation - FVector::UpVector * GroundTraceDepth;

	FHitResult HitResult;
	const bool bHitGround = World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ImpactTraceChannel,
		QueryParams);

	return bHitGround
		       ? HitResult.ImpactPoint
		       : BaseLocation;
}

FVector UGA_EnemyAttackBombard::ResolveMuzzleLocation(int32 ShotIndex) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (CachedAttackRow)
	{
		if (const UNSEnemyPartComponent* PartComponent = GetEnemyPartComponent())
		{
			TArray<FTransform> MuzzleTransforms;
			PartComponent->GetMuzzleTransformsByAttackId(
				CachedAttackRow->AttackId,
				MuzzleTransforms);

			if (!MuzzleTransforms.IsEmpty())
			{
				return MuzzleTransforms[ShotIndex % MuzzleTransforms.Num()].GetLocation();
			}
		}
	}

	return IsValid(AvatarActor)
		       ? AvatarActor->GetActorLocation() + FVector::UpVector * 300.0f
		       : FVector::ZeroVector;
}

FVector UGA_EnemyAttackBombard::GetTargetCheckLocation(AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return FVector::ZeroVector;
	}

	if (const UPrimitiveComponent* PrimitiveComponent =
		Cast<UPrimitiveComponent>(TargetActor->GetRootComponent()))
	{
		return PrimitiveComponent->Bounds.Origin;
	}

	return TargetActor->GetActorLocation();
}

void UGA_EnemyAttackBombard::CollectTargetsAtImpact(
	const FVector& ImpactLocation,
	TSet<TObjectKey<AActor>>& OutTargets) const
{
	OutTargets.Reset();

	if (!CachedAttackRow)
	{
		return;
	}

	UWorld* World = GetWorld();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (!World || !IsValid(AvatarActor))
	{
		return;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);

	if (const UNSEnemyPartComponent* PartComponent = GetEnemyPartComponent())
	{
		TArray<AActor*> IgnoredPartActors;
		PartComponent->GetSpawnedPartActorsByAttackId(
			CachedAttackRow->AttackId,
			IgnoredPartActors);

		for (AActor* IgnoredActor : IgnoredPartActors)
		{
			if (IsValid(IgnoredActor))
			{
				QueryParams.AddIgnoredActor(IgnoredActor);
			}
		}
	}

	TArray<FOverlapResult> Overlaps;
	const bool bHasOverlap = World->OverlapMultiByChannel(
		Overlaps,
		ImpactLocation,
		FQuat::Identity,
		ImpactTraceChannel,
		FCollisionShape::MakeSphere(GetImpactRadius(*CachedAttackRow)),
		QueryParams);

	if (!bHasOverlap)
	{
		return;
	}

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (IsValidDamageTarget(TargetActor))
		{
			OutTargets.Add(TObjectKey<AActor>(TargetActor));
		}
	}
}

bool UGA_EnemyAttackBombard::IsValidDamageTarget(AActor* TargetActor) const
{
	AActor* SourceActor = GetAvatarActorFromActorInfo();

	if (!IsValid(SourceActor) ||
		!IsValid(TargetActor) ||
		TargetActor == SourceActor)
	{
		return false;
	}

	const IGenericTeamAgentInterface* SourceTeam =
		Cast<IGenericTeamAgentInterface>(SourceActor);

	const IGenericTeamAgentInterface* TargetTeam =
		Cast<IGenericTeamAgentInterface>(TargetActor);

	if (SourceTeam &&
		TargetTeam &&
		SourceTeam->GetGenericTeamId() == TargetTeam->GetGenericTeamId())
	{
		return false;
	}

	const IAbilitySystemInterface* TargetASI =
		Cast<IAbilitySystemInterface>(TargetActor);

	UAbilitySystemComponent* TargetASC =
		TargetASI ? TargetASI->GetAbilitySystemComponent() : nullptr;

	if (!IsValid(TargetASC))
	{
		return false;
	}

	for (UAttributeSet* AttributeSet : TargetASC->GetSpawnedAttributes())
	{
		if (!AttributeSet)
		{
			continue;
		}

		if (FProperty* HealthProperty =
			AttributeSet->GetClass()->FindPropertyByName(TEXT("Health")))
		{
			const FGameplayAttribute HealthAttribute(HealthProperty);
			return TargetASC->GetNumericAttribute(HealthAttribute) > 0.0f;
		}
	}

	return false;
}

float UGA_EnemyAttackBombard::CalculateBombardDamage(
	const FNSEnemyAttackRow& AttackRow) const
{
	const UAbilitySystemComponent* SourceASC =
		GetAbilitySystemComponentFromActorInfo();

	if (!SourceASC)
	{
		return -1.0f;
	}

	const float SourceBaseDamage =
		SourceASC->GetNumericAttribute(
			UNSBaseAttributeSet::GetBaseDamageAttribute());

	return FMath::Max(
		SourceBaseDamage * AttackRow.DamageScale,
		0.0f);
}

bool UGA_EnemyAttackBombard::ApplyBombardDamageToTarget(
	AActor* TargetActor,
	const FVector& ImpactLocation,
	const FNSEnemyAttackRow& AttackRow)
{
	AActor* SourceActor = GetAvatarActorFromActorInfo();

	const IAbilitySystemInterface* TargetASI =
		Cast<IAbilitySystemInterface>(TargetActor);

	UAbilitySystemComponent* SourceASC =
		GetAbilitySystemComponentFromActorInfo();

	UAbilitySystemComponent* TargetASC =
		TargetASI ? TargetASI->GetAbilitySystemComponent() : nullptr;

	if (!IsValid(SourceActor) ||
		!IsValid(SourceASC) ||
		!IsValid(TargetASC) ||
		!DamageEffectClass)
	{
		return false;
	}

	FHitResult HitResult;
	HitResult.Location = ImpactLocation;
	HitResult.ImpactPoint = ImpactLocation;
	HitResult.TraceStart = SourceActor->GetActorLocation();
	HitResult.TraceEnd = ImpactLocation;

	FGameplayEffectContextHandle EffectContext =
		SourceASC->MakeEffectContext();

	EffectContext.AddSourceObject(SourceActor);
	EffectContext.AddHitResult(HitResult);

	FGameplayEffectSpecHandle SpecHandle =
		SourceASC->MakeOutgoingSpec(
			DamageEffectClass,
			1.0f,
			EffectContext);

	if (!SpecHandle.IsValid())
	{
		return false;
	}

	const float Damage = CalculateBombardDamage(AttackRow);
	if (Damage >= 0.0f)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(
			NSGameplayTags::Effect_Damage_Base,
			Damage);
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(
		*SpecHandle.Data.Get(),
		TargetASC);

	return true;
}

float UGA_EnemyAttackBombard::GetImpactRadius(
	const FNSEnemyAttackRow& AttackRow) const
{
	if (AttackRow.BombardData.ImpactRadius > 0.0f)
	{
		return AttackRow.BombardData.ImpactRadius;
	}

	return FMath::Max(AttackRow.AreaData.Radius, 1.0f);
}

void UGA_EnemyAttackBombard::DrawDebugBombardWarning(
	const FVector& MuzzleLocation,
	const FVector& ImpactLocation,
	const FNSEnemyAttackRow& AttackRow) const
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

	const float DrawTime =
		FMath::Max(
			AttackRow.DebugData.DrawTime,
			AttackRow.BombardData.ImpactDelay);

	const float ImpactRadius = GetImpactRadius(AttackRow);

	DrawDebugLine(
		World,
		MuzzleLocation,
		ImpactLocation,
		FColor::Cyan,
		false,
		DrawTime,
		0,
		2.0f);

	DrawDebugCylinder(
		World,
		ImpactLocation - FVector::UpVector * 5.0f,
		ImpactLocation + FVector::UpVector * 5.0f,
		ImpactRadius,
		32,
		FColor::Yellow,
		false,
		DrawTime,
		0,
		2.0f);

	DrawDebugSphere(
		World,
		ImpactLocation,
		24.0f,
		8,
		FColor::Yellow,
		false,
		DrawTime);
}

void UGA_EnemyAttackBombard::DrawDebugBombardImpact(
	const FVector& ImpactLocation,
	const FNSEnemyAttackRow& AttackRow) const
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

	const float ImpactRadius = GetImpactRadius(AttackRow);

	DrawDebugSphere(
		World,
		ImpactLocation,
		ImpactRadius,
		24,
		FColor::Red,
		false,
		AttackRow.DebugData.DrawTime,
		0,
		2.0f);

	DrawDebugPoint(
		World,
		ImpactLocation,
		16.0f,
		FColor::Red,
		false,
		AttackRow.DebugData.DrawTime);
}

void UGA_EnemyAttackBombard::ClearBombardTimers()
{
	if (UWorld* World = GetWorld())
	{
		for (FTimerHandle& ShotTimerHandle : ShotTimerHandles)
		{
			World->GetTimerManager().ClearTimer(ShotTimerHandle);
		}

		for (FTimerHandle& ImpactTimerHandle : ImpactTimerHandles)
		{
			World->GetTimerManager().ClearTimer(ImpactTimerHandle);
		}
	}

	ShotTimerHandles.Reset();
	ImpactTimerHandles.Reset();
}

void UGA_EnemyAttackBombard::TryFinishBombardAbility()
{
	if (!bMontageCompleted || !CachedAttackRow)
	{
		return;
	}

	const int32 ShotCount = FMath::Max(CachedAttackRow->BombardData.ShotCount, 1);

	if (FiredShotCount < ShotCount)
	{
		return;
	}

	if (PendingImpactCount > 0)
	{
		return;
	}

	FinishAttackAbility();
}

void UGA_EnemyAttackBombard::PrepareForAttackMontage()
{
	Super::PrepareForAttackMontage();

	SendBombardPrepareCosmeticEvent();
}

void UGA_EnemyAttackBombard::SendBombardPrepareCosmeticEvent() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	FNSCosmeticEventNetData EventData;
	EventData.EventTag = NSGameplayTags::Cosmetic_Enemy_TitanWalker_Bombard_Prepare;
	EventData.Phase = ENSCosmeticEventPhase::OneShot;
	EventData.Location = AvatarActor->GetActorLocation();
	EventData.Direction = AvatarActor->GetActorForwardVector();
	EventData.Duration = CachedAttackRow ? CachedAttackRow->WarnTime : 0.0f;

	SendBombardCosmeticEvent(EventData, true);
}

void UGA_EnemyAttackBombard::SendBombardLaunchCosmeticEvent(
	const FVector& MuzzleLocation,
	const FVector& ImpactLocation) const
{
	const FVector Direction = (ImpactLocation - MuzzleLocation).GetSafeNormal();

	FNSCosmeticEventNetData EventData;
	EventData.EventTag = NSGameplayTags::Cosmetic_Enemy_TitanWalker_Bombard_Launch;
	EventData.Phase = ENSCosmeticEventPhase::OneShot;
	EventData.Location = MuzzleLocation;
	EventData.EndLocation = ImpactLocation;
	EventData.Direction = Direction;
	EventData.Range = FVector::Dist(MuzzleLocation, ImpactLocation);
	EventData.Duration = CachedAttackRow ? CachedAttackRow->BombardData.ImpactDelay : 0.0f;

	SendBombardCosmeticEvent(EventData, true);
}

void UGA_EnemyAttackBombard::SendBombardWarningCosmeticEvent(
	const FVector& ImpactLocation) const
{
	if (!CachedAttackRow)
	{
		return;
	}

	FNSCosmeticEventNetData EventData;
	EventData.EventTag = NSGameplayTags::Cosmetic_Enemy_TitanWalker_Bombard_Warning;
	EventData.Phase = ENSCosmeticEventPhase::OneShot;
	EventData.Location = ImpactLocation;
	EventData.Direction = FVector::UpVector;
	EventData.Radius = GetImpactRadius(*CachedAttackRow);
	EventData.Duration = FMath::Max(CachedAttackRow->BombardData.ImpactDelay, 0.0f);

	SendBombardCosmeticEvent(EventData, true);
}

void UGA_EnemyAttackBombard::SendBombardImpactCosmeticEvent(
	const FVector& ImpactLocation) const
{
	if (!CachedAttackRow)
	{
		return;
	}

	FNSCosmeticEventNetData EventData;
	EventData.EventTag = NSGameplayTags::Cosmetic_Enemy_TitanWalker_Bombard_Impact;
	EventData.Phase = ENSCosmeticEventPhase::OneShot;
	EventData.Location = ImpactLocation;
	EventData.Direction = FVector::UpVector;
	EventData.Radius = GetImpactRadius(*CachedAttackRow);

	SendBombardCosmeticEvent(EventData, true);
}

void UGA_EnemyAttackBombard::SendBombardCosmeticEvent(
	const FNSCosmeticEventNetData& EventData,
	bool bReliable) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		return;
	}

	UNSEnemyCosmeticComponent* CosmeticComponent =
		AvatarActor->FindComponentByClass<UNSEnemyCosmeticComponent>();

	if (CosmeticComponent)
	{
		CosmeticComponent->SendCosmeticEvent(EventData, bReliable);
	}
}
