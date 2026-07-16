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
#include "NeoSanctum/Combat/Cosmetic/NSEnemyCosmeticComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "NeoSanctum/AI/Enemy/Controller/NSBossAIController.h"
#include "NeoSanctum/Combat/Component/NSEnemyCombatComponent.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"

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
	LaserCosmeticInstanceId = INDEX_NONE;
	LockedLaserAimPoint = FVector::ZeroVector;
	bHasLockedLaserAimPoint = false;
	LockedLaserYaw = 0.0f;
	LockedLaserInitialPitch = 0.0f;
	LockedLaserBeamStartTime = 0.0f;
}

void UGA_EnemyAttackLaser::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	SendLaserStopCosmeticEvent();
	ClearLaserTimers();
	ClearLockedLaserAimPoint();

	LaserCosmeticInstanceId = INDEX_NONE;
	CachedAttackRow = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
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

	const float Range = GetLaserRange(*CachedAttackRow);
	AActor* AttackActor = bHasLockedLaserAimPoint ? nullptr : ResolveAttackActor();

	if (const UNSEnemyPartComponent* PartComponent = GetEnemyPartComponent())
	{
		TArray<FTransform> MuzzleTransforms;
		PartComponent->GetMuzzleTransformsByAttackId(CachedAttackRow->AttackId, MuzzleTransforms);

		for (const FTransform& MuzzleTransform : MuzzleTransforms)
		{
			const FVector Start = MuzzleTransform.GetLocation();
			const FVector Direction = bHasLockedLaserAimPoint
				                          ? ResolveLockedLaserDirection(*CachedAttackRow, MuzzleTransform)
				                          : ResolveLaserDirection(*CachedAttackRow, MuzzleTransform, AttackActor);

			if (Direction.IsNearlyZero())
			{
				continue;
			}

			FNSLaserBeam Beam;
			Beam.Start = Start;
			Beam.End = Start + Direction * Range;
			OutBeams.Add(Beam);
		}

		if (!OutBeams.IsEmpty())
		{
			return;
		}

		const AActor* AvatarActorForFallback = GetAvatarActorFromActorInfo();
		FVector FallbackDirection = AvatarActorForFallback
			                            ? AvatarActorForFallback->GetActorForwardVector().GetSafeNormal()
			                            : FVector::ForwardVector;

		if (FallbackDirection.IsNearlyZero())
		{
			FallbackDirection = FVector::ForwardVector;
		}

		TArray<FNSEnemyPartTraceSegment> TraceSegments;
		PartComponent->GetTraceSegmentsByAttackId(
			CachedAttackRow->AttackId,
			Range,
			FallbackDirection,
			TraceSegments);

		for (const FNSEnemyPartTraceSegment& TraceSegment : TraceSegments)
		{
			const FVector Start = TraceSegment.Start;
			const FVector SegmentDirection = (TraceSegment.End - TraceSegment.Start).GetSafeNormal();
			const FTransform BeamStartTransform(SegmentDirection.Rotation(), Start, FVector::OneVector);

			const FVector Direction = bHasLockedLaserAimPoint
				                          ? ResolveLockedLaserDirection(*CachedAttackRow, BeamStartTransform)
				                          : ResolveLaserDirection(*CachedAttackRow, BeamStartTransform, AttackActor);

			if (Direction.IsNearlyZero())
			{
				continue;
			}

			FNSLaserBeam Beam;
			Beam.Start = Start;
			Beam.End = Start + Direction * Range;
			OutBeams.Add(Beam);
		}

		if (!OutBeams.IsEmpty())
		{
			return;
		}
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return;
	}

	const FTransform FallbackTransform(AvatarActor->GetActorRotation(), AvatarActor->GetActorLocation(),
	                                   FVector::OneVector);
	const FVector Direction = bHasLockedLaserAimPoint
		                          ? ResolveLockedLaserDirection(*CachedAttackRow, FallbackTransform)
		                          : ResolveLaserDirection(*CachedAttackRow, FallbackTransform, AttackActor);

	if (Direction.IsNearlyZero())
	{
		return;
	}

	FNSLaserBeam Beam;
	Beam.Start = FallbackTransform.GetLocation();
	Beam.End = Beam.Start + Direction * Range;
	OutBeams.Add(Beam);
}

