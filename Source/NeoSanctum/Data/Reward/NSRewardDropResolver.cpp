// Copyright 2026 One Team. All rights reserved.


#include "NSRewardDropResolver.h"

#include "NSRewardTypes.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Tag/NSGameplayTags_Reward.h"

void UNSRewardDropResolver::ResolveDropResultsFromTable(
	const UDataTable* DropTable,
	FRandomStream& RandomStream,
	TArray<FNSRewardDropResult>& OutResults)
{
	OutResults.Reset();
	
	if (!DropTable)
	{
		NS_LOG(LogNS, Warning, "DropTable이 유효하지 않습니다.");
		return;
	}
	
	if (DropTable->GetRowStruct() != FNSRewardDropRow::StaticStruct())
	{
		NS_LOG(LogNS, Warning,
				"DropTable의 RowStruct가 FNSRewardDropRow가 아닙니다. DropTable={DropTable}",
				("DropTable", GetNameSafe(DropTable))
		);
		return;
	}
	
	TArray<FNSRewardDropRow*> Rows;
	// 모든 행을 읽어 Rows에 추가
	DropTable->GetAllRows<FNSRewardDropRow>(
		TEXT("UNSRewardDropResolver::ResolveDropResultsFromTable"), Rows);
	
	TMap<FGameplayTag, TArray<const FNSRewardDropRow*>> RowsByGroup;
	
	for (const FNSRewardDropRow* Row : Rows)
	{
		if (!Row)
		{
			continue;
		}
		
		if (!IsValidDropRow(*Row))
		{
			continue;
		}
		
		// 재화와 파츠가 서로 확률을 경쟁하지 않도록 DropGroupTag별로 독립 Roll 대상으로 분리
		RowsByGroup.FindOrAdd(Row->DropGroupTag).Add(Row);
	}
	
	if (RowsByGroup.IsEmpty())
	{
		NS_LOG(LogNS, Warning,
			"유효한 보상 드랍 그룹이 없습니다. DropTable={DropTable}",
			("DropTable", GetNameSafe(DropTable))
		);
		return;
	}
	
	for (const TPair<FGameplayTag, TArray<const FNSRewardDropRow*>>& GroupPair : RowsByGroup)
	{
		const FNSRewardDropRow* SelectedRow = SelectDropRow(GroupPair.Value, RandomStream);
		
		if (!SelectedRow)
		{
			continue;
		}
		
		FNSRewardDropResult Result;
		ApplyDropRowToResult(*SelectedRow, RandomStream, Result);
		
		if (Result.RewardTypeTag == NSGameplayTags::Reward_Type_None)
		{
			// None은 실패가 아니라 해당 그룹에서 드랍이 없다는 정상 결과
			continue;
		}
		
		OutResults.Add(Result);
	}
}

bool UNSRewardDropResolver::IsValidDropRow(const FNSRewardDropRow& Row)
{
	if (!Row.DropGroupTag.IsValid())
	{
		return false;
	}
	
	if (!Row.RewardTypeTag.IsValid())
	{
		return false;
	}
	
	if (Row.Weight <= 0.0f)
	{
		return false;
	}
	
	return true;
}

const FNSRewardDropRow* UNSRewardDropResolver::SelectDropRow(
	const TArray<const FNSRewardDropRow*>& Rows, FRandomStream& RandomStream)
{
	int32 TotalWeight = 0.0f;
	
	for (const FNSRewardDropRow* Row : Rows)
	{
		if (!Row)
		{
			continue;
		}
		
		TotalWeight += Row->Weight;
	}
	
	if (TotalWeight <= 0.0f)
	{
		return nullptr;
	}
	
	const int32 RollValue = RandomStream.RandRange(1, TotalWeight);
	float AccumulatedWeight = 0.0f;
	
	for (const FNSRewardDropRow* Row : Rows)
	{
		if (!Row)
		{
			continue;
		}
		
		AccumulatedWeight += Row->Weight;
		
		if (RollValue <= AccumulatedWeight)
		{
			return Row;
		}
	}
	
	// 부동소수점 오차로 선택되지 않았을 때 마지막 유효 Row 사용
	for (int32 RowIndex = Rows.Num() - 1; RowIndex >= 0; --RowIndex)
	{
		const FNSRewardDropRow* FallbackRow = Rows[RowIndex];
		
		if (FallbackRow)
		{
			return FallbackRow;
		}
	}
	
	return nullptr;
}

void UNSRewardDropResolver::ApplyDropRowToResult(
	const FNSRewardDropRow& Row,
	FRandomStream& RandomStream,
	FNSRewardDropResult& OutResult)
{
	OutResult.DropGroupTag = Row.DropGroupTag;
	OutResult.RewardTypeTag = Row.RewardTypeTag;
	OutResult.CurrencyTag = Row.CurrencyTag;
	OutResult.PartDefinition = Row.PartDefinition;
	OutResult.AugmentPoolTag = Row.AugmentPoolTag;
	
	if (Row.RewardTypeTag == NSGameplayTags::Reward_Type_None)
	{
		OutResult.Quantity = 0;
		return;
	}
	
	const int32 MinQuantity = FMath::Max(0, Row.MinQuantity);
	const int32 MaxQuantity = FMath::Max(MinQuantity, Row.MaxQuantity);
	
	OutResult.Quantity = RandomStream.RandRange(MinQuantity, MaxQuantity);
}
