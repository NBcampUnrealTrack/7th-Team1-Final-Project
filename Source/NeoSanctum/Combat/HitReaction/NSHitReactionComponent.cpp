// Copyright 2026 One Team. All rights reserved.

#include "NSHitReactionComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/DataTable.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/Combat/NSHitReactionData.h"

UNSHitReactionComponent::UNSHitReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSHitReactionComponent::PlayHitReaction(const FNSHitReactionContext& Context) const
{
	// 실제 재생 전에 TargetType, Outcome, CueTag를 순서대로 확정
	const FNSHitReactionContext ResolvedContext = BuildResolvedContext(Context);
	const FGameplayTag CueTag = ResolveCueTag(ResolvedContext);
	if (!CueTag.IsValid())
	{
		return;
	}

	ExecuteHitCue(ResolvedContext, CueTag);
}

FNSHitReactionContext UNSHitReactionComponent::BuildResolvedContext(
	const FNSHitReactionContext& Context) const
{
	FNSHitReactionContext ResolvedContext = Context;

	if (ResolvedContext.TargetType == ENSHitFeedbackTargetType::Any)
	{
		// AttributeSet에서 TargetType을 모르면 컴포넌트의 기본 분류를 사용
		ResolvedContext.TargetType = TargetType;
	}

	ResolvedContext.Outcome = ResolveOutcome(ResolvedContext);
	return ResolvedContext;
}

ENSHitFeedbackOutcome UNSHitReactionComponent::ResolveOutcome(
	const FNSHitReactionContext& Context) const
{
	if (Context.Outcome != ENSHitFeedbackOutcome::None)
	{
		return Context.Outcome;
	}

	if (!Context.bTargetDead)
	{
		// 일반 피격은 결과 없음으로 유지
		return ENSHitFeedbackOutcome::None;
	}

	if (Context.TargetType == ENSHitFeedbackTargetType::Enemy)
	{
		return ENSHitFeedbackOutcome::Kill;
	}

	if (Context.TargetType == ENSHitFeedbackTargetType::Barrier ||
		Context.TargetType == ENSHitFeedbackTargetType::DestructibleObject ||
		Context.TargetType == ENSHitFeedbackTargetType::Turret)
	{
		return ENSHitFeedbackOutcome::Destroy;
	}

	return ENSHitFeedbackOutcome::None;
}

FGameplayTag UNSHitReactionComponent::ResolveCueTag(const FNSHitReactionContext& Context) const
{
	if (const FNSHitReactionData* ReactionData = FindBestReactionData(Context))
	{
		if (ReactionData->GameplayCueTag.IsValid())
		{
			return ReactionData->GameplayCueTag;
		}
	}
	
	return DefaultHitCueTag;
}

const FNSHitReactionData* UNSHitReactionComponent::FindBestReactionData(
	const FNSHitReactionContext& Context) const
{
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem)
	{
		return nullptr;
	}
	
	const TArray<FNSHitReactionData>& Rows = DataSubsystem->GetCachedHitReactionRows();
	const FNSHitReactionData* BestData = nullptr;
	int32 BestPriority = TNumericLimits<int32>::Lowest();
	
	for (const FNSHitReactionData& Row : Rows)
	{
		if (CheckReactionDataMatch(Row, Context) && (!BestData || Row.Priority > BestPriority))
		{
			BestData = &Row;
			BestPriority = Row.Priority;
		}
	}
	
	return BestData;
}

bool UNSHitReactionComponent::CheckReactionDataMatch(
	const FNSHitReactionData& Data,
	const FNSHitReactionContext& Context) const
{
	const bool bTargetMatched =
		Data.TargetType == ENSHitFeedbackTargetType::Any ||
		Data.TargetType == Context.TargetType;

	const bool bDamageLayerMatched =
		Data.DamageLayer == ENSHitReactionDamageLayer::Any ||
		Data.DamageLayer == Context.DamageLayer;
	
	const bool bQualityMatched =
		Data.HitQuality == ENSHitFeedbackQuality::Any ||
		Data.HitQuality == Context.HitQuality;
	
	const bool bOutcomeMatched =
		Data.Outcome == ENSHitFeedbackOutcome::Any ||
		Data.Outcome == Context.Outcome;
	
	return bTargetMatched && bDamageLayerMatched && bQualityMatched && bOutcomeMatched;
}

void UNSHitReactionComponent::ExecuteHitCue(
	const FNSHitReactionContext& Context,
	const FGameplayTag CueTag) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	if (!ASC)
	{
		return;
	}

	FGameplayCueParameters CueParameters;
	
	// GameplayCue Notify에서 Location, Normal, Damage 값을 사용할 수 있도록 전달
	CueParameters.Instigator = Context.InstigatorActor;
	CueParameters.EffectCauser = Context.InstigatorActor;
	CueParameters.Location = Context.HitLocation;
	CueParameters.Normal = Context.HitNormal;
	CueParameters.RawMagnitude = Context.DamageAmount;

	ASC->ExecuteGameplayCue(CueTag, CueParameters);
}
