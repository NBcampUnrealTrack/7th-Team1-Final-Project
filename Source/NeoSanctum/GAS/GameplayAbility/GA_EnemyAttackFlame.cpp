// Copyright 2026 One Team. All rights reserved.

#include "GA_EnemyAttackFlame.h"

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
#include "NeoSanctum/Combat/Cosmetic/NSEnemyCosmeticComponent.h"
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

	FlameStartTime = 0.0f;
	FlameCurrentRange = 0.0f;
	FlameCosmeticInstanceId = INDEX_NONE;
}

void UGA_EnemyAttackFlame::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	SendFlameStopCosmeticEvent();
	ClearFlameTimers();

	FlameCurrentRange = 0.0f;
	FlameStartTime = 0.0f;
	FlameCosmeticInstanceId = INDEX_NONE;
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

	FlameStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	FlameCurrentRange = 0.0f;

	if (UNSEnemyCosmeticComponent* CosmeticComponent = GetEnemyCosmeticComponent())
	{
		FlameCosmeticInstanceId = CosmeticComponent->AllocateCosmeticInstanceId();
	}

	SendFlameStartCosmeticEvent();
	TickFlameCosmeticUpdate();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FlameCosmeticUpdateTimerHandle,
			this,
			&ThisClass::TickFlameCosmeticUpdate,
			FMath::Max(FlameCosmeticUpdateInterval, 0.01f),
			true);
	}

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
	if (!CachedAttackRow)
	{
		return;
	}

	TArray<FNSFlameEmitter> Emitters;
	GetCurrentFlameEmitters(Emitters);

	if (Emitters.IsEmpty())
	{
		return;
	}

	FlameCurrentRange = GetCurrentFlameRange();

	if (FlameCurrentRange <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	TSet<AActor*> DamagedTargets;

	for (const FNSFlameEmitter& Emitter : Emitters)
	{
		CollectTargetsForEmitter(Emitter, FlameCurrentRange, DamagedTargets);
	}

	for (AActor* Target : DamagedTargets)
	{
		ApplyFlameDamageToTarget(Target, *CachedAttackRow);
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
		FTimerManager& TimerManager = World->GetTimerManager();

		TimerManager.ClearTimer(FlameTickTimerHandle);
		TimerManager.ClearTimer(FlameEndTimerHandle);
		TimerManager.ClearTimer(FlameCosmeticUpdateTimerHandle);
	}
}

void UGA_EnemyAttackFlame::CollectTargetsForEmitter(
	const FNSFlameEmitter& Emitter,
	float EffectiveRange,
	TSet<AActor*>& OutTargets) const
{
	if (!CachedAttackRow)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Range = FMath::Clamp(EffectiveRange, 0.0f, GetFlameRange(*CachedAttackRow));

	if (Range <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float QueryRadius = FMath::Max(Range + CachedAttackRow->AreaData.Radius, 1.0f);

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyFlameAttack), false, GetAvatarActorFromActorInfo());

	const bool bHit = World->OverlapMultiByChannel(
		OverlapResults,
		Emitter.Start,
		FQuat::Identity,
		FlameTraceChannel.GetValue(),
		FCollisionShape::MakeSphere(QueryRadius),
		QueryParams);

	if (!bHit)
	{
		return;
	}

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* TargetActor = Result.GetActor();

		if (!IsValidDamageTarget(TargetActor))
		{
			continue;
		}

		const FVector TargetLocation = GetTargetCheckLocation(TargetActor);

		if (IsLocationInsideCone(Emitter.Start, Emitter.Direction, TargetLocation, *CachedAttackRow, Range))
		{
			OutTargets.Add(TargetActor);
		}
	}
}

