// Copyright 2026 One Team. All rights reserved.

#include "GA_EnemyAttackFlame.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "DrawDebugHelpers.h"
#include "GenericTeamAgentInterface.h"
#include "NiagaraComponent.h"
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

UGA_EnemyAttackFlame::UGA_EnemyAttackFlame()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Enemy_TitanWalker_Flame);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(NSGameplayTags::State_Enemy_Combat);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}

void UGA_EnemyAttackFlame::ActivateAbility(
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

	TArray<FNSFlameEmitter> Emitters;
	GetCurrentFlameEmitters(Emitters);

	if (Emitters.IsEmpty())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartFlame();
}

void UGA_EnemyAttackFlame::InitializeAttack()
{
	CachedAttackRow = GetCurrentAttackRow();
}

void UGA_EnemyAttackFlame::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	StopFlameCosmetics();
	ClearFlameTimers();

	CachedAttackRow = nullptr;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

const FNSEnemyAttackRow* UGA_EnemyAttackFlame::GetCurrentAttackRow() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(AvatarActor);

	return EnemyAgent
		       ? EnemyAgent->GetCurrentAttackRow()
		       : nullptr;
}

UNSEnemyPartComponent* UGA_EnemyAttackFlame::GetEnemyPartComponent() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();

	return AvatarActor
		       ? AvatarActor->FindComponentByClass<UNSEnemyPartComponent>()
		       : nullptr;
}

ANSBossAIController* UGA_EnemyAttackFlame::GetBossController() const
{
	const APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!AvatarPawn)
	{
		return nullptr;
	}

	return Cast<ANSBossAIController>(AvatarPawn->GetController());
}

void UGA_EnemyAttackFlame::GetCurrentFlameEmitters(
	TArray<FNSFlameEmitter>& OutEmitters) const
{
	OutEmitters.Reset();

	if (!CachedAttackRow)
	{
		return;
	}

	const UNSEnemyPartComponent* PartComponent = GetEnemyPartComponent();
	if (!PartComponent)
	{
		return;
	}

	TArray<FTransform> MuzzleTransforms;
	PartComponent->GetMuzzleTransformsByAttackId(
		CachedAttackRow->AttackId,
		MuzzleTransforms);

	AActor* AttackActor = ResolveAttackActor();

	for (const FTransform& MuzzleTransform : MuzzleTransforms)
	{
		FNSFlameEmitter Emitter;
		Emitter.Start = MuzzleTransform.GetLocation();
		Emitter.Direction = ResolveFlameDirection(
			*CachedAttackRow,
			MuzzleTransform,
			AttackActor);

		if (!Emitter.Direction.IsNearlyZero())
		{
			OutEmitters.Add(Emitter);
		}
	}

	if (!OutEmitters.IsEmpty())
	{
		return;
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const FVector FallbackDirection = AvatarActor
		                                  ? AvatarActor->GetActorForwardVector()
		                                  : FVector::ForwardVector;

	const float FallbackRange =
		CachedAttackRow->AreaData.Range > 0.0f
			? CachedAttackRow->AreaData.Range
			: CachedAttackRow->Condition.MaxRange;

	TArray<FNSEnemyPartTraceSegment> TraceSegments;
	PartComponent->GetTraceSegmentsByAttackId(
		CachedAttackRow->AttackId,
		FallbackRange,
		FallbackDirection,
		TraceSegments);

	for (const FNSEnemyPartTraceSegment& TraceSegment : TraceSegments)
	{
		const FVector Direction =
			(TraceSegment.End - TraceSegment.Start).GetSafeNormal();

		if (Direction.IsNearlyZero())
		{
			continue;
		}

		FNSFlameEmitter Emitter;
		Emitter.Start = TraceSegment.Start;
		Emitter.Direction = Direction;
		OutEmitters.Add(Emitter);
	}
}

void UGA_EnemyAttackFlame::StartFlame()
{
	if (!CachedAttackRow)
	{
		CancelAttackAbility();
		return;
	}

	StartFlameCosmetics();
	TickFlameDamage();

	const float Duration = FMath::Max(CachedAttackRow->SustainData.Duration, 0.0f);
	if (Duration <= 0.0f)
	{
		FinishAttackAbility();
		return;
	}

	const float TickInterval =
		FMath::Max(CachedAttackRow->SustainData.TickInterval, 0.01f);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FlameTickTimerHandle,
			this,
			&ThisClass::TickFlameDamage,
			TickInterval,
			true);

		World->GetTimerManager().SetTimer(
			FlameEndTimerHandle,
			this,
			&ThisClass::CompleteFlame,
			Duration,
			false);
	}
}

