// Copyright 2026 One Team. All rights reserved.


#include "NSRewardHandler.h"

#include "NeoSanctum/Data/Reward/NSRewardDataRegistry.h"
#include "NeoSanctum/Data/Reward/NSRewardDropResolver.h"
#include "NeoSanctum/Data/Reward/NSRewardTriggerData.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Progression/Part/NSDroppedPart.h"
#include "NeoSanctum/System/Subsystem/NSCurrencyDropSubsystem.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_Reward.h"

void UNSRewardHandler::HandleRewardTrigger(
	UWorld* World,
	const UNSRewardDataRegistry* RewardDataRegistry,
	const FGameplayTag& TriggerTag,
	const FVector& DropLocation,
	FRandomStream& RandomStream,
	TSubclassOf<ANSDroppedPart> DroppedPartClass,
	float CurrencyDropDuration)
{
	if (!World)
	{
		NS_LOG(LogNS, Warning, "Reward 처리에 필요한 World가 유효하지 않습니다.");
		return;
	}
	
	if (World->GetNetMode() == NM_Client)
	{
		return;
	}
	
	if (!RewardDataRegistry)
	{
		NS_LOG(LogNS, Warning,
			"RewardDataRegistry가 유효하지 않습니다. TriggerTag={TriggerTag}",
			("TriggerTag", TriggerTag.ToString())
		);
		return;
	}
	
	if (!TriggerTag.IsValid())
	{
		NS_LOG(LogNS, Warning, "Reward TriggerTag가 유효하지 않습니다.");
		return;
	}
	
	const UNSRewardTriggerData* RewardTriggerData = RewardDataRegistry->FindRewardTriggerDataByTag(TriggerTag);
	
	if (!RewardTriggerData)
	{
		NS_LOG(LogNS, Warning,
			"RewardTriggerData를 찾을 수 없습니다. TriggerTag={TriggerTag}",
			("TriggerTag", TriggerTag.ToString())
		);
		return;
	}
	
	HandleRewardEntries(*RewardTriggerData, TriggerTag);
	
	if (RewardTriggerData->DropTable.IsNull())
	{
		return;
	}
	
	const UDataTable* DropTable = RewardTriggerData->DropTable.Get();
	
	if (!DropTable)
	{
		NS_LOG(LogNS, Warning,
			"RewardTriggerData의 DropTable이 로드되어 있지 않습니다. TriggerTag={TriggerTag}, Asset={Asset}",
			("TriggerTag", TriggerTag.ToString()),
			("Asset", GetNameSafe(RewardTriggerData))
		);
		return;
	}
	
	TArray<FNSRewardDropResult> DropResults;
	UNSRewardDropResolver::ResolveDropResultsFromTable(DropTable, RandomStream, DropResults);
	
	HandleDropResults(
		World,
		DropResults,
		DropLocation,
		RandomStream,
		DroppedPartClass,
		CurrencyDropDuration
	);
}

void UNSRewardHandler::HandleRewardEntries(
	const UNSRewardTriggerData& RewardTriggerData, const FGameplayTag& TriggerTag)
{
	for (const FNSRewardEntry& RewardEntry : RewardTriggerData.RewardEntries)
	{
		if (!RewardEntry.RewardTypeTag.IsValid())
		{
			continue;
		}
		
		if (RewardEntry.RewardTypeTag == NSGameplayTags::Reward_Type_Augment)
		{
			NS_LOG(LogNS, Log,
				"Augment RewardEntry가 감지되었습니다. TriggerTag={TriggerTag}, AugmentPool={AugmentPool}",
				("TriggerTag", TriggerTag.ToString()),
				("AugmentPool", RewardEntry.AugmentPool.ToSoftObjectPath().ToString())
			);
			continue;
		}
		
		NS_LOG(LogNS, Warning,
			"아직 RewardEntry 처리가 연결되지 않은 RewardType입니다. TriggerTag={TriggerTag}, RewardType={RewardType}",
			("TriggerTag", TriggerTag.ToString()),
			("RewardType", RewardEntry.RewardTypeTag.ToString())
		);
	}
}