bool UGA_EnemyAttackFlame::IsLocationInsideCone(
	const FVector& Origin,
	const FVector& Direction,
	const FVector& TargetLocation,
	const FNSEnemyAttackRow& AttackRow,
	float EffectiveRange
) const
{
	const FVector SafeDirection = Direction.GetSafeNormal();

	if (SafeDirection.IsNearlyZero())
	{
		return false;
	}

	const float Range = FMath::Max(EffectiveRange, 0.0f);

	if (Range <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector ToTarget = TargetLocation - Origin;
	const float ForwardDistance = FVector::DotProduct(ToTarget, SafeDirection);

	if (ForwardDistance < 0.0f || ForwardDistance > Range)
	{
		return false;
	}

	const FVector ClosestPointOnAxis = Origin + SafeDirection * ForwardDistance;
	const float DistanceFromAxisSq = FVector::DistSquared(TargetLocation, ClosestPointOnAxis);

	const float ConeHalfAngleRadians = FMath::DegreesToRadians(FMath::Max(AttackRow.AreaData.ConeHalfAngle, 0.0f));
	const float RadiusAtDistance = FMath::Max(AttackRow.AreaData.Radius, 0.0f) + FMath::Tan(ConeHalfAngleRadians) *
		ForwardDistance;

	return DistanceFromAxisSq <= FMath::Square(RadiusAtDistance);
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

UNSEnemyCosmeticComponent* UGA_EnemyAttackFlame::GetEnemyCosmeticComponent() const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	return IsValid(AvatarActor)
		       ? AvatarActor->FindComponentByClass<UNSEnemyCosmeticComponent>()
		       : nullptr;
}

void UGA_EnemyAttackFlame::SendFlameStartCosmeticEvent() const
{
	if (!CachedAttackRow || FlameCosmeticInstanceId == INDEX_NONE)
	{
		return;
	}

	TArray<FNSFlameEmitter> Emitters;
	GetCurrentFlameEmitters(Emitters);

	FNSCosmeticEventNetData EventData;
	BuildFlameCosmeticEvent(
		EventData,
		NSGameplayTags::Cosmetic_Enemy_TitanWalker_Flame_Start,
		ENSCosmeticEventPhase::Start,
		Emitters,
		0.0f);

	if (UNSEnemyCosmeticComponent* CosmeticComponent = GetEnemyCosmeticComponent())
	{
		CosmeticComponent->SendCosmeticEvent(EventData, true);
	}
}

void UGA_EnemyAttackFlame::SendFlameUpdateCosmeticEvent(
	const TArray<FNSFlameEmitter>& Emitters,
	float EffectiveRange) const
{
	if (!CachedAttackRow || FlameCosmeticInstanceId == INDEX_NONE)
	{
		return;
	}

	FNSCosmeticEventNetData EventData;
	BuildFlameCosmeticEvent(
		EventData,
		NSGameplayTags::Cosmetic_Enemy_TitanWalker_Flame_Update,
		ENSCosmeticEventPhase::Update,
		Emitters,
		EffectiveRange);

	if (UNSEnemyCosmeticComponent* CosmeticComponent = GetEnemyCosmeticComponent())
	{
		CosmeticComponent->SendCosmeticEvent(EventData, false);
	}
}

void UGA_EnemyAttackFlame::SendFlameStopCosmeticEvent() const
{
	if (FlameCosmeticInstanceId == INDEX_NONE)
	{
		return;
	}

	FNSCosmeticEventNetData EventData;
	EventData.EventTag = NSGameplayTags::Cosmetic_Enemy_TitanWalker_Flame_Stop;
	EventData.InstanceId = FlameCosmeticInstanceId;
	EventData.Phase = ENSCosmeticEventPhase::Stop;

	if (UNSEnemyCosmeticComponent* CosmeticComponent = GetEnemyCosmeticComponent())
	{
		CosmeticComponent->SendCosmeticEvent(EventData, true);
	}
}

void UGA_EnemyAttackFlame::BuildFlameCosmeticEvent(
	FNSCosmeticEventNetData& OutEventData,
	FGameplayTag EventTag,
	ENSCosmeticEventPhase Phase,
	const TArray<FNSFlameEmitter>& Emitters,
	float EffectiveRange) const
{
	OutEventData = FNSCosmeticEventNetData();

	OutEventData.EventTag = EventTag;
	OutEventData.InstanceId = FlameCosmeticInstanceId;
	OutEventData.Phase = Phase;

	float ClampedRange = FMath::Max(EffectiveRange, 0.0f);

	if (CachedAttackRow)
	{
		const float FinalRange = GetFlameRange(*CachedAttackRow);

		ClampedRange = FMath::Clamp(ClampedRange, 0.0f, FinalRange);

		OutEventData.Radius = CachedAttackRow->AreaData.Radius;
		OutEventData.ConeHalfAngle = CachedAttackRow->AreaData.ConeHalfAngle;
		OutEventData.Duration = CachedAttackRow->SustainData.Duration;
	}

	OutEventData.Range = ClampedRange;

	for (const FNSFlameEmitter& Emitter : Emitters)
	{
		const FVector Direction = Emitter.Direction.GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			continue;
		}

		FNSCosmeticEventPointNetData PointData;
		PointData.Location = Emitter.Start;
		PointData.Direction = Direction;
		PointData.EndLocation = Emitter.Start + Direction * OutEventData.Range;

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

void UGA_EnemyAttackFlame::TickFlameCosmeticUpdate()
{
	if (!CachedAttackRow || FlameCosmeticInstanceId == INDEX_NONE)
	{
		return;
	}

	TArray<FNSFlameEmitter> Emitters;
	GetCurrentFlameEmitters(Emitters);

	FlameCurrentRange = GetCurrentFlameRange();

	SendFlameUpdateCosmeticEvent(
		Emitters,
		FlameCurrentRange);
}

float UGA_EnemyAttackFlame::GetFlameRange(const FNSEnemyAttackRow& AttackRow) const
{
	return AttackRow.AreaData.Range > 0.0f
		       ? AttackRow.AreaData.Range
		       : AttackRow.Condition.MaxRange;
}

float UGA_EnemyAttackFlame::GetFlameRangeAlpha() const
{
	if (FlameRangeGrowDuration <= KINDA_SMALL_NUMBER)
	{
		return 1.0f;
	}

	const UWorld* World = GetWorld();
	if (!World || FlameStartTime <= 0.0f)
	{
		return 0.0f;
	}

	const float ElapsedTime = World->GetTimeSeconds() - FlameStartTime;
	const float LinearAlpha = FMath::Clamp(ElapsedTime / FlameRangeGrowDuration, 0.0f, 1.0f);

	return FMath::Pow(LinearAlpha, 1.6f);
}

float UGA_EnemyAttackFlame::GetCurrentFlameRange() const
{
	if (!CachedAttackRow)
	{
		return 0.0f;
	}

	return GetFlameRange(*CachedAttackRow) * GetFlameRangeAlpha();
}