void UGA_EnemyAttackFlame::TickFlameDamage()
{
	if (!IsActive() || !CachedAttackRow)
	{
		ClearFlameTimers();
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	TArray<FNSFlameEmitter> Emitters;
	GetCurrentFlameEmitters(Emitters);

	if (Emitters.IsEmpty())
	{
		ClearFlameTimers();
		CancelAttackAbility();
		return;
	}

	PlayFlameVFX(Emitters);

	TSet<TObjectKey<AActor>> TargetsThisTick;

	for (const FNSFlameEmitter& Emitter : Emitters)
	{
		CollectTargetsForEmitter(
			Emitter,
			TargetsThisTick);

		DrawDebugFlameCone(
			Emitter,
			*CachedAttackRow);
	}

	for (const TObjectKey<AActor>& TargetKey : TargetsThisTick)
	{
		AActor* TargetActor = TargetKey.ResolveObjectPtr();
		if (IsValid(TargetActor))
		{
			ApplyFlameDamageToTarget(
				TargetActor,
				*CachedAttackRow);
		}
	}
}

void UGA_EnemyAttackFlame::CompleteFlame()
{
	FinishAttackAbility();
}

void UGA_EnemyAttackFlame::ClearFlameTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FlameTickTimerHandle);
		World->GetTimerManager().ClearTimer(FlameEndTimerHandle);
	}
}

void UGA_EnemyAttackFlame::CollectTargetsForEmitter(
	const FNSFlameEmitter& Emitter,
	TSet<TObjectKey<AActor>>& OutTargets) const
{
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

	const float Range =
		CachedAttackRow->AreaData.Range > 0.0f
			? CachedAttackRow->AreaData.Range
			: CachedAttackRow->Condition.MaxRange;

	const float QueryRadius =
		FMath::Max(Range + CachedAttackRow->AreaData.Radius, 1.0f);

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
		Emitter.Start,
		FQuat::Identity,
		FlameTraceChannel,
		FCollisionShape::MakeSphere(QueryRadius),
		QueryParams);

	if (!bHasOverlap)
	{
		return;
	}

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!IsValidDamageTarget(TargetActor))
		{
			continue;
		}

		const FVector TargetLocation = GetTargetCheckLocation(TargetActor);

		if (!IsLocationInsideCone(
			Emitter,
			TargetLocation,
			*CachedAttackRow))
		{
			continue;
		}

		OutTargets.Add(TObjectKey<AActor>(TargetActor));
	}
}

bool UGA_EnemyAttackFlame::IsLocationInsideCone(
	const FNSFlameEmitter& Emitter,
	const FVector& TargetLocation,
	const FNSEnemyAttackRow& AttackRow) const
{
	const FVector Direction = Emitter.Direction.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return false;
	}

	const FVector ToTarget = TargetLocation - Emitter.Start;
	const float DistanceSquared = ToTarget.SizeSquared();

	const float Range =
		AttackRow.AreaData.Range > 0.0f
			? AttackRow.AreaData.Range
			: AttackRow.Condition.MaxRange;

	const float Radius = FMath::Max(AttackRow.AreaData.Radius, 0.0f);

	if (DistanceSquared <= FMath::Square(Radius))
	{
		return true;
	}

	const float ForwardDistance = FVector::DotProduct(ToTarget, Direction);
	if (ForwardDistance < 0.0f || ForwardDistance > Range + Radius)
	{
		return false;
	}

	const float LateralDistanceSquared =
		FMath::Max(
			DistanceSquared - FMath::Square(ForwardDistance),
			0.0f);

	const float ConeHalfAngleRadians =
		FMath::DegreesToRadians(
			FMath::Clamp(AttackRow.AreaData.ConeHalfAngle, 0.0f, 89.0f));

	const float ConeRadiusAtDistance =
		FMath::Tan(ConeHalfAngleRadians) * ForwardDistance + Radius;

	return LateralDistanceSquared <= FMath::Square(ConeRadiusAtDistance);
}

bool UGA_EnemyAttackFlame::IsValidDamageTarget(AActor* TargetActor) const
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

FVector UGA_EnemyAttackFlame::GetTargetCheckLocation(AActor* TargetActor) const
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