void UGA_EnemyAttackLaser::StartLaser()
{
	if (!CachedAttackRow)
	{
		CancelAttackAbility();
		return;
	}

	if (UNSEnemyCosmeticComponent* CosmeticComponent = GetEnemyCosmeticComponent())
	{
		LaserCosmeticInstanceId = CosmeticComponent->AllocateCosmeticInstanceId();
	}

	TArray<FNSLaserBeam> Beams;
	GetCurrentLaserBeams(Beams);

	SendLaserChargeStartCosmeticEvent(Beams);
	TickLaserChargeCosmeticUpdate();

	const float WarnTime = FMath::Max(CachedAttackRow->WarnTime, 0.0f);

	if (WarnTime <= 0.0f)
	{
		BeginLaserDamage();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			LaserCosmeticUpdateTimerHandle,
			this,
			&ThisClass::TickLaserChargeCosmeticUpdate,
			FMath::Max(LaserCosmeticUpdateInterval, 0.01f),
			true);

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

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LaserCosmeticUpdateTimerHandle);
	}

	if (!bHasLockedLaserAimPoint && !LockLaserAimPoint())
	{
		ClearLaserTimers();
		CancelAttackAbility();
		return;
	}

	TArray<FNSLaserBeam> Beams;
	GetCurrentLaserBeams(Beams);

	if (Beams.IsEmpty())
	{
		ClearLaserTimers();
		CancelAttackAbility();
		return;
	}

	SendLaserBeamStartCosmeticEvent(Beams);
	TickLaserBeamCosmeticUpdate();

	TickLaserDamage();

	if (UWorld* World = GetWorld())
	{
		const float TickInterval = FMath::Max(0.01f, CachedAttackRow->SustainData.TickInterval);
		World->GetTimerManager().SetTimer(
			LaserTickTimerHandle,
			this,
			&UGA_EnemyAttackLaser::TickLaserDamage,
			TickInterval,
			true
		);

		const float Duration = FMath::Max(0.f, CachedAttackRow->SustainData.Duration);
		World->GetTimerManager().SetTimer(
			LaserEndTimerHandle,
			this,
			&UGA_EnemyAttackLaser::CompleteLaser,
			Duration,
			false
		);

		World->GetTimerManager().SetTimer(
			LaserCosmeticUpdateTimerHandle,
			this,
			&UGA_EnemyAttackLaser::TickLaserBeamCosmeticUpdate,
			LaserCosmeticUpdateInterval,
			true
		);
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
		FTimerManager& TimerManager = World->GetTimerManager();

		TimerManager.ClearTimer(LaserStartTimerHandle);
		TimerManager.ClearTimer(LaserTickTimerHandle);
		TimerManager.ClearTimer(LaserEndTimerHandle);
		TimerManager.ClearTimer(LaserCosmeticUpdateTimerHandle);
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
	QueryParams.bFindInitialOverlaps = true;

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

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(NSCollisionChannels::Player);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(NSCollisionChannels::PlayerConstruct);

	TArray<FHitResult> HitResults;
	const bool bHit = World->SweepMultiByObjectType(
		HitResults,
		Beam.Start,
		Beam.End,
		FQuat::Identity,
		ObjectQueryParams,
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

UNSEnemyCosmeticComponent* UGA_EnemyAttackLaser::GetEnemyCosmeticComponent() const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	return IsValid(AvatarActor)
		       ? AvatarActor->FindComponentByClass<UNSEnemyCosmeticComponent>()
		       : nullptr;
}

void UGA_EnemyAttackLaser::SendLaserChargeStartCosmeticEvent(const TArray<FNSLaserBeam>& Beams) const
{
	if (!CachedAttackRow || LaserCosmeticInstanceId == INDEX_NONE || Beams.IsEmpty())
	{
		return;
	}

	FNSCosmeticEventNetData EventData;
	BuildLaserCosmeticEvent(
		EventData,
		NSGameplayTags::Cosmetic_Enemy_TitanWalker_Laser_ChargeStart,
		ENSCosmeticEventPhase::Start,
		Beams,
		FMath::Max(CachedAttackRow->WarnTime, 0.0f));

	if (UNSEnemyCosmeticComponent* CosmeticComponent = GetEnemyCosmeticComponent())
	{
		CosmeticComponent->SendCosmeticEvent(EventData, true);
	}
}

void UGA_EnemyAttackLaser::SendLaserChargeUpdateCosmeticEvent(const TArray<FNSLaserBeam>& Beams) const
{
	if (!CachedAttackRow || LaserCosmeticInstanceId == INDEX_NONE || Beams.IsEmpty())
	{
		return;
	}

	FNSCosmeticEventNetData EventData;
	BuildLaserCosmeticEvent(
		EventData,
		NSGameplayTags::Cosmetic_Enemy_TitanWalker_Laser_ChargeUpdate,
		ENSCosmeticEventPhase::Update,
		Beams,
		FMath::Max(CachedAttackRow->WarnTime, 0.0f));

	if (UNSEnemyCosmeticComponent* CosmeticComponent = GetEnemyCosmeticComponent())
	{
		CosmeticComponent->SendCosmeticEvent(EventData, false);
	}
}

void UGA_EnemyAttackLaser::SendLaserBeamStartCosmeticEvent(const TArray<FNSLaserBeam>& Beams) const
{
	if (!CachedAttackRow || LaserCosmeticInstanceId == INDEX_NONE || Beams.IsEmpty())
	{
		return;
	}

	FNSCosmeticEventNetData EventData;
	BuildLaserCosmeticEvent(
		EventData,
		NSGameplayTags::Cosmetic_Enemy_TitanWalker_Laser_BeamStart,
		ENSCosmeticEventPhase::Start,
		Beams,
		FMath::Max(CachedAttackRow->SustainData.Duration, 0.0f));

	if (UNSEnemyCosmeticComponent* CosmeticComponent = GetEnemyCosmeticComponent())
	{
		CosmeticComponent->SendCosmeticEvent(EventData, true);
	}
}

void UGA_EnemyAttackLaser::SendLaserBeamUpdateCosmeticEvent(const TArray<FNSLaserBeam>& Beams) const
{
	if (!CachedAttackRow || LaserCosmeticInstanceId == INDEX_NONE || Beams.IsEmpty())
	{
		return;
	}

	FNSCosmeticEventNetData EventData;
	BuildLaserCosmeticEvent(
		EventData,
		NSGameplayTags::Cosmetic_Enemy_TitanWalker_Laser_BeamUpdate,
		ENSCosmeticEventPhase::Update,
		Beams,
		FMath::Max(CachedAttackRow->SustainData.Duration, 0.0f));

	if (UNSEnemyCosmeticComponent* CosmeticComponent = GetEnemyCosmeticComponent())
	{
		CosmeticComponent->SendCosmeticEvent(EventData, false);
	}
}

void UGA_EnemyAttackLaser::SendLaserStopCosmeticEvent() const
{
	if (LaserCosmeticInstanceId == INDEX_NONE)
	{
		return;
	}

	FNSCosmeticEventNetData EventData;
	EventData.EventTag = NSGameplayTags::Cosmetic_Enemy_TitanWalker_Laser_Stop;
	EventData.InstanceId = LaserCosmeticInstanceId;
	EventData.Phase = ENSCosmeticEventPhase::Stop;

	if (UNSEnemyCosmeticComponent* CosmeticComponent = GetEnemyCosmeticComponent())
	{
		CosmeticComponent->SendCosmeticEvent(EventData, true);
	}
}

void UGA_EnemyAttackLaser::BuildLaserCosmeticEvent(
	FNSCosmeticEventNetData& OutEventData,
	FGameplayTag EventTag,
	ENSCosmeticEventPhase Phase,
	const TArray<FNSLaserBeam>& Beams,
	float Duration) const
{
	OutEventData = FNSCosmeticEventNetData();

	OutEventData.EventTag = EventTag;
	OutEventData.InstanceId = LaserCosmeticInstanceId;
	OutEventData.Phase = Phase;
	OutEventData.Duration = FMath::Max(Duration, 0.0f);

	if (CachedAttackRow)
	{
		OutEventData.Radius = GetLaserRadius(*CachedAttackRow);
		OutEventData.Range = GetLaserRange(*CachedAttackRow);
	}

	for (const FNSLaserBeam& Beam : Beams)
	{
		const FVector Direction = (Beam.End - Beam.Start).GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			continue;
		}

		FNSCosmeticEventPointNetData PointData;
		PointData.Location = Beam.Start;
		PointData.EndLocation = Beam.End;
		PointData.Direction = Direction;

		OutEventData.Points.Add(PointData);
	}

	if (!OutEventData.Points.IsEmpty())
	{
		const FNSCosmeticEventPointNetData& FirstPoint = OutEventData.Points[0];

		OutEventData.Location = FirstPoint.Location;
		OutEventData.EndLocation = FirstPoint.EndLocation;
		OutEventData.Direction = FirstPoint.Direction;
	}
}

