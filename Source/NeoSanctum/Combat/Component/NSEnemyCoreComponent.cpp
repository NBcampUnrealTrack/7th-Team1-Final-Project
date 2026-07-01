// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyCoreComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"

UNSEnemyCoreComponent::UNSEnemyCoreComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNSEnemyCoreComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNSEnemyCoreComponent, EnemyData);
}

void UNSEnemyCoreComponent::SetEnemyData(UNSEnemyData* InEnemyData)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !InEnemyData)
	{
		return;
	}

	EnemyData = InEnemyData;
	EnemyData->InvalidateCachedRows();

	OnEnemyDataChanged.Broadcast(EnemyData);
}

void UNSEnemyCoreComponent::OnRep_EnemyData()
{
	if (EnemyData)
	{
		EnemyData->InvalidateCachedRows();
	}

	OnEnemyDataChanged.Broadcast(EnemyData);
}

void UNSEnemyCoreComponent::InitializeFromData(
	bool bFullInit,
	UNSMonsterAttributeSet* AttributeSet,
	TSubclassOf<UGameplayAbility> DeathAbilityClass)
{
	if (!EnemyData)
	{
		return;
	}

	EnemyData->InvalidateCachedRows();

	InitializeAttributes(AttributeSet);

	if (bFullInit)
	{
		GrantStartupAbilities(DeathAbilityClass);
	}
}

UAbilitySystemComponent* UNSEnemyCoreComponent::GetOwnerASC() const
{
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner());
	return ASI ? ASI->GetAbilitySystemComponent() : nullptr;
}

void UNSEnemyCoreComponent::InitializeAttributes(UNSMonsterAttributeSet* AttributeSet)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !EnemyData || !EnemyData->AttributeInitData || !AttributeSet)
	{
		return;
	}

	const FName RowName = EnemyData->EnemyId.GetTagName();
	const FNSMonsterAttributeRow* StatRow =
		EnemyData->AttributeInitData->FindRow<FNSMonsterAttributeRow>(RowName, TEXT(""));

	if (!StatRow)
	{
		return;
	}

	const float ScaledMaxHealth =
		(StatRow->MaxHealth * (1.0f + DifficultyScale.HealthAddRatio)) * DifficultyScale.Multiply;

	const float ScaledBaseDamage =
		(StatRow->BaseDamage * (1.0f + DifficultyScale.DamageAddRatio)) * DifficultyScale.Multiply;

	const float ScaledDefense =
		(StatRow->Defense * (1.0f + DifficultyScale.DefenseAddRatio)) * DifficultyScale.Multiply;

	AttributeSet->SetMaxHealth(ScaledMaxHealth);
	AttributeSet->SetHealth(ScaledMaxHealth);
	AttributeSet->SetBaseDamage(ScaledBaseDamage);
	AttributeSet->SetDefense(ScaledDefense);

	AttributeSet->SetMaxHitGauge(FMath::Max(StatRow->MaxHitGauge, 1.0f));
	AttributeSet->SetHitGaugeGainPerHit(FMath::Max(StatRow->HitGaugeGainPerHit, 0.0f));
	AttributeSet->ResetHitGauge();
}

void UNSEnemyCoreComponent::GrantStartupAbilities(
	TSubclassOf<UGameplayAbility> DeathAbilityClass)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !EnemyData)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : EnemyData->DefaultEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(GetOwner());

		FGameplayEffectSpecHandle SpecHandle =
			ASC->MakeOutgoingSpec(EffectClass, 1.0f, Context);

		if (SpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	TSet<UClass*> GrantedAbilityClasses;

	auto GiveAbilityOnce =
		[ASC, &GrantedAbilityClasses](
		TSubclassOf<UGameplayAbility> AbilityClass,
		bool bUseAbilityNetPolicyInputId)
	{
		if (!ASC || !AbilityClass)
		{
			return;
		}

		UClass* AbilityRawClass = AbilityClass.Get();
		if (!AbilityRawClass || GrantedAbilityClasses.Contains(AbilityRawClass))
		{
			return;
		}

		const UGameplayAbility* AbilityCDO = AbilityClass.GetDefaultObject();
		if (!AbilityCDO)
		{
			return;
		}

		GrantedAbilityClasses.Add(AbilityRawClass);

		const int32 InputId = bUseAbilityNetPolicyInputId
			                      ? static_cast<int32>(AbilityCDO->GetNetExecutionPolicy())
			                      : -1;

		ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, InputId));
	};

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : EnemyData->DefaultAbilities)
	{
		GiveAbilityOnce(AbilityClass, true);
	}

	GiveAbilityOnce(EnemyData->HitReactionAbilityClass, true);

	for (const FNSEnemyAttackRow* AttackRow : EnemyData->GetAttackRows())
	{
		if (AttackRow)
		{
			GiveAbilityOnce(AttackRow->AbilityClass, true);
		}
	}

	GiveAbilityOnce(DeathAbilityClass, false);
}