void UNSRewardHandler::HandleDropResults(
	UWorld* World,
	const TArray<FNSRewardDropResult>& DropResults,
	const FVector& DropLocation,
	FRandomStream& RandomStream,
	TSubclassOf<ANSDroppedPart> DroppedPartClass,
	float CurrencyDropDuration)
{
	if (DropResults.IsEmpty())
	{
		NS_LOG(LogNS, Log, "RewardDropResult가 비어 있습니다. 이번 판정에서는 드랍이 없습니다.");
		return;
	}
	
	for (const FNSRewardDropResult& DropResult : DropResults)
	{
		if (DropResult.RewardTypeTag == NSGameplayTags::Reward_Type_Currency)
		{
			HandleCurrencyDropResult(
				World,
				DropResult,
				DropLocation,
				CurrencyDropDuration
			);
			continue;
		}
		
		if (DropResult.RewardTypeTag == NSGameplayTags::Reward_Type_Part)
		{
			HandlePartDropResult(
				World,
				DropResult,
				DropLocation,
				RandomStream,
				DroppedPartClass
			);
			continue;
		}
		
		if (DropResult.RewardTypeTag == NSGameplayTags::Reward_Type_Augment)
		{
			HandleAugmentDropResult(DropResult);
			continue;
		}
		
		NS_LOG(LogNS, Warning,
			"처리되지 않은 RewardDropResult입니다. RewardType={RewardType}",
			("RewardType", DropResult.RewardTypeTag.ToString())
		);
	}
}

void UNSRewardHandler::HandleCurrencyDropResult(
	UWorld* World,
	const FNSRewardDropResult& DropResult,
	const FVector& DropLocation,
	float CurrencyDropDuration)
{
	if (!World)
	{
		return;
	}
	
	if (!DropResult.CurrencyTag.IsValid())
	{
		NS_LOG(LogNS, Warning,
			"Currency RewardResult의 CurrencyTag가 유효하지 않습니다. RewardType={RewardType}",
			("RewardType", DropResult.RewardTypeTag.ToString())
		);
		return;
	}
	
	if (DropResult.Quantity <= 0)
	{
		NS_LOG(LogNS, Warning,
			"Currency RewardResult의 Quantity가 0 이하입니다. Currency={Currency}, Quantity={Quantity}",
			("Currency", DropResult.CurrencyTag.ToString()),
			("Quantity", DropResult.Quantity)
		);
		return;
	}
	
	UNSCurrencyDropSubsystem* CurrencyDropSubsystem = World->GetSubsystem<UNSCurrencyDropSubsystem>();
	
	if (!CurrencyDropSubsystem)
	{
		NS_LOG(LogNS, Warning,
			"CurrencyDropSubsystem을 찾을 수 없습니다. Currency={Currency}",
			("Currency", DropResult.CurrencyTag.ToString())
		);
		return;
	}
	
	// TODO: @원종 임시로 CurrenyGrade를 None으로 테스트
	const int32 DropId = CurrencyDropSubsystem->RegisterDrop(
		DropResult.CurrencyTag,
		ENSCurrencyGrade::None,
		static_cast<int64>(DropResult.Quantity),
		DropLocation,
		CurrencyDropDuration
	);
	
	if (DropId == INDEX_NONE)
	{
		NS_LOG(LogNS, Warning,
			"Currency 드랍 등록에 실패했습니다. Currency={Currency}, Quantity={Quantity}",
			("Currency", DropResult.CurrencyTag.ToString()),
			("Quantity", DropResult.Quantity)
		);
		return;
	}
	
	NS_LOG(LogNS, Log,
		"Currency 드랍을 등록했습니다. DropId={DropId}, Currency={Currency}, Quantity={Quantity}",
		("DropId", DropId),
		("Currency", DropResult.CurrencyTag.ToString()),
		("Quantity", DropResult.Quantity)
	);
}