void UGA_EnemyAttackLaser::TickLaserChargeCosmeticUpdate()
{
	if (!CachedAttackRow || LaserCosmeticInstanceId == INDEX_NONE)
	{
		return;
	}

	TArray<FNSLaserBeam> Beams;
	GetCurrentLaserBeams(Beams);

	SendLaserChargeUpdateCosmeticEvent(Beams);
}

void UGA_EnemyAttackLaser::TickLaserBeamCosmeticUpdate()
{
	if (!CachedAttackRow)
	{
		return;
	}

	UpdateReplicatedLockedLaserAimTargetLocation();

	if (LaserCosmeticInstanceId == INDEX_NONE)
	{
		return;
	}

	TArray<FNSLaserBeam> Beams;
	GetCurrentLaserBeams(Beams);

	SendLaserBeamUpdateCosmeticEvent(Beams);
}

// 현재 Avatar를 제어하는 BossAIController를 반환하는 함수
ANSBossAIController* UGA_EnemyAttackLaser::GetBossController() const
{
	const APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!AvatarPawn)
	{
		return nullptr;
	}

	return Cast<ANSBossAIController>(AvatarPawn->GetController());
}

// 현재 공격 대상 Actor를 반환하는 함수
AActor* UGA_EnemyAttackLaser::ResolveAttackActor() const
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

