// Copyright 2026 One Team. All rights reserved.


#include "NSAugmentDisplayBridgeSubsystem.h"
#include "Engine/DataTable.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/Augment/NSAugmentDefinition.h"

bool UNSAugmentDisplayBridgeSubsystem::TryBuildCardViewData(
	const FPrimaryAssetId& DefId,
	ENSAugmentRarity Rarity,
	int32 CurrentStack,
	FNSAugmentCardViewData& OutViewData
) const
{
	OutViewData = FNSAugmentCardViewData();

	if (!DefId.IsValid())
	{
		return false;
	}

	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem || !DataSubsystem->IsRunReady())
	{
		return false;
	}

	const UNSAugmentDefinition* Definition =
		DataSubsystem->GetData<UNSAugmentDefinition>(DefId);

	if (!Definition)
	{
		return false;
	}

	const UDataTable* DefinitionTable =
		DataSubsystem->GetCurrentAugmentDefinitionTable();

	TArray<const FNSAugmentDefinitionRow*> DefinitionRows;
	if (!TryCollectDefinitionRows(
		DefinitionTable,
		Definition,
		DefinitionRows))
	{
		return false;
	}

	const FNSAugmentDefinitionRow* PrimaryRow = DefinitionRows[0];

	OutViewData.DefId = DefId;
	OutViewData.AugmentTag = PrimaryRow->AugmentTag;
	OutViewData.DisplayName = Definition->DisplayName;
	OutViewData.Description = FormatDescription(
		Definition->Description,
		DefinitionRows);
	OutViewData.Icon = Definition->Icon;
	OutViewData.Rarity = Rarity;
	OutViewData.CurrentStack = FMath::Max(0, CurrentStack);
	OutViewData.MaxStack = FMath::Max(1, PrimaryRow->MaxStack);

	return true;
}

bool UNSAugmentDisplayBridgeSubsystem::TryCollectDefinitionRows(
	const UDataTable* DefinitionTable,
	const UNSAugmentDefinition* Definition,
	TArray<const FNSAugmentDefinitionRow*>& OutRows
	) const
{
	OutRows.Reset();

	if (!DefinitionTable || !Definition)
	{
		return false;
	}

	if (DefinitionTable->GetRowStruct() !=
		FNSAugmentDefinitionRow::StaticStruct())
	{
		return false;
	}

	const FString ContextString =
		TEXT("AugmentDisplayBridge");

	const FSoftObjectPath DefinitionPath(Definition);

	for (const FName& RowName : DefinitionTable->GetRowNames())
	{
		const FNSAugmentDefinitionRow* Row =
			DefinitionTable->FindRow<FNSAugmentDefinitionRow>(
				RowName,
				ContextString,
				false);

		if (!Row || !Row->bEnabled)
		{
			continue;
		}

		if (Row->Definition.ToSoftObjectPath() == DefinitionPath)
		{
			OutRows.Add(Row);
		}
	}

	return !OutRows.IsEmpty();
}

FText UNSAugmentDisplayBridgeSubsystem::FormatDescription(
	const FText& DescriptionFormat,
	const TArray<const FNSAugmentDefinitionRow*>& Rows
	) const
{
	if (DescriptionFormat.IsEmpty() || Rows.IsEmpty())
	{
		return DescriptionFormat;
	}

	FFormatNamedArguments Arguments;

	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		const FNSAugmentDefinitionRow* Row = Rows[Index];
		if (!Row)
		{
			continue;
		}

		const FText DisplayValue =
			MakeDisplayValue(Row->ValuePerStack, Row->Operation);

		// 값이 하나인 일반적인 설명에서는 {Value}를 사용합니다.
		if (Index == 0)
		{
			Arguments.Add(TEXT("Value"), DisplayValue);
		}

		// 여러 값이 있는 설명은 {Damage}, {FireRate}처럼 사용할 수 있습니다.
		const FName ArgumentName =
			MakeFormatArgumentName(Row->StatTag);

		if (!ArgumentName.IsNone())
		{
			Arguments.Add(ArgumentName.ToString(), DisplayValue);
		}
	}

	return FText::Format(DescriptionFormat, Arguments);
}

FName UNSAugmentDisplayBridgeSubsystem::MakeFormatArgumentName(
	const FGameplayTag& StatTag)
{
	if (!StatTag.IsValid())
	{
		return NAME_None;
	}

	const FString TagString = StatTag.ToString();

	FString Left;
	FString Right;

	if (TagString.Split(
		TEXT("."),
		&Left,
		&Right,
		ESearchCase::CaseSensitive,
		ESearchDir::FromEnd))
	{
		return FName(*Right);
	}

	return FName(*TagString);
}

FText UNSAugmentDisplayBridgeSubsystem::MakeDisplayValue(
	float ValuePerStack, ENSCombatStatModifierOperation Operation)
{
	FNumberFormattingOptions NumberOptions;
	NumberOptions.MinimumFractionalDigits = 0;
	NumberOptions.MaximumFractionalDigits = 2;
	
	const float AbsoluteValue = FMath::Abs(ValuePerStack);
	
	if (Operation == ENSCombatStatModifierOperation::Multiply)
	{
		return FText::AsPercent(
			AbsoluteValue * 0.01f,
			&NumberOptions);
	}

	return FText::AsNumber(
		AbsoluteValue,
		&NumberOptions);
}

