// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "NSRewardDataRegistry.generated.h"

class UNSRewardTriggerData;

/**
 * 런타임에 보상 데이터를 조회하는 용도의 Registry
 */
UCLASS()
class NEOSANCTUM_API UNSRewardDataRegistry : public UObject
{
	GENERATED_BODY()
	
public:
	void Build(const TArray<UNSRewardTriggerData*>& InRewardTriggerDataList);
	
	void Reset();
	
	const UNSRewardTriggerData* FindRewardTriggerDataByTag(const FGameplayTag& TriggerTag) const;
	
	bool HasRewardTriggerData(const FGameplayTag& TriggerTag) const;
	
private:
	// Registry는 데이터 소유자가 아니므로, 로드된 RewardTriggerData를 약한 참조로 색인
	UPROPERTY(Transient)
	TMap<FGameplayTag, TWeakObjectPtr<UNSRewardTriggerData>> RewardTriggerDataMap;
};
