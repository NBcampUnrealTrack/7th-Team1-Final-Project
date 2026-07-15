// Copyright 2026 One Team. All rights reserved.

#include "GA_EnemyAttackBombard.h"

#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Combat/Component/Artillery/NSBossArtilleryComponent.h"
#include "NeoSanctum/Combat/Cosmetic/NSEnemyCosmeticComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "NeoSanctum/Type/NSCosmeticEventTypes.h"

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
	UnbindArtilleryFinishedDelegate();

	CachedAttackRow = GetCurrentAttackRow();

	bArtilleryStarted = false;
	bMontageCompleted = false;
	bArtilleryCompleted = false;
	ActiveArtilleryExecutionId = 0;
	ArtilleryFinishedDelegateHandle.Reset();
	ActiveArtilleryComponent.Reset();
}

void UGA_EnemyAttackBombard::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UNSBossArtilleryComponent* ArtilleryComponent = ActiveArtilleryComponent.Get();

	if (ArtilleryComponent && ArtilleryFinishedDelegateHandle.IsValid())
	{
		ArtilleryComponent->OnArtilleryExecutionFinished.Remove(ArtilleryFinishedDelegateHandle);
	}

	if (bWasCancelled && ArtilleryComponent && ActiveArtilleryExecutionId > 0)
	{
		ArtilleryComponent->CancelArtilleryExecution(ActiveArtilleryExecutionId);
	}

	CachedAttackRow = nullptr;
	bArtilleryStarted = false;
	bMontageCompleted = false;
	bArtilleryCompleted = false;
	ActiveArtilleryExecutionId = 0;
	ArtilleryFinishedDelegateHandle.Reset();
	ActiveArtilleryComponent.Reset();

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

	if (!bArtilleryStarted)
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

UNSBossArtilleryComponent* UGA_EnemyAttackBombard::GetBossArtilleryComponent() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();

	return AvatarActor
		       ? AvatarActor->FindComponentByClass<UNSBossArtilleryComponent>()
		       : nullptr;
}

void UGA_EnemyAttackBombard::StartBombardVolley()
{
	if (!IsActive() || !CachedAttackRow || bArtilleryStarted)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	UNSBossArtilleryComponent* ArtilleryComponent = GetBossArtilleryComponent();
	if (!ArtilleryComponent || !ArtilleryComponent->HasPatternData())
	{
		CancelAttackAbility();
		return;
	}

	FNSBossArtilleryExecutionData ExecutionData;
	if (!ArtilleryComponent->SelectAndBuildExecutionDataNowFromRegisteredCombatants(ExecutionData, true))
	{
		CancelAttackAbility();
		return;
	}

	bArtilleryStarted = true;
	bArtilleryCompleted = false;
	ActiveArtilleryExecutionId = ExecutionData.ExecutionId;
	ActiveArtilleryComponent = ArtilleryComponent;

	ArtilleryFinishedDelegateHandle =
		ArtilleryComponent->OnArtilleryExecutionFinished.AddUObject(
			this,
			&ThisClass::HandleArtilleryExecutionFinished);

	if (!ArtilleryComponent->ExecuteArtilleryExecutionData(ExecutionData))
	{
		UnbindArtilleryFinishedDelegate();

		bArtilleryStarted = false;
		bArtilleryCompleted = false;
		ActiveArtilleryExecutionId = 0;

		CancelAttackAbility();
	}
}

void UGA_EnemyAttackBombard::HandleArtilleryExecutionFinished(int32 ExecutionId)
{
	if (ExecutionId != ActiveArtilleryExecutionId)
	{
		return;
	}

	bArtilleryCompleted = true;
	ActiveArtilleryExecutionId = 0;

	UnbindArtilleryFinishedDelegate();
	TryFinishBombardAbility();
}

void UGA_EnemyAttackBombard::UnbindArtilleryFinishedDelegate()
{
	if (UNSBossArtilleryComponent* ArtilleryComponent = ActiveArtilleryComponent.Get())
	{
		if (ArtilleryFinishedDelegateHandle.IsValid())
		{
			ArtilleryComponent->OnArtilleryExecutionFinished.Remove(ArtilleryFinishedDelegateHandle);
		}
	}

	ArtilleryFinishedDelegateHandle.Reset();
	ActiveArtilleryComponent.Reset();
}

void UGA_EnemyAttackBombard::TryFinishBombardAbility()
{
	if (!bMontageCompleted || !bArtilleryCompleted)
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