// AimMode와 타깃 상태 기준으로 레이저 조준 위치를 계산하는 함수
FVector UGA_EnemyAttackLaser::ResolveLaserAimPoint(
	const FNSEnemyAttackRow& AttackRow,
	const FTransform& MuzzleTransform,
	const AActor* AttackActor) const
{
	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	const FVector MuzzleForward = MuzzleTransform.GetRotation().GetForwardVector();
	const float Range = GetLaserRange(AttackRow);

	if (AttackRow.AimMode == ENSEnemyAimMode::None ||
		AttackRow.AimMode == ENSEnemyAimMode::Forward ||
		!IsValid(AttackActor))
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

// Muzzle 위치와 공격 대상 기준으로 레이저 진행 방향을 계산하는 함수
FVector UGA_EnemyAttackLaser::ResolveLaserDirection(
	const FNSEnemyAttackRow& AttackRow,
	const FTransform& MuzzleTransform,
	const AActor* AttackActor) const
{
	const FVector MuzzleLocation = MuzzleTransform.GetLocation();

	FVector Direction =
		MuzzleTransform.GetRotation().GetForwardVector().GetSafeNormal();

	if (AttackRow.AimMode != ENSEnemyAimMode::None &&
		AttackRow.AimMode != ENSEnemyAimMode::Forward &&
		IsValid(AttackActor))
	{
		const FVector AimPoint = ResolveLaserAimPoint(AttackRow, MuzzleTransform, AttackActor);
		const FVector TargetDirection = (AimPoint - MuzzleLocation).GetSafeNormal();

		if (!TargetDirection.IsNearlyZero())
		{
			Direction = TargetDirection;
		}
	}

	return Direction;
}

UNSEnemyCombatComponent* UGA_EnemyAttackLaser::GetEnemyCombatComponent() const
{
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		return AvatarActor->FindComponentByClass<UNSEnemyCombatComponent>();
	}

	return nullptr;
}

bool UGA_EnemyAttackLaser::LockLaserAimPoint()
{
	if (!CachedAttackRow)
	{
		return false;
	}

	FTransform ReferenceTransform;
	if (!TryBuildLaserReferenceTransform(ReferenceTransform))
	{
		return false;
	}

	AActor* AttackActor = ResolveAttackActor();
	LockedLaserTargetActor = AttackActor;
	LockedLaserAimPoint = ResolveLaserAimPoint(*CachedAttackRow, ReferenceTransform, AttackActor);
	bHasLockedLaserAimPoint = true;

	FVector LockedDirection = (LockedLaserAimPoint - ReferenceTransform.GetLocation()).GetSafeNormal();
	if (LockedDirection.IsNearlyZero())
	{
		LockedDirection = ReferenceTransform.GetRotation().GetForwardVector().GetSafeNormal();
	}

	if (LockedDirection.IsNearlyZero())
	{
		LockedDirection = FVector::ForwardVector;
	}

	const FRotator LockedRotation = LockedDirection.Rotation();
	LockedLaserYaw = LockedRotation.Yaw;
	LockedLaserInitialPitch = LockedRotation.Pitch;

	const UWorld* World = GetWorld();
	LockedLaserBeamStartTime = World ? World->GetTimeSeconds() : 0.0f;

	UpdateReplicatedLockedLaserAimTargetLocation();

	return true;
}

