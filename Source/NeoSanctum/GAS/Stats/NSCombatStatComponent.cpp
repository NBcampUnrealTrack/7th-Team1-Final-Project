// Copyright 2026 One Team. All rights reserved.


#include "NSCombatStatComponent.h"

#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Progression/Augment/NSAugmentInventoryComponent.h"


UNSCombatStatComponent::UNSCombatStatComponent()
{

	PrimaryComponentTick.bCanEverTick = false;
}

void UNSCombatStatComponent::BeginPlay()
{
	Super::BeginPlay();

	RebuildBaseStatCache();
	
	// Modifier DataTable은 먼저 원본 기준으로 캐싱
	RebuildModifierSourceCache();
	
	// AugmentInventory 변경 이벤트를 받아 Active Modifier를 갱신
	BindAugmentInventory();
	
	// 이미 보유 중인 증강이 있다면 현재 상태 기준으로 한 번 갱신
	RebuildActiveModifierCache();
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

bool UNSCombatStatComponent::TryGetFinalAbilityStat(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag,
	float& OutValue) const
{
	float BaseValue = 0.0f;
	
	if (!TryGetBaseAbilityStat(AbilityTag, StatTag, BaseValue))
	{
		return false;
	}
	
	const TMap<FGameplayTag, FNSCombatStatModifierSum>* StatMap = ActiveModifiersByAbility.Find(AbilityTag);
	
	if (!StatMap)
	{
		OutValue = BaseValue;
		return true;
	}
	
	const FNSCombatStatModifierSum* ModifierSum = StatMap->Find(StatTag);
	
	if (!ModifierSum)
	{
		OutValue = BaseValue;
		return true;
	}
	
	// 최종 스탯은 Add 보정을 먼저 더한 뒤 Multiply 보정을 곱함
	OutValue = (BaseValue + ModifierSum->AddValue) * ModifierSum->MultiplyValue;
	return true;
}

void UNSCombatStatComponent::BindAugmentInventory()
{
	ANSPlayerState* NSPlayerState = Cast<ANSPlayerState>(GetOwner());
	
	if (!NSPlayerState)
	{
		return;
	}
	
	UNSAugmentInventoryComponent* AugmentInventory = NSPlayerState->GetAugmentInventory();
	
	if (!AugmentInventory)
	{
		return;
	}
	
	CachedAugmentInventory = AugmentInventory;
	
	// 재초기화 상황에서 중복 바인딩을 방지
	AugmentInventory->OnInventoryChanged.RemoveDynamic(
		this, &UNSCombatStatComponent::HandleAugmentInventoryChanged);
	
	AugmentInventory->OnInventoryChanged.AddDynamic(
		this, &UNSCombatStatComponent::HandleAugmentInventoryChanged);
}

void UNSCombatStatComponent::HandleAugmentInventoryChanged()
{
	NS_OBJ_LOG(LogNSGAS, Log, "AugmentInventory 변경 감지. Active Modifier 캐시를 갱신합.");
	
	// 증강 보유 상태가 바뀌면 최종 계산용 Modifier만 다시 만듬
	RebuildActiveModifierCache();
}

void UNSCombatStatComponent::RebuildModifierSourceCache()
{
	CachedModifierRowsBySource.Reset();
	
	if (!CombatStatModifierTable)
	{
		NS_OBJ_LOG(LogNSGAS, Warning, "CombatStat Modifier DataTable이 설정되지 않았습니다.");
		
		return;
	}
	
	if (CombatStatModifierTable->GetRowStruct() != FNSCombatStatModifierRow::StaticStruct())
	{
		NS_OBJ_LOG(LogNSGAS, Warning,
			"CombatStat Modifier DataTable의 Row Struct가 올바르지 않습니다. Table={Table}",
			("Table", CombatStatModifierTable->GetName())
		);
		
		return;
	}
	
	const FString ContextString = TEXT("CombatStatModifierCache");
	
	for (const FName& RowName : CombatStatModifierTable->GetRowNames())
	{
		const FNSCombatStatModifierRow* Row =
			CombatStatModifierTable->FindRow<FNSCombatStatModifierRow>(RowName, ContextString, false);
		
		if (!Row)
		{
			continue;
		}
		
		if (!Row->bEnabled)
		{
			continue;
		}
		
		if (!Row->SourceDefId.IsValid() || !Row->TargetAbilityTag.IsValid() || !Row->StatTag.IsValid())
		{
			NS_OBJ_LOG(LogNSGAS, Warning,
				"유효하지 않은 CombatStat Modifier Row입니다. RowName={RowName}, SourceDefId={SourceDefId}, AbilityTag={AbilityTag}, StatTag={StatTag}",
				("RowName", RowName.ToString()),
				("SourceDefId", Row->SourceDefId.ToString()),
				("AbilityTag", Row->TargetAbilityTag.ToString()),
				("StatTag", Row->StatTag.ToString())
			);
			
			continue;
		}
		
		// SourceDefId로 보유 증강과 빠르게 매칭하기 위한 원본 캐시
		CachedModifierRowsBySource.FindOrAdd(Row->SourceDefId).Add(*Row);
	}
	
	NS_OBJ_LOG(LogNSGAS, Log,
		"CombatStat Modifier 캐시 생성 완료. SourceCount={SourceCount}",
		("SourceCount", CachedModifierRowsBySource.Num())
	);
}

void UNSCombatStatComponent::RebuildActiveModifierCache()
{
	ActiveModifiersByAbility.Reset();
	
	UNSAugmentInventoryComponent* AugmentInventory = CachedAugmentInventory.Get();
	
	if (!AugmentInventory)
	{
		return;
	}
	
	for (const FNSAugmentInstance& OwnedAugment : AugmentInventory->GetOwned())
	{
		const TArray<FNSCombatStatModifierRow>* ModifierRows =
			CachedModifierRowsBySource.Find(OwnedAugment.DefId);
		
		if (!ModifierRows)
		{
			continue;
		}
		
		for (const FNSCombatStatModifierRow& ModifierRow : *ModifierRows)
		{
			// 보유 증강의 현재 스택 수를 반영해 활성 Modifier를 누적
			ApplyModifierRow(ModifierRow, OwnedAugment.Stacks);
		}
	}
	
	NS_OBJ_LOG(LogNSGAS, Log,
		"CombatStat Active Modifier 캐시 갱신 완료. AbilityCount={AbilityCount}",
		("AbilityCount", ActiveModifiersByAbility.Num())
	);
}

void UNSCombatStatComponent::ApplyModifierRow(const FNSCombatStatModifierRow& ModifierRow, int32 Stacks)
{
	if (Stacks <= 0)
	{
		return;
	}
	
	if (!IsAbilityStatModifiable(ModifierRow.TargetAbilityTag, ModifierRow.StatTag))
	{
		return;
	}
	
	TMap<FGameplayTag, FNSCombatStatModifierSum>& StatMap =
		ActiveModifiersByAbility.FindOrAdd(ModifierRow.TargetAbilityTag);
	
	FNSCombatStatModifierSum& ModifierSum = StatMap.FindOrAdd(ModifierRow.StatTag);

	switch (ModifierRow.Operation)
	{
	case ENSCombatStatModifierOperation::Add:
		// Add는 스택 수만큼 단순 누적
		ModifierSum.AddValue += ModifierRow.Value * static_cast<float>(Stacks);
		break;
		
	case ENSCombatStatModifierOperation::Multiply:
		// Multiply는 스택마다 증가분만 누적. (예: 1.2배 2스택 = 1.4배)
		ModifierSum.MultiplyValue *= 1.0f + (ModifierRow.Value - 1.0f) * static_cast<float>(Stacks);
		break;
		
	default:
		break;
	}
	
	NS_OBJ_LOG(LogNSGAS, Log,
		"CombatStat Modifier 적용. AbilityTag={AbilityTag}, StatTag={StatTag}, Value={Value}, Stacks={Stacks}",
		("AbilityTag", ModifierRow.TargetAbilityTag.ToString()),
		("StatTag", ModifierRow.StatTag.ToString()),
		("Value", ModifierRow.Value),
		("Stacks", Stacks)
	);
}
