// Copyright 2026 One Team. All rights reserved.

#include "NSEnemyPhaseComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"

UNSEnemyPhaseComponent::UNSEnemyPhaseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UNSEnemyPhaseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetPhaseState();

	Super::EndPlay(EndPlayReason);
}

const FNSEnemyPhaseRow* UNSEnemyPhaseComponent::UpdatePhase(float HealthRatio)
{
	const UNSEnemyData* EnemyData = GetEnemyData();
	if (!EnemyData)
	{
		return nullptr;
	}

	const FNSEnemyPhaseRow* PhaseRow = EnemyData->FindPhaseRowByHealthRatio(HealthRatio);

	if (!PhaseRow)
	{
		return nullptr;
	}

	if (!bPhaseInitialized)
	{
		bPhaseInitialized = true;
		EnterPhase(*PhaseRow, false);
		return CurrentPhaseRow;
	}

	if (CurrentPhaseId != PhaseRow->PhaseId)
	{
		EnterPhase(*PhaseRow, true);
	}

	return CurrentPhaseRow;
}

void UNSEnemyPhaseComponent::ResetPhaseState()
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (ASC)
	{
		ASC->OnAbilityEnded.RemoveAll(this);

		if (CurrentPhaseTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(CurrentPhaseTag);
		}
	}

	CurrentPhaseRow = nullptr;
	CurrentPhaseId = NAME_None;
	CurrentPhaseTag = FGameplayTag();
	bPhaseInitialized = false;
	bPatternLocked = false;
	CurrentTransitionGA = nullptr;
}

const UNSEnemyData* UNSEnemyPhaseComponent::GetEnemyData() const
{
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(GetOwner());
	return EnemyAgent ? EnemyAgent->GetEnemyData() : nullptr;
}

UAbilitySystemComponent* UNSEnemyPhaseComponent::GetOwnerASC() const
{
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner());
	return ASI ? ASI->GetAbilitySystemComponent() : nullptr;
}

void UNSEnemyPhaseComponent::EnterPhase(
	const FNSEnemyPhaseRow& NewPhaseRow,
	bool bPlayTransition)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	ASC->OnAbilityEnded.RemoveAll(this);

	if (CurrentPhaseTag.IsValid())
	{
		ASC->RemoveLooseGameplayTag(CurrentPhaseTag);
	}

	CurrentPhaseRow = &NewPhaseRow;
	CurrentPhaseId = NewPhaseRow.PhaseId;
	CurrentPhaseTag = NewPhaseRow.PhaseTag;
	bPatternLocked = false;
	CurrentTransitionGA = nullptr;

	if (CurrentPhaseTag.IsValid())
	{
		ASC->AddLooseGameplayTag(CurrentPhaseTag);
	}

	if (!bPlayTransition || !NewPhaseRow.TransitionGA)
	{
		return;
	}

	CurrentTransitionGA = NewPhaseRow.TransitionGA;
	bPatternLocked = NewPhaseRow.bLockPattern;

	ASC->OnAbilityEnded.AddUObject(
		this,
		&UNSEnemyPhaseComponent::OnTransitionAbilityEnded);

	const bool bActivated = ASC->TryActivateAbilityByClass(NewPhaseRow.TransitionGA);
	if (!bActivated)
	{
		ASC->OnAbilityEnded.RemoveAll(this);
		bPatternLocked = false;
		CurrentTransitionGA = nullptr;
	}
}

void UNSEnemyPhaseComponent::OnTransitionAbilityEnded(
	const FAbilityEndedData& AbilityEndedData)
{
	if (CurrentTransitionGA &&
		AbilityEndedData.AbilityThatEnded &&
		!AbilityEndedData.AbilityThatEnded->IsA(CurrentTransitionGA))
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		ASC->OnAbilityEnded.RemoveAll(this);
	}

	bPatternLocked = false;
	CurrentTransitionGA = nullptr;
}