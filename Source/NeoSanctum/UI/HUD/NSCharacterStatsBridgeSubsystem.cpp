// Copyright 2026 One Team. All rights reserved.


#include "NSCharacterStatsBridgeSubsystem.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"
#include "NeoSanctum/Type/NSCharacterStatsMessageTypes.h"



void UNSCharacterStatsBridgeSubsystem::BroadcastCharacterStats(APlayerController* OwningPlayer)
{
	if (!IsValid(OwningPlayer))
	{
		return;
	}
	
	const ANSPlayerState* NSPlayerState = OwningPlayer->GetPlayerState<ANSPlayerState>();
	if (!IsValid(NSPlayerState))
	{
		return;
	}
	
	const UAbilitySystemComponent* ASC = NSPlayerState->GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}
	
	BindCharacterStats(OwningPlayer);
	
	FNSCharacterStatsSnapshotMessage Message;
	Message.RequestId = FGuid::NewGuid();
	
	AddAttributeStat(
		Message.Stats,
		ASC,
		UNSBaseAttributeSet::GetMaxHealthAttribute(),
		NSGameplayTags::CombatStat_MaxHealth,
		NSLOCTEXT("CharacterStats", "MaxHealth", "최대 체력"));

	AddAttributeStat(
		Message.Stats,
		ASC,
		UNSPlayerAttributeSet::GetMaxShieldAttribute(),
		NSGameplayTags::CombatStat_MaxShield,
		NSLOCTEXT("CharacterStats", "MaxShield", "최대 보호막"));

	AddAttributeStat(
		Message.Stats,
		ASC,
		UNSPlayerAttributeSet::GetShieldRechargeRateAttribute(),
		NSGameplayTags::CombatStat_ShieldRechargeRate,
		NSLOCTEXT("CharacterStats", "ShieldRechargeRate", "보호막 재생량"));

		AddAttributeStat(
			Message.Stats,
			ASC,
			UNSPlayerAttributeSet::GetShieldRechargeCooldownAttribute(),
			NSGameplayTags::CombatStat_ShieldRechargeCooldown,
			NSLOCTEXT("CharacterStats", "ShieldRechargeCooldown", "보호막 재생 대기시간"),
			ENSCharacterStatDisplayType::Seconds);
			
	AddAttributeStat(Message.Stats,
		ASC,
		UNSBaseAttributeSet::GetBaseDamageAttribute(),
		NSGameplayTags::CombatStat_Damage,
		NSLOCTEXT("CharacterStats", "BaseDamage", "기본 공격력"));
	
	AddAttributeStat(
		Message.Stats,
		ASC,
		UNSBaseAttributeSet::GetDefenseAttribute(),
		NSGameplayTags::CombatStat_Defense,
		NSLOCTEXT("CharacterStats", "Defense", "방어력"));

	AddAttributeStat(
		Message.Stats,
		ASC,
		UNSBaseAttributeSet::GetMoveSpeedAttribute(),
		NSGameplayTags::CombatStat_MoveSpeed,
		NSLOCTEXT("CharacterStats", "MoveSpeed", "이동 속도"));

		AddAttributeStat(
			Message.Stats,
			ASC,
			UNSPlayerAttributeSet::GetCritChanceAttribute(),
			NSGameplayTags::CombatStat_CritChance,
			NSLOCTEXT("CharacterStats", "CritChance", "치명타 확률"),
			ENSCharacterStatDisplayType::Percent);

		AddAttributeStat(
			Message.Stats,
			ASC,
			UNSPlayerAttributeSet::GetCritDamageAttribute(),
			NSGameplayTags::CombatStat_CritDamage,
			NSLOCTEXT("CharacterStats", "CritDamage", "치명타 피해"),
			ENSCharacterStatDisplayType::Percent);

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_CharacterStats_Snapshot,
		Message);
}

void UNSCharacterStatsBridgeSubsystem::StopBroadcastCharacterStats()
{
	UnbindCharacterStats();
}

