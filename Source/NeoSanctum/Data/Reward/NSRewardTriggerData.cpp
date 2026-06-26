// Copyright 2026 One Team. All rights reserved.

#include "NSRewardTriggerData.h"

#include "Engine/DataTable.h"
#include "Misc/DataValidation.h"

FPrimaryAssetId UNSRewardTriggerData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("NSRewardTriggerData"), GetFName());
}

#if WITH_EDITOR
EDataValidationResult UNSRewardTriggerData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	
	if (!TriggerTag.IsValid())
	{
		Context.AddError(FText::FromString(TEXT("TriggerTag가 비어 있습니다.")));
		Result = EDataValidationResult::Invalid;
	}
	
	if (RewardEntries.IsEmpty() && DropTable.IsNull())
	{
		Context.AddWarning(FText::FromString(TEXT("RewardEntries와 DropTable이 모두 비어 있습니다.")));
	}
	
	for (int32 EntryIndex = 0; EntryIndex < RewardEntries.Num(); ++EntryIndex)
	{
		const FNSRewardEntry& Entry = RewardEntries[EntryIndex];
		
		if (!Entry.RewardTypeTag.IsValid())
		{
			Context.AddError(FText::Format(
				FText::FromString(TEXT("RewardEntries[{0}]의 RewardTypeTag가 비어 있습니다.")),
				FText::AsNumber(EntryIndex))
			);
			Result = EDataValidationResult::Invalid;
		}
		
		if (Entry.Weight <= 0.0f)
		{
			Context.AddError(FText::Format(
				FText::FromString(TEXT("RewardEntries[{0}]의 Weight는 0보다 커야 합니다.")),
				FText::AsNumber(EntryIndex))
			);
			Result = EDataValidationResult::Invalid;
		}
	}
	
	if (!DropTable.IsNull())
	{
		const UDataTable* LoadedDropTable = DropTable.LoadSynchronous();
		
		if (!IsValid(LoadedDropTable))
		{
			Context.AddError(FText::FromString(TEXT("DropTable을 로드할 수 없습니다.")));
			Result = EDataValidationResult::Invalid;
		}
		else if (LoadedDropTable->GetRowStruct() != FNSRewardDropRow::StaticStruct())
		{
			Context.AddError(FText::FromString(TEXT("DropTable의 RowStruct가 FNSRewardDropRow가 아닙니다.")));
			Result = EDataValidationResult::Invalid;
		}
	}	
	
	return Result;
}
#endif
