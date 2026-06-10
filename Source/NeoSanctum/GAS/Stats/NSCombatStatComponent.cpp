// Copyright 2026 One Team. All rights reserved.


#include "NSCombatStatComponent.h"

#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"


UNSCombatStatComponent::UNSCombatStatComponent()
{

	PrimaryComponentTick.bCanEverTick = false;
}

void UNSCombatStatComponent::BeginPlay()
{
	Super::BeginPlay();

	RebuildBaseStatCache();
}

void UNSCombatStatComponent::RebuildBaseStatCache()
{
	CachedBaseStatsByAbility.Reset();
	
	if (!AbilityBaseStatTable)
	{
		NS_OBJ_LOG(LogNSGAS, Warning, "스킬 기본 스탯 DataTable이 설정되지 않았습니다.");
		
		return;
	}
	
	if (AbilityBaseStatTable->GetRowStruct() != FNSAbilityBaseStatRow::StaticStruct())
	{
		NS_OBJ_LOG(LogNSGAS, Warning,
			"스킬 기본 스탯 DataTable의 Row Struct가 올바르지 않습니다. Table={Table}",
			("Table", AbilityBaseStatTable->GetName())
		);
		
		return;
	}
	
	const FString ContextString = TEXT("AbilityBaseStatCache");
	
	for (const FName& RowName : AbilityBaseStatTable->GetRowNames())
	{
		const FNSAbilityBaseStatRow* Row = 
			AbilityBaseStatTable->FindRow<FNSAbilityBaseStatRow>(RowName, ContextString, false);
		
		if (!Row)
		{
			continue;
		}
		
		if (!Row->AbilityTag.IsValid() || !Row->StatTag.IsValid())
		{
			NS_OBJ_LOG(LogNSGAS, Warning,
				"유효하지 않은 스킬 스탯 Row입니다. RowName={RowName}, AbilityTag={AbilityTag}, StatTag={StatTag}",
				("RowName", RowName.ToString()),
				("AbilityTag", Row->AbilityTag.ToString()),
				("StatTag", Row->StatTag.ToString())
			);

			continue;
		}
		
		TMap<FGameplayTag, FNSCachedAbilityBaseStat>& StatMap = 
			CachedBaseStatsByAbility.FindOrAdd(Row->AbilityTag);
		
		if (StatMap.Contains(Row->StatTag))
		{
			// 같은 AbilityTag + StatTag 조합은 하나의 기본값만 허용
			NS_OBJ_LOG(LogNSGAS, Warning,
				"중복된 스킬 기본 스탯 Row입니다. RowName={RowName}, AbilityTag={AbilityTag}, StatTag={StatTag}",
				("RowName", RowName.ToString()),
				("AbilityTag", Row->AbilityTag.ToString()),
				("StatTag", Row->StatTag.ToString())
			);

			continue;
		}
		
		FNSCachedAbilityBaseStat CachedStat;
		CachedStat.BaseValue = Row->BaseValue;
		CachedStat.bModifiable = Row->bModifiable;
		
		StatMap.Add(Row->StatTag, CachedStat);
	}
	
	NS_OBJ_LOG(LogNSGAS, Log,
		"스킬 기본 스탯 캐시 생성 완료. AbilityCount={AbilityCount}",
		("AbilityCount", CachedBaseStatsByAbility.Num())
	);
}

bool UNSCombatStatComponent::TryGetBaseAbilityStat(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag,
	float& OutValue) const
{
	const TMap<FGameplayTag, FNSCachedAbilityBaseStat>* StatMap = CachedBaseStatsByAbility.Find(AbilityTag);
	
	if (!StatMap)
	{
		return false;
	}
	
	const FNSCachedAbilityBaseStat* CachedStat = StatMap->Find(StatTag);
	
	if (!CachedStat)
	{
		return false;
	}
	
	OutValue = CachedStat->BaseValue;
	return true;
}

bool UNSCombatStatComponent::IsAbilityStatModifiable(
	const FGameplayTag& AbilityTag, const FGameplayTag& StatTag) const
{
	const TMap<FGameplayTag, FNSCachedAbilityBaseStat>* StatMap = CachedBaseStatsByAbility.Find(AbilityTag);
	
	if (!StatMap)
	{
		return false;
	}
	
	const FNSCachedAbilityBaseStat* CachedStat = StatMap->Find(StatTag);
	
	if (!CachedStat)
	{
		return false;
	}
	
	return CachedStat->bModifiable;
}
