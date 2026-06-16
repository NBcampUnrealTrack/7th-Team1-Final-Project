// Copyright 2026 One Team. All rights reserved.

#include "NSRewardTriggerData.h"

#include "Misc/DataValidation.h"
#include "NeoSanctum/Tag/NSGameplayTags_Reward.h"

#if WITH_EDITOR
EDataValidationResult UNSRewardTriggerData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	
	if (!TriggerTag.IsValid())
	{
		Context.AddError(FText::FromString(TEXT("TriggerTag가 비어 있습니다.")));
		Result = EDataValidationResult::Invalid;
	}
	
	if (RewardEntries.IsEmpty())
	{
		Context.AddWarning(FText::FromString(TEXT("RewardEntries가 비어 있습니다.")));
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
		
		if (Entry.RewardTypeTag == NSGameplayTags::Reward_Type_Augment && Entry.AugmentPool.IsNull())
		{
			Context.AddError(FText::Format(
				FText::FromString(TEXT("RewardEntries[{0}]는 증강 보상 타입이지만 AugmentPool이 비어 있습니다.")),
				FText::AsNumber(EntryIndex)));
			Result = EDataValidationResult::Invalid;
		}
	}
	
	return Result;
}
#endif