void UNSCharacterStatsBridgeSubsystem::AddAttributeStat(
	TArray<FNSCharacterStatViewData>& OutStats,
	const UAbilitySystemComponent* ASC,
	const FGameplayAttribute& Attribute,
	const FGameplayTag& StatTag,
	const FText& DisplayName,
	ENSCharacterStatDisplayType DisplayType) const
{
	if (!IsValid(ASC) || !Attribute.IsValid())
	{
		return;
	}

	FNSCharacterStatViewData StatData;
	StatData.StatTag = StatTag;
	StatData.DisplayName = DisplayName;
	StatData.Value = ASC->GetNumericAttribute(Attribute);
	StatData.DisplayType = DisplayType;

	OutStats.Add(StatData);
}
void UNSCharacterStatsBridgeSubsystem::BindCharacterStats(APlayerController* OwningPlayer)
{
	if (!IsValid(OwningPlayer))
	{
		return;
	}

	const ANSPlayerState* NSPlayerState = OwningPlayer->GetPlayerState<ANSPlayerState>();
	if (!IsValid(NSPlayerState))
	{
		return;
	}

	UAbilitySystemComponent* ASC = NSPlayerState->GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	if (CachedASC.Get() == ASC)
	{
		CachedOwningPlayer = OwningPlayer;
		return;
	}

	UnbindCharacterStats();

	CachedOwningPlayer = OwningPlayer;
	CachedASC = ASC;

	auto AddObservedAttribute = [this, ASC](const FGameplayAttribute& Attribute)
	{
		FNSObservedCharacterStatAttribute ObservedAttribute;
		ObservedAttribute.Attribute = Attribute;
		ObservedAttribute.Handle =
			ASC->GetGameplayAttributeValueChangeDelegate(Attribute)
			.AddUObject(
				this,
				&ThisClass::HandleObservedAttributeChanged);

		ObservedAttributes.Add(ObservedAttribute);
	};
	AddObservedAttribute(UNSBaseAttributeSet::GetMaxHealthAttribute());
	AddObservedAttribute(UNSPlayerAttributeSet::GetMaxShieldAttribute());
	AddObservedAttribute(UNSPlayerAttributeSet::GetShieldRechargeRateAttribute());
	AddObservedAttribute(UNSPlayerAttributeSet::GetShieldRechargeCooldownAttribute());
	AddObservedAttribute(UNSBaseAttributeSet::GetBaseDamageAttribute());
	AddObservedAttribute(UNSBaseAttributeSet::GetDefenseAttribute());
	AddObservedAttribute(UNSBaseAttributeSet::GetMoveSpeedAttribute());
	AddObservedAttribute(UNSPlayerAttributeSet::GetCritChanceAttribute());
	AddObservedAttribute(UNSPlayerAttributeSet::GetCritDamageAttribute());
}

void UNSCharacterStatsBridgeSubsystem::UnbindCharacterStats()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		for (const FNSObservedCharacterStatAttribute& ObservedAttribute : ObservedAttributes)
		{
			if (ObservedAttribute.Attribute.IsValid() && ObservedAttribute.Handle.IsValid())
			{
				ASC->GetGameplayAttributeValueChangeDelegate(
					ObservedAttribute.Attribute)
					.Remove(ObservedAttribute.Handle);
			}
		}
	}

	ObservedAttributes.Empty();
	CachedASC.Reset();
	CachedOwningPlayer.Reset();
}

void UNSCharacterStatsBridgeSubsystem::HandleObservedAttributeChanged(const FOnAttributeChangeData& Data)
{
	BroadcastCachedCharacterStats();
}

void UNSCharacterStatsBridgeSubsystem::BroadcastCachedCharacterStats()
{
	APlayerController* OwningPlayer = CachedOwningPlayer.Get();
	if (!IsValid(OwningPlayer))
	{
		return;
	}

	BroadcastCharacterStats(OwningPlayer);
}
