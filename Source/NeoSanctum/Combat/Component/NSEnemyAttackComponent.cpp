// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyAttackComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "NSBossModeComponent.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"
#include "NeoSanctum/Interaction/Prop/NSDestructibleObjectBase.h"

UNSEnemyAttackComponent::UNSEnemyAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UNSEnemyAttackComponent::ResetAttackState()
{
	LastAttackTimeById.Reset();
}

void UNSEnemyAttackComponent::RecordAttackUsed(const FNSEnemyAttackRow& AttackRow)
{
	if (AttackRow.AttackId.IsNone())
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		LastAttackTimeById.FindOrAdd(AttackRow.AttackId) = World->GetTimeSeconds();
	}
}

const FNSEnemyAttackRow* UNSEnemyAttackComponent::SelectAttack(
	const AActor* TargetActor,
	const AActor* AttackActor,
	float Distance,
	bool bHasDirectLineOfSight,
	bool bSelectWeightedAttack) const
{
	const UNSEnemyData* EnemyData = GetEnemyData();
	if (!EnemyData)
	{
		return nullptr;
	}

	const float HealthRatio = GetOwnerHealthRatio();

	TArray<const FNSEnemyAttackRow*> Candidates;
	int32 BestPriority = TNumericLimits<int32>::Lowest();

	for (const FNSEnemyAttackRow* AttackRow : EnemyData->GetAttackRows())
	{
		if (!AttackRow)
		{
			continue;
		}

		if (!EnemyData->IsAttackAllowedByPhase(AttackRow->AttackId, HealthRatio))
		{
			continue;
		}

		if (!CanUseAttack(
			*AttackRow,
			TargetActor,
			AttackActor,
			Distance,
			bHasDirectLineOfSight))
		{
			continue;
		}

		if (!bSelectWeightedAttack)
		{
			return AttackRow;
		}

		if (AttackRow->Priority > BestPriority)
		{
			BestPriority = AttackRow->Priority;
			Candidates.Reset();
		}

		if (AttackRow->Priority == BestPriority)
		{
			Candidates.Add(AttackRow);
		}
	}

	if (Candidates.IsEmpty())
	{
		return nullptr;
	}

	float TotalWeight = 0.0f;
	for (const FNSEnemyAttackRow* Candidate : Candidates)
	{
		TotalWeight += EnemyData->GetPhaseAttackWeight(*Candidate, HealthRatio);
	}

	if (TotalWeight <= 0.0f)
	{
		return Candidates[0];
	}

	float Pick = FMath::FRandRange(0.0f, TotalWeight);

	for (const FNSEnemyAttackRow* Candidate : Candidates)
	{
		Pick -= EnemyData->GetPhaseAttackWeight(*Candidate, HealthRatio);

		if (Pick <= 0.0f)
		{
			return Candidate;
		}
	}

	return Candidates.Last();
}

bool UNSEnemyAttackComponent::CanUseAttack(
	const FNSEnemyAttackRow& AttackRow,
	const AActor* TargetActor,
	const AActor* AttackActor,
	float Distance,
	bool bHasDirectLineOfSight) const
{
	return CanUseAttackInternal(
		AttackRow,
		GetOwnerBossModeTag(),
		true,
		TargetActor,
		AttackActor,
		Distance,
		bHasDirectLineOfSight);
}

bool UNSEnemyAttackComponent::CanUseAttackInMode(
	const FNSEnemyAttackRow& AttackRow,
	FGameplayTag ModeTag,
	const AActor* TargetActor,
	const AActor* AttackActor,
	float Distance,
	bool bHasDirectLineOfSight) const
{
	return CanUseAttackInternal(
		AttackRow,
		ModeTag,
		false,
		TargetActor,
		AttackActor,
		Distance,
		bHasDirectLineOfSight);
}

const UNSEnemyData* UNSEnemyAttackComponent::GetEnemyData() const
{
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(GetOwner());
	return EnemyAgent ? EnemyAgent->GetEnemyData() : nullptr;
}

float UNSEnemyAttackComponent::GetOwnerHealthRatio() const
{
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner());
	const UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return 1.0f;
	}

	const UNSMonsterAttributeSet* Attr = ASC->GetSet<UNSMonsterAttributeSet>();
	if (!Attr)
	{
		return 1.0f;
	}

	const float MaxHealth = FMath::Max(Attr->GetMaxHealth(), 1.0f);
	return FMath::Clamp(Attr->GetHealth(), 0.0f, MaxHealth) / MaxHealth;
}

