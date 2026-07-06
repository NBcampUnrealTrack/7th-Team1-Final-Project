// Copyright 2026 One Team. All rights reserved.

#include "GA_EnemyAttackLaser.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "DrawDebugHelpers.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Components/PrimitiveComponent.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPartComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_EnemyAttackLaser::UGA_EnemyAttackLaser()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Enemy_TitanWalker_Laser);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(NSGameplayTags::State_Enemy_Combat);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}

void UGA_EnemyAttackLaser::ActivateAbility(
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

	if (CachedAttackRow->LaserMode != ENSBossLaserMode::Straight)
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_EnemyAttackLaser: 현재는 Straight LaserMode만 지원. AttackId=%s"),
		       *CachedAttackRow->AttackId.ToString());

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TArray<FNSLaserBeam> LaserBeams;
	GetCurrentLaserBeams(LaserBeams);

	if (LaserBeams.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_EnemyAttackLaser: 사용할 Laser TraceSocket/MuzzleSocket 없음. AttackId=%s"),
		       *CachedAttackRow->AttackId.ToString());

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartLaser();
}

void UGA_EnemyAttackLaser::InitializeAttack()
{
	CachedAttackRow = GetCurrentAttackRow();
}

void UGA_EnemyAttackLaser::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ClearLaserTimers();

	CachedAttackRow = nullptr;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

const FNSEnemyAttackRow* UGA_EnemyAttackLaser::GetCurrentAttackRow() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(AvatarActor);

	return EnemyAgent
		       ? EnemyAgent->GetCurrentAttackRow()
		       : nullptr;
}

UNSEnemyPartComponent* UGA_EnemyAttackLaser::GetEnemyPartComponent() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();

	return AvatarActor
		       ? AvatarActor->FindComponentByClass<UNSEnemyPartComponent>()
		       : nullptr;
}

void UGA_EnemyAttackLaser::GetCurrentLaserBeams(TArray<FNSLaserBeam>& OutBeams) const
{
	OutBeams.Reset();

	if (!CachedAttackRow)
	{
		return;
	}

	const UNSEnemyPartComponent* PartComponent = GetEnemyPartComponent();
	if (!PartComponent)
	{
		return;
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const FVector FallbackDirection = AvatarActor
		                                  ? AvatarActor->GetActorForwardVector()
		                                  : FVector::ForwardVector;

	const float Range = GetLaserRange(*CachedAttackRow);

	TArray<FNSEnemyPartTraceSegment> TraceSegments;
	PartComponent->GetTraceSegmentsByAttackId(
		CachedAttackRow->AttackId,
		Range,
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

		FNSLaserBeam Beam;
		Beam.Start = TraceSegment.Start;
		Beam.End = TraceSegment.Start + Direction * Range;
		OutBeams.Add(Beam);
	}

	if (!OutBeams.IsEmpty())
	{
		return;
	}

	TArray<FTransform> MuzzleTransforms;
	PartComponent->GetMuzzleTransformsByAttackId(
		CachedAttackRow->AttackId,
		MuzzleTransforms);

	for (const FTransform& MuzzleTransform : MuzzleTransforms)
	{
		FVector Direction =
			MuzzleTransform.GetRotation().GetForwardVector().GetSafeNormal();

		if (Direction.IsNearlyZero())
		{
			Direction = FallbackDirection.GetSafeNormal();
		}

		if (Direction.IsNearlyZero())
		{
			continue;
		}

		FNSLaserBeam Beam;
		Beam.Start = MuzzleTransform.GetLocation();
		Beam.End = Beam.Start + Direction * Range;
		OutBeams.Add(Beam);
	}
}

void UGA_EnemyAttackLaser::StartLaser()
{
	if (!CachedAttackRow)
	{
		CancelAttackAbility();
		return;
	}

	const float WarnTime = FMath::Max(CachedAttackRow->WarnTime, 0.0f);

	if (WarnTime <= 0.0f)
	{
		BeginLaserDamage();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			LaserStartTimerHandle,
			this,
			&ThisClass::BeginLaserDamage,
			WarnTime,
			false);
	}
}

void UGA_EnemyAttackLaser::BeginLaserDamage()
{
	if (!IsActive() || !CachedAttackRow)
	{
		return;
	}

	TickLaserDamage();

	if (!IsActive() || !CachedAttackRow)
	{
		return;
	}

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
			LaserTickTimerHandle,
			this,
			&ThisClass::TickLaserDamage,
			TickInterval,
			true);

		World->GetTimerManager().SetTimer(
			LaserEndTimerHandle,
			this,
			&ThisClass::CompleteLaser,
			Duration,
			false);
	}
}