void UNSRewardHandler::HandlePartDropResult(
	UWorld* World,
	const FNSRewardDropResult& DropResult,
	const FVector& DropLocation,
	FRandomStream& RandomStream,
	TSubclassOf<ANSDroppedPart> DroppedPartClass)
{
	if (!World)
	{
		return;
	}
	
	if (DropResult.PartDefinition.IsNull())
	{
		NS_LOG(LogNS, Warning, "Part RewardResult의 PartDefinition이 비어 있습니다.");
		return;
	}
	
	if (DropResult.Quantity <= 0)
	{
		NS_LOG(LogNS, Warning,
			"Part RewardResult의 Quantity가 0 이하입니다. PartDefinition={PartDefinition}, Quantity={Quantity}",
			("PartDefinition", DropResult.PartDefinition.ToSoftObjectPath().ToString()),
			("Quantity", DropResult.Quantity)
		);
		return;
	}
	
	for (int32 SpawnIndex = 0; SpawnIndex < DropResult.Quantity; ++SpawnIndex)
	{
		FNSPartData PartData = MakePartDataFromDropResult(DropResult, RandomStream);
		
		if (!PartData.IsValid())
		{
			NS_LOG(LogNS, Warning,
				"Part RewardResult에서 유효한 FNSPartData를 만들 수 없습니다. PartDefinition={PartDefinition}",
				("PartDefinition", DropResult.PartDefinition.ToSoftObjectPath().ToString())
			);
			continue;
		}
		
		ANSDroppedPart* DroppedPart = ANSDroppedPart::SpawnInWorld(
			World,
			DroppedPartClass,
			PartData,
			DropLocation
		);
		
		if (!DroppedPart)
		{
			NS_LOG(LogNS, Warning,
				"Part 드랍 액터 생성에 실패했습니다. PartDefinition={PartDefinition}",
				("PartDefinition", DropResult.PartDefinition.ToSoftObjectPath().ToString())
			);
			continue;
		}
		
		NS_LOG(LogNS, Log,
			"Part 드랍 액터를 생성했습니다. PartDefinition={PartDefinition}, Rarity={Rarity}, Value={Value}",
			("PartDefinition", DropResult.PartDefinition.ToSoftObjectPath().ToString()),
			("Rarity", static_cast<int32>(PartData.CurrentRarity)),
			("Value", PartData.CurrentValue)
		);
	}
}

void UNSRewardHandler::HandleAugmentDropResult(const FNSRewardDropResult& DropResult)
{
	NS_LOG(LogNS, Log,
		"Augment DropResult가 감지되었습니다. AugmentPoolTag={AugmentPoolTag}",
		("AugmentPoolTag", DropResult.AugmentPoolTag.ToString())
	);
}

FNSPartData UNSRewardHandler::MakePartDataFromDropResult(
	const FNSRewardDropResult& DropResult, FRandomStream& RandomStream)
{
	FNSPartData PartData;
	
	if (DropResult.PartDefinition.IsNull())
	{
		return PartData;
	}
	
	// MVP: 드랍 시점에 수치 산출을 위해 Definition을 확인
	// TODO: @원종 후속 작업에서 PartDefinition 선로딩이 보장되면 Get() 기반으로 변경
	UNSPartDefinition* PartDefinition = DropResult.PartDefinition.LoadSynchronous();
	
	if (!PartDefinition)
	{
		return PartData;
	}
	
	PartData.DefinitionPtr = DropResult.PartDefinition;
	PartData.Slot = PartDefinition->PartSlot;
	PartData.CurrentRarity = ENSPartRarity::Common;
	PartData.RollCount = 0;
	
	const FNSPartValueRange* ValueRange = PartDefinition->ValueRange.Find(PartData.CurrentRarity);
	
	if (ValueRange)
	{
		const float MinValue = FMath::Min(ValueRange->Min, ValueRange->Max);
		const float MaxValue = FMath::Max(ValueRange->Min, ValueRange->Max);
		PartData.CurrentValue = RandomStream.FRandRange(MinValue, MaxValue);
	}
	
	return PartData;
}
