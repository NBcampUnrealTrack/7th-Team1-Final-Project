// Copyright 2026 One Team. All rights reserved.


#include "NSCharacterStatsWidget.h"
#include "Components/TextBlock.h"
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

void UNSCharacterStatsWidget::HandleCharacterStatsSnapshot(FGameplayTag Channel,
	const FNSCharacterStatsSnapshotMessage& Message)
{
	if (!StatsText)
	{
		return;
	}
	
	FString Result;
	
	for (const FNSCharacterStatViewData& Stat : Message.Stats)
	{
		switch (Stat.DisplayType)
		{
		case ENSCharacterStatDisplayType::Percent:
			Result += FString::Printf(
				TEXT("%s : %.0f%%\n"),
				*Stat.DisplayName.ToString(),
				Stat.Value);
			break;

		case ENSCharacterStatDisplayType::Seconds:
			Result += FString::Printf(
				TEXT("%s : %.1f초\n"),
				*Stat.DisplayName.ToString(),
				Stat.Value);
			break;

		case ENSCharacterStatDisplayType::Number:
		default:
			Result += FString::Printf(
				TEXT("%s : %.0f\n"),
				*Stat.DisplayName.ToString(),
				Stat.Value);
			break;
		}
	}
	
	StatsText->SetText(FText::FromString(Result));
}