bool UNSEnemyAttackComponent::IsValidLivingTarget(const AActor* Target) const
{
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Target);
	const UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return false;
	}

	for (UAttributeSet* AttributeSet : ASC->GetSpawnedAttributes())
	{
		if (!AttributeSet)
		{
			continue;
		}

		if (FProperty* Prop = AttributeSet->GetClass()->FindPropertyByName(TEXT("Health")))
		{
			const FGameplayAttribute HealthAttribute(Prop);
			return ASC->GetNumericAttribute(HealthAttribute) > 0.0f;
		}
	}

	return false;
}

bool UNSEnemyAttackComponent::CanUseDestructibleCoverAttack(
	const FNSEnemyAttackRow& AttackRow,
	const AActor* TargetActor,
	const AActor* AttackActor,
	bool bHasDirectLineOfSight) const
{
	if (bHasDirectLineOfSight ||
		!IsValidLivingTarget(TargetActor) ||
		!IsValidLivingTarget(AttackActor) ||
		AttackActor == TargetActor ||
		!AttackActor->IsA<ANSDestructibleObjectBase>())
	{
		return false;
	}

	return AttackRow.AttackType == ENSEnemyAttackType::Projectile ||
		AttackRow.AttackType == ENSEnemyAttackType::Hitscan;
}

bool UNSEnemyAttackComponent::CanUseAttackInternal(
	const FNSEnemyAttackRow& AttackRow,
	FGameplayTag ModeTag,
	bool bAllowInvalidModeTag,
	const AActor* TargetActor,
	const AActor* AttackActor,
	float Distance,
	bool bHasDirectLineOfSight) const
{
	const UNSEnemyData* EnemyData = GetEnemyData();
	if (!EnemyData ||
		AttackRow.AttackId.IsNone() ||
		!AttackRow.AbilityClass ||
		!IsValidLivingTarget(TargetActor) ||
		!IsValid(AttackActor))
	{
		return false;
	}

	if (!IsAttackAllowedByModeTag(AttackRow, ModeTag, bAllowInvalidModeTag))
	{
		return false;
	}

	if (Distance < AttackRow.Condition.MinRange ||
		Distance > AttackRow.Condition.MaxRange)
	{
		return false;
	}

	if (AttackRow.Condition.bRequireLineOfSight &&
		!bHasDirectLineOfSight &&
		!CanUseDestructibleCoverAttack(
			AttackRow,
			TargetActor,
			AttackActor,
			bHasDirectLineOfSight))
	{
		return false;
	}

	const float Cooldown =
		EnemyData->GetPhaseAttackCooldown(AttackRow, GetOwnerHealthRatio());

	if (Cooldown > 0.0f)
	{
		const float* LastAttackTime = LastAttackTimeById.Find(AttackRow.AttackId);
		if (LastAttackTime)
		{
			const UWorld* World = GetWorld();
			if (!World)
			{
				return false;
			}

			if (World->GetTimeSeconds() - *LastAttackTime < Cooldown)
			{
				return false;
			}
		}
	}

	return true;
}

bool UNSEnemyAttackComponent::IsAttackAllowedByModeTag(
	const FNSEnemyAttackRow& AttackRow,
	FGameplayTag ModeTag,
	bool bAllowInvalidModeTag) const
{
	if (AttackRow.AllowedModeTags.IsEmpty())
	{
		return true;
	}

	if (!ModeTag.IsValid())
	{
		return bAllowInvalidModeTag;
	}

	return ModeTag.MatchesAny(AttackRow.AllowedModeTags);
}

bool UNSEnemyAttackComponent::IsAttackAllowedByMode(
	const FNSEnemyAttackRow& AttackRow) const
{
	return IsAttackAllowedByModeTag(
		AttackRow,
		GetOwnerBossModeTag(),
		true);
}

FGameplayTag UNSEnemyAttackComponent::GetOwnerBossModeTag() const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return FGameplayTag();
	}

	const UNSBossModeComponent* BossModeComponent = OwnerActor->FindComponentByClass<UNSBossModeComponent>();

	return BossModeComponent
		       ? BossModeComponent->GetCurrentModeTag()
		       : FGameplayTag();
}
