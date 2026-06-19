// Copyright 2026 One Team. All rights reserved.

#include "NSRewardDataRegistry.h"

#include "NSRewardTriggerData.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"

void UNSRewardDataRegistry::Build(const TArray<UNSRewardTriggerData*>& InRewardTriggerDataList)
{
	// Run 데이터가 다시 로드될 수 있으므로 기존 색인은 항상 새로 구성
	Reset();
	
	for (UNSRewardTriggerData* RewardTriggerData : InRewardTriggerDataList)
	{
		if (!IsValid(RewardTriggerData))
		{
			continue;
		}
		
		const FGameplayTag TriggerTag = RewardTriggerData->TriggerTag;
		
		if (!TriggerTag.IsValid())
		{
			NS_OBJ_LOG(LogNS, Warning,
				"RewardTriggerData의 TriggerTag가 유효하지 않습니다. Asset={Asset}",
				("Asset", GetNameSafe(RewardTriggerData))
			);
			continue;
		}
		
		const TWeakObjectPtr<UNSRewardTriggerData>* ExistingDataPtr = RewardTriggerDataMap.Find(TriggerTag);
		
		if (ExistingDataPtr)
		{
			const UNSRewardTriggerData* ExistingData = ExistingDataPtr->Get();
			
			// 중복 TriggerTag는 먼저 등록된 데이터를 유지하고 이후 데이터는 무시
			NS_OBJ_LOG(LogNS, Warning,
				"중복된 Reward TriggerTag가 감지되었습니다. TriggerTag={TriggerTag}, 기존 데이터={ExistingData}, 무시된 데이터={IgnoredData}",
				("TriggerTag", TriggerTag.ToString()),
				("ExistingData", GetNameSafe(ExistingData)),
				("IgnoredData", GetNameSafe(RewardTriggerData))
			);
			continue;
		}
		
		// 데이터의 생명주기는 DataSubsystem/AssetManager가 관리하고 Registry는 조회용 약한 참조만 보관
		RewardTriggerDataMap.Add(TriggerTag, RewardTriggerData);
	}
}

void UNSRewardDataRegistry::Reset()
{
	RewardTriggerDataMap.Reset();
}

const UNSRewardTriggerData* UNSRewardDataRegistry::FindRewardTriggerDataByTag(const FGameplayTag& TriggerTag) const
{
	if (!TriggerTag.IsValid())
	{
		return nullptr;
	}
	
	const TWeakObjectPtr<UNSRewardTriggerData>* FoundData = RewardTriggerDataMap.Find(TriggerTag);
	
	// Run 데이터 언로드 이후에는 WeakObjectPtr이 만료될 수 있으므로 조회 시점에 유효성 확인
	if (!FoundData || !FoundData->IsValid())
	{
		return nullptr;
	}
	
	return FoundData->Get();
}

bool UNSRewardDataRegistry::HasRewardTriggerData(const FGameplayTag& TriggerTag) const
{
	return FindRewardTriggerDataByTag(TriggerTag) != nullptr;
}