void UGA_EnemyAttackLaser::ClearLockedLaserAimPoint()
{
	bHasLockedLaserAimPoint = false;
	LockedLaserAimPoint = FVector::ZeroVector;
	LockedLaserTargetActor.Reset();
	LockedLaserYaw = 0.0f;
	LockedLaserInitialPitch = 0.0f;
	LockedLaserBeamStartTime = 0.0f;

	if (UNSEnemyCombatComponent* CombatComponent = GetEnemyCombatComponent())
	{
		CombatComponent->ClearReplicatedAimTargetLocation();
	}
}

FVector UGA_EnemyAttackLaser::ResolveLockedLaserDirection(
	const FNSEnemyAttackRow& AttackRow,
	const FTransform& MuzzleTransform) const
{
	if (!bHasLockedLaserAimPoint)
	{
		return ResolveLaserDirection(AttackRow, MuzzleTransform, ResolveAttackActor());
	}

	const FVector CurrentDirection = ResolveCurrentLockedLaserDirection(AttackRow, MuzzleTransform);
	if (!CurrentDirection.IsNearlyZero())
	{
		return CurrentDirection;
	}

	return MuzzleTransform.GetRotation().GetForwardVector().GetSafeNormal();
}

bool UGA_EnemyAttackLaser::TryBuildLaserReferenceTransform(FTransform& OutReferenceTransform) const
{
	if (!CachedAttackRow)
	{
		return false;
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const UNSEnemyPartComponent* PartComponent = GetEnemyPartComponent();

	if (PartComponent)
	{
		TArray<FTransform> MuzzleTransforms;
		PartComponent->GetMuzzleTransformsByAttackId(CachedAttackRow->AttackId, MuzzleTransforms);

		if (!MuzzleTransforms.IsEmpty())
		{
			OutReferenceTransform = MuzzleTransforms[0];
			return true;
		}
	}

	if (PartComponent)
	{
		FVector FallbackDirection = AvatarActor
			                            ? AvatarActor->GetActorForwardVector().GetSafeNormal()
			                            : FVector::ForwardVector;

		if (FallbackDirection.IsNearlyZero())
		{
			FallbackDirection = FVector::ForwardVector;
		}

		TArray<FNSEnemyPartTraceSegment> TraceSegments;
		PartComponent->GetTraceSegmentsByAttackId(
			CachedAttackRow->AttackId,
			GetLaserRange(*CachedAttackRow),
			FallbackDirection,
			TraceSegments);

		if (!TraceSegments.IsEmpty())
		{
			FVector Direction = (TraceSegments[0].End - TraceSegments[0].Start).GetSafeNormal();

			if (Direction.IsNearlyZero() && AvatarActor)
			{
				Direction = AvatarActor->GetActorForwardVector().GetSafeNormal();
			}

			if (!Direction.IsNearlyZero())
			{
				OutReferenceTransform = FTransform(Direction.Rotation(), TraceSegments[0].Start, FVector::OneVector);
				return true;
			}
		}
	}

	if (AvatarActor)
	{
		OutReferenceTransform = FTransform(AvatarActor->GetActorRotation(), AvatarActor->GetActorLocation(),
		                                   FVector::OneVector);
		return true;
	}

	return false;
}

float UGA_EnemyAttackLaser::GetCurrentLockedLaserPitch(
	const FNSEnemyAttackRow& AttackRow,
	const FTransform& MuzzleTransform) const
{
	if (!bTrackLockedLaserPitchToTarget ||
		AttackRow.AimMode == ENSEnemyAimMode::None ||
		AttackRow.AimMode == ENSEnemyAimMode::Forward)
	{
		return LockedLaserInitialPitch;
	}

	const AActor* TrackingActor = LockedLaserTargetActor.Get();
	if (!IsValid(TrackingActor))
	{
		TrackingActor = ResolveAttackActor();
	}

	if (!IsValid(TrackingActor))
	{
		return LockedLaserInitialPitch;
	}

	const FVector AimPoint = ResolveLaserAimPoint(AttackRow, MuzzleTransform, TrackingActor);
	const FVector ToTarget = AimPoint - MuzzleTransform.GetLocation();

	if (ToTarget.IsNearlyZero())
	{
		return LockedLaserInitialPitch;
	}

	const float TargetPitch = ToTarget.Rotation().Pitch;
	float DeltaPitch = FMath::FindDeltaAngleDegrees(LockedLaserInitialPitch, TargetPitch);

	if (LaserPitchTrackingMaxAngle > 0.0f)
	{
		DeltaPitch = FMath::Clamp(DeltaPitch, -LaserPitchTrackingMaxAngle, LaserPitchTrackingMaxAngle);
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return FRotator::NormalizeAxis(LockedLaserInitialPitch + DeltaPitch);
	}

	const float ElapsedTime = FMath::Max(0.0f, World->GetTimeSeconds() - LockedLaserBeamStartTime);
	const float Duration = FMath::Max(0.01f, LaserPitchTrackingDuration);
	const float Alpha = FMath::Clamp(ElapsedTime / Duration, 0.0f, 1.0f);

	return FRotator::NormalizeAxis(LockedLaserInitialPitch + DeltaPitch * Alpha);
}

float UGA_EnemyAttackLaser::GetCurrentLockedLaserYaw(
	const FNSEnemyAttackRow& AttackRow,
	const FTransform& MuzzleTransform) const
{
	if (!bTrackLockedLaserYawToTarget ||
		AttackRow.AimMode == ENSEnemyAimMode::None ||
		AttackRow.AimMode == ENSEnemyAimMode::Forward)
	{
		return LockedLaserYaw;
	}

	const AActor* TrackingActor = LockedLaserTargetActor.Get();
	if (!IsValid(TrackingActor))
	{
		TrackingActor = ResolveAttackActor();
	}

	if (!IsValid(TrackingActor))
	{
		return LockedLaserYaw;
	}

	const FVector AimPoint = ResolveLaserAimPoint(AttackRow, MuzzleTransform, TrackingActor);
	const FVector ToTarget = AimPoint - MuzzleTransform.GetLocation();

	if (ToTarget.IsNearlyZero())
	{
		return LockedLaserYaw;
	}

	const float TargetYaw = ToTarget.Rotation().Yaw;
	float DeltaYaw = FMath::FindDeltaAngleDegrees(LockedLaserYaw, TargetYaw);

	if (LaserYawTrackingMaxAngle > 0.0f)
	{
		DeltaYaw = FMath::Clamp(DeltaYaw, -LaserYawTrackingMaxAngle, LaserYawTrackingMaxAngle);
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return FRotator::NormalizeAxis(LockedLaserYaw + DeltaYaw);
	}

	const float ElapsedTime = FMath::Max(0.0f, World->GetTimeSeconds() - LockedLaserBeamStartTime);
	const float Duration = FMath::Max(0.01f, LaserYawTrackingDuration);
	const float Alpha = FMath::Clamp(ElapsedTime / Duration, 0.0f, 1.0f);

	return FRotator::NormalizeAxis(LockedLaserYaw + DeltaYaw * Alpha);
}

FVector UGA_EnemyAttackLaser::ResolveCurrentLockedLaserDirection(
	const FNSEnemyAttackRow& AttackRow,
	const FTransform& MuzzleTransform) const
{
	if (!bHasLockedLaserAimPoint)
	{
		return FVector::ZeroVector;
	}

	const float CurrentPitch = GetCurrentLockedLaserPitch(AttackRow, MuzzleTransform);
	const float CurrentYaw = GetCurrentLockedLaserYaw(AttackRow, MuzzleTransform);
	const FRotator CurrentRotation(CurrentPitch, CurrentYaw, 0.0f);

	return CurrentRotation.Vector().GetSafeNormal();
}

void UGA_EnemyAttackLaser::UpdateReplicatedLockedLaserAimTargetLocation()
{
	if (!CachedAttackRow || !bHasLockedLaserAimPoint)
	{
		return;
	}

	UNSEnemyCombatComponent* CombatComponent = GetEnemyCombatComponent();
	if (!CombatComponent)
	{
		return;
	}

	FTransform ReferenceTransform;
	if (!TryBuildLaserReferenceTransform(ReferenceTransform))
	{
		return;
	}

	const FVector CurrentDirection = ResolveCurrentLockedLaserDirection(*CachedAttackRow, ReferenceTransform);
	if (CurrentDirection.IsNearlyZero())
	{
		return;
	}

	const FVector ReplicatedAimLocation =
		ReferenceTransform.GetLocation() + CurrentDirection * GetLaserRange(*CachedAttackRow);

	CombatComponent->SetReplicatedAimTargetLocation(ReplicatedAimLocation);
}