float UGA_EnemyAttackFlame::CalculateFlameDamage(
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

bool UGA_EnemyAttackFlame::ApplyFlameDamageToTarget(
	AActor* TargetActor,
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

	FGameplayEffectContextHandle EffectContext =
		SourceASC->MakeEffectContext();

	EffectContext.AddSourceObject(SourceActor);

	FGameplayEffectSpecHandle SpecHandle =
		SourceASC->MakeOutgoingSpec(
			DamageEffectClass,
			1.0f,
			EffectContext);

	if (!SpecHandle.IsValid())
	{
		return false;
	}

	const float Damage = CalculateFlameDamage(AttackRow);
	if (Damage >= 0.0f)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(
			NSGameplayTags::Effect_Damage_Base,
			Damage);
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(
		*SpecHandle.Data.Get(),
		TargetASC);

	if (GameplayCueTag.IsValid())
	{
		FGameplayCueParameters CueParameters;
		CueParameters.Location = GetTargetCheckLocation(TargetActor);
		CueParameters.Normal =
			(CueParameters.Location - SourceActor->GetActorLocation()).GetSafeNormal();

		SourceASC->ExecuteGameplayCue(
			GameplayCueTag,
			CueParameters);
	}

	return true;
}

AActor* UGA_EnemyAttackFlame::ResolveAttackActor() const
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

FVector UGA_EnemyAttackFlame::ResolveAimPoint(
	const FNSEnemyAttackRow& AttackRow,
	const FTransform& MuzzleTransform,
	const AActor* AttackActor) const
{
	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	const FVector MuzzleForward = MuzzleTransform.GetRotation().GetForwardVector();

	const float Range =
		AttackRow.AreaData.Range > 0.0f
			? AttackRow.AreaData.Range
			: AttackRow.Condition.MaxRange;

	if (AttackRow.AimMode == ENSEnemyAimMode::Forward || !IsValid(AttackActor))
	{
		return MuzzleLocation + MuzzleForward * Range;
	}

	FVector TargetLocation = AttackActor->GetActorLocation();

	if (const UPrimitiveComponent* PrimitiveComponent =
		Cast<UPrimitiveComponent>(AttackActor->GetRootComponent()))
	{
		TargetLocation = PrimitiveComponent->Bounds.Origin;
	}

	if (AttackRow.AimMode == ENSEnemyAimMode::Ground)
	{
		TargetLocation.Z = AttackActor->GetActorLocation().Z;
	}

	return TargetLocation;
}

FVector UGA_EnemyAttackFlame::ResolveFlameDirection(
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

	return Direction;
}

void UGA_EnemyAttackFlame::DrawDebugFlameCone(
	const FNSFlameEmitter& Emitter,
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

	const FVector Direction = Emitter.Direction.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const float Range =
		AttackRow.AreaData.Range > 0.0f
			? AttackRow.AreaData.Range
			: AttackRow.Condition.MaxRange;

	const float ConeHalfAngleRadians =
		FMath::DegreesToRadians(
			FMath::Clamp(AttackRow.AreaData.ConeHalfAngle, 0.0f, 89.0f));

	DrawDebugCone(
		World,
		Emitter.Start,
		Direction,
		Range,
		ConeHalfAngleRadians,
		ConeHalfAngleRadians,
		24,
		FColor::Orange,
		false,
		AttackRow.DebugData.DrawTime,
		0,
		2.0f);

	DrawDebugSphere(
		World,
		Emitter.Start,
		FMath::Max(AttackRow.AreaData.Radius, 8.0f),
		12,
		FColor::Red,
		false,
		AttackRow.DebugData.DrawTime);
}

void UGA_EnemyAttackFlame::StartFlameCosmetics()
{
	UWorld* World = GetWorld();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!World || World->GetNetMode() == NM_DedicatedServer || !IsValid(AvatarActor))
	{
		return;
	}

	if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(this))
	{
		ActiveFlameAudioComponent =
			SoundSubsystem->PlaySoundAttached(FlameSoundID, AvatarActor->GetRootComponent());
	}
}

void UGA_EnemyAttackFlame::PlayFlameVFX(const TArray<FNSFlameEmitter>& Emitters) const
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer || !CachedAttackRow || FlameVFXID.IsNone())
	{
		return;
	}

	UNSVFXSubsystem* VFXSubsystem = UNSVFXSubsystem::Get(this);
	if (!VFXSubsystem)
	{
		return;
	}

	const float Range = GetFlameRange(*CachedAttackRow);
	const float Scale = FMath::Max(Range / FlameVFXBaseRange, 0.1f);

	for (const FNSFlameEmitter& Emitter : Emitters)
	{
		UNiagaraComponent* VFX = VFXSubsystem->PlayVFXAtLocation(
			FlameVFXID,
			Emitter.Start,
			Emitter.Direction.Rotation(),
			Scale);

		if (VFX)
		{
			VFX->SetVariableFloat(FlameRangeParameterName, Range);
			VFX->SetVariableFloat(FlameRadiusParameterName, CachedAttackRow->AreaData.Radius);
		}
	}
}

void UGA_EnemyAttackFlame::StopFlameCosmetics()
{
	if (ActiveFlameAudioComponent)
	{
		if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(this))
		{
			SoundSubsystem->StopSound(ActiveFlameAudioComponent, 0.15f);
		}
		ActiveFlameAudioComponent = nullptr;
	}
}

float UGA_EnemyAttackFlame::GetFlameRange(const FNSEnemyAttackRow& AttackRow) const
{
	return AttackRow.AreaData.Range > 0.0f
		       ? AttackRow.AreaData.Range
		       : AttackRow.Condition.MaxRange;
}
