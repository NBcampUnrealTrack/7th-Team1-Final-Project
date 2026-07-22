// Copyright 2026 One Team. All rights reserved.

#include "NSEnemyPhaseComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
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

	if (bPatternLocked)
	{
		return CurrentPhaseRow;
	}

	const FNSEnemyPhaseRow* DesiredPhaseRow =
		EnemyData->FindPhaseRowByHealthRatio(HealthRatio);

	if (!DesiredPhaseRow)
	{
		return nullptr;
	}

	if (!bPhaseInitialized)
	{
		bPhaseInitialized = true;
		EnterPhase(*DesiredPhaseRow, false);
		return CurrentPhaseRow;
	}

	const FNSEnemyPhaseRow* NextPhaseRow =
		ResolveNextPhaseRow(DesiredPhaseRow);

	if (NextPhaseRow && CurrentPhaseId != NextPhaseRow->PhaseId)
	{
		EnterPhase(*NextPhaseRow, true);
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

	const bool bPhaseChanged =
		CurrentPhaseRow != nullptr &&
		CurrentPhaseId != NewPhaseRow.PhaseId;

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

	if (bPhaseChanged)
	{
		ResetOwnerHitGauge();
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

const FNSEnemyPhaseRow* UNSEnemyPhaseComponent::ResolveNextPhaseRow(
	const FNSEnemyPhaseRow* DesiredPhaseRow) const
{
	if (!DesiredPhaseRow || !CurrentPhaseRow)
	{
		return DesiredPhaseRow;
	}

	if (CurrentPhaseId == DesiredPhaseRow->PhaseId)
	{
		return CurrentPhaseRow;
	}

	const UNSEnemyData* EnemyData = GetEnemyData();
	if (!EnemyData)
	{
		return DesiredPhaseRow;
	}

	const TArray<const FNSEnemyPhaseRow*>& PhaseRows = EnemyData->GetPhaseRows();

	int32 CurrentIndex = INDEX_NONE;
	int32 DesiredIndex = INDEX_NONE;

	for (int32 Index = 0; Index < PhaseRows.Num(); ++Index)
	{
		const FNSEnemyPhaseRow* PhaseRow = PhaseRows[Index];
		if (!PhaseRow)
		{
			continue;
		}

		if (PhaseRow->PhaseId == CurrentPhaseId)
		{
			CurrentIndex = Index;
		}

		if (PhaseRow->PhaseId == DesiredPhaseRow->PhaseId)
		{
			DesiredIndex = Index;
		}
	}

	if (CurrentIndex == INDEX_NONE || DesiredIndex == INDEX_NONE)
	{
		return DesiredPhaseRow;
	}

	if (DesiredIndex < CurrentIndex)
	{
		const int32 NextIndex = FMath::Max(CurrentIndex - 1, 0);
		return PhaseRows.IsValidIndex(NextIndex) ? PhaseRows[NextIndex] : CurrentPhaseRow;
	}

	return CurrentPhaseRow;
}

UNSEnemyStateComponent* UNSEnemyPhaseComponent::GetOwnerStateComponent() const
{
	const AActor* OwnerActor = GetOwner();

	return OwnerActor
		       ? OwnerActor->FindComponentByClass<UNSEnemyStateComponent>()
		       : nullptr;
}

void UNSEnemyPhaseComponent::ResetOwnerHitGauge() const
{
	if (UNSEnemyStateComponent* StateComponent = GetOwnerStateComponent())
	{
		StateComponent->ResetHitGauge();
	}
}
