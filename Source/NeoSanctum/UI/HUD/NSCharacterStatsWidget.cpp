// Copyright 2026 One Team. All rights reserved.

#include "NSCharacterStatsWidget.h"
#include "Components/TextBlock.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"

void UNSCharacterStatsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	StatsSnapshotListenerHandle =
		UGameplayMessageSubsystem::Get(this)
		.RegisterListener<FNSCharacterStatsSnapshotMessage>(
			NSGameplayTags::Message_UI_CharacterStats_Snapshot,
			this,
			&ThisClass::HandleCharacterStatsSnapshot);
}

void UNSCharacterStatsWidget::NativeDestruct()
{
	UGameplayMessageSubsystem::Get(this).UnregisterListener(
		StatsSnapshotListenerHandle);

	Super::NativeDestruct();
}

void UNSCharacterStatsWidget::HandleCharacterStatsSnapshot(
	FGameplayTag Channel,
	const FNSCharacterStatsSnapshotMessage& Message)
{
	for (const FNSCharacterStatViewData& Stat : Message.Stats)
	{
		if (Stat.StatTag == NSGameplayTags::CombatStat_MaxHealth)
		{
			ApplyStatToWidgets(Stat, CT_MaxHealthLabel, MaxHealthText);
		}
		else if (Stat.StatTag == NSGameplayTags::CombatStat_MaxShield)
		{
			ApplyStatToWidgets(Stat, CT_MaxShieldLabel, MaxShieldText);
		}
		else if (Stat.StatTag == NSGameplayTags::CombatStat_ShieldRechargeRate)
		{
			ApplyStatToWidgets(
				Stat,
				CT_ShieldRegenRateLabel,
				ShieldRegenRateText);
		}
		else if (Stat.StatTag == NSGameplayTags::CombatStat_ShieldRechargeCooldown)
		{
			ApplyStatToWidgets(
				Stat,
				CT_ShieldRegenCooldownLabel,
				ShieldRegenCooldownText);
		}
		else if (Stat.StatTag == NSGameplayTags::CombatStat_Damage)
		{
			ApplyStatToWidgets(Stat, CT_BaseDamageLabel, BaseDamageText);
		}
		else if (Stat.StatTag == NSGameplayTags::CombatStat_Defense)
		{
			ApplyStatToWidgets(Stat, CT_DefenseLabel, DefenseText);
		}
		else if (Stat.StatTag == NSGameplayTags::CombatStat_MoveSpeed)
		{
			ApplyStatToWidgets(Stat, CT_MoveSpeedLabel, MoveSpeedText);
		}
		else if (Stat.StatTag == NSGameplayTags::CombatStat_CritChance)
		{
			ApplyStatToWidgets(Stat, CT_CritChanceLabel, CritChanceText);
		}
		else if (Stat.StatTag == NSGameplayTags::CombatStat_CritDamage)
		{
			ApplyStatToWidgets(Stat, CT_CritDamageLabel, CritDamageText);
		}
	}
}

void UNSCharacterStatsWidget::ApplyStatToWidgets(
	const FNSCharacterStatViewData& Stat,
	UTextBlock* LabelWidget,
	UTextBlock* ValueWidget) const
{
	if (LabelWidget)
	{
		LabelWidget->SetText(Stat.DisplayName);
	}

	if (ValueWidget)
	{
		ValueWidget->SetText(FormatStatValue(Stat.Value, Stat.DisplayType));
	}
}

FText UNSCharacterStatsWidget::FormatStatValue(
	float Value,
	ENSCharacterStatDisplayType DisplayType) const
{
	switch (DisplayType)
	{
	case ENSCharacterStatDisplayType::Percent:
		return FText::Format(
			NSLOCTEXT("CharacterStats", "PercentFormat", "{0}%"),
			FText::AsNumber(FMath::RoundToInt(Value)));

	case ENSCharacterStatDisplayType::Seconds:
	{
		FNumberFormattingOptions FormattingOptions;
		FormattingOptions.MinimumFractionalDigits = 1;
		FormattingOptions.MaximumFractionalDigits = 1;

		return FText::Format(
			NSLOCTEXT("CharacterStats", "SecondsFormat", "{0}\uCD08"),
			FText::AsNumber(Value, &FormattingOptions));
	}

	case ENSCharacterStatDisplayType::Number:
	default:
		return FText::AsNumber(FMath::RoundToInt(Value));
	}
}