void UGA_EnemyAttackLaser::TickLaserDamage()
{
	if (!IsActive() || !CachedAttackRow)
	{
		ClearLaserTimers();
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	TArray<FNSLaserBeam> LaserBeams;
	GetCurrentLaserBeams(LaserBeams);

	if (LaserBeams.IsEmpty())
	{
		ClearLaserTimers();
		CancelAttackAbility();
		return;
	}

	TSet<TObjectKey<AActor>> TargetsThisTick;

	for (const FNSLaserBeam& Beam : LaserBeams)
	{
		CollectTargetsForBeam(
			Beam,
			TargetsThisTick);

		DrawDebugLaserBeam(
			Beam,
			*CachedAttackRow);
	}

	for (const TObjectKey<AActor>& TargetKey : TargetsThisTick)
	{
		AActor* TargetActor = TargetKey.ResolveObjectPtr();
		if (IsValid(TargetActor))
		{
			ApplyLaserDamageToTarget(
				TargetActor,
				*CachedAttackRow);
		}
	}
}

void UGA_EnemyAttackLaser::CompleteLaser()
{
	FinishAttackAbility();
}

void UGA_EnemyAttackLaser::ClearLaserTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LaserStartTimerHandle);
		World->GetTimerManager().ClearTimer(LaserTickTimerHandle);
		World->GetTimerManager().ClearTimer(LaserEndTimerHandle);
	}
}

void UGA_EnemyAttackLaser::CollectTargetsForBeam(
	const FNSLaserBeam& Beam,
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

	const float Radius = GetLaserRadius(*CachedAttackRow);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyLaser), false);
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

	TArray<FHitResult> HitResults;
	const bool bHit = World->SweepMultiByChannel(
		HitResults,
		Beam.Start,
		Beam.End,
		FQuat::Identity,
		LaserTraceChannel,
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	if (!bHit)
	{
		return;
	}

	for (const FHitResult& HitResult : HitResults)
	{
		AActor* TargetActor = HitResult.GetActor();
		if (!IsValidDamageTarget(TargetActor))
		{
			continue;
		}

		OutTargets.Add(TObjectKey<AActor>(TargetActor));
	}
}

bool UGA_EnemyAttackLaser::IsValidDamageTarget(AActor* TargetActor) const
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

FVector UGA_EnemyAttackLaser::GetTargetCheckLocation(AActor* TargetActor) const
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

float UGA_EnemyAttackLaser::CalculateLaserDamage(
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

bool UGA_EnemyAttackLaser::ApplyLaserDamageToTarget(
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

	const float Damage = CalculateLaserDamage(AttackRow);
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

float UGA_EnemyAttackLaser::GetLaserRange(
	const FNSEnemyAttackRow& AttackRow) const
{
	if (AttackRow.AreaData.Range > 0.0f)
	{
		return AttackRow.AreaData.Range;
	}

	return FMath::Max(AttackRow.Condition.MaxRange, 0.0f);
}

float UGA_EnemyAttackLaser::GetLaserRadius(
	const FNSEnemyAttackRow& AttackRow) const
{
	return FMath::Max(AttackRow.AreaData.Radius, 1.0f);
}

void UGA_EnemyAttackLaser::DrawDebugLaserBeam(
	const FNSLaserBeam& Beam,
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

	const FVector Direction = (Beam.End - Beam.Start).GetSafeNormal();
	const float Distance = FVector::Dist(Beam.Start, Beam.End);
	const float Radius = GetLaserRadius(AttackRow);

	if (Direction.IsNearlyZero() || Distance <= 0.0f)
	{
		return;
	}

	const FVector Center = (Beam.Start + Beam.End) * 0.5f;
	const float HalfHeight = FMath::Max(Distance * 0.5f + Radius, Radius);
	const FQuat CapsuleRotation = FRotationMatrix::MakeFromZ(Direction).ToQuat();

	DrawDebugLine(
		World,
		Beam.Start,
		Beam.End,
		FColor::Red,
		false,
		AttackRow.DebugData.DrawTime,
		0,
		4.0f);

	DrawDebugCapsule(
		World,
		Center,
		HalfHeight,
		Radius,
		CapsuleRotation,
		FColor::Magenta,
		false,
		AttackRow.DebugData.DrawTime,
		0,
		1.5f);

	DrawDebugSphere(
		World,
		Beam.Start,
		FMath::Max(Radius, 12.0f),
		12,
		FColor::Cyan,
		false,
		AttackRow.DebugData.DrawTime);
}
