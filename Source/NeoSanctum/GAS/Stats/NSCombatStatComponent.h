// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "NSCombatStatComponent.generated.h"

class UNSAugmentInventoryComponent;
class UDataTable;


// DataTable Row를 런타임 조회용으로 가볍게 캐싱한 값
struct FNSCachedAbilityBaseStat
{
	float BaseValue = 0.0f;
	bool bModifiable = false;
};

/**
 * 최종 스탯 계산에 사용할 Add / Multiply Modifier 누적값입니다.
 *
 * FinalValue = (BaseValue + AddValue) * MultiplyValue
 */
struct FNSCombatStatModifierSum
{
	float AddValue = 0.0f;
	float MultiplyValue = 1.0f;
};

/**
 * 스킬 전투 스탯 DataTable을 캐싱하고 조회하는 컴포넌트
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSCombatStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSCombatStatComponent();
	
	bool TryGetBaseAbilityStat(
		const FGameplayTag& AbilityTag,
		const FGameplayTag& StatTag,
		float& OutValue
	) const;
	
	bool IsAbilityStatModifiable(const FGameplayTag& AbilityTag, const FGameplayTag& StatTag) const;
	
	bool TryGetFinalAbilityStat(
		const FGameplayTag& AbilityTag,
		const FGameplayTag& StatTag,
		float& OutValue
	) const;

protected:
	virtual void BeginPlay() override;
	
	// DataTable은 편집용 원본이고, 실제 Ability 실행 중에는 캐시를 조회
	void RebuildBaseStatCache();
	
	void BindAugmentInventory();
	
	UFUNCTION()
	void HandleAugmentInventoryChanged();
	
	/**
	 * CombatStat Modifier DataTable을 SourceDefId 기준으로 캐싱.
	 *
	 * DataTable 전체 순회를 Ability 실행 중에 반복하지 않기 위한 초기화용 캐시.
	 * 증강 보유 여부와는 무관하게, 사용 가능한 Modifier Row 전체를 원본 기준으로 정리.
	 */
	void RebuildModifierSourceCache();
	
	/**
	 * 현재 보유 중인 증강과 스택 수를 기준으로 활성 Modifier 캐시를 다시 만듬.
	 *
	 * AugmentInventory가 변경될 때 호출되며,
	 * SourceDefId로 Modifier 원본 캐시를 찾아 최종 스탯 계산에 사용할 Add/Multiply 값을 누적한다.
	 */
	void RebuildActiveModifierCache();
	
	void ApplyModifierRow(const FNSCombatStatModifierRow& ModifierRow, int32 Stacks);
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (RequiredAssetDataTags = "RowStructure=/Script/NeoSanctum.NSAbilityBaseStatRow"))
	TObjectPtr<UDataTable> AbilityBaseStatTable;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (RequiredAssetDataTags = "RowStructure=/Script/NeoSanctum.NSCombatStatModifierRow"))
	TObjectPtr<UDataTable> CombatStatModifierTable;

private:
	TMap<FGameplayTag, TMap<FGameplayTag, FNSCachedAbilityBaseStat>> CachedBaseStatsByAbility;
	
	TMap<FPrimaryAssetId, TArray<FNSCombatStatModifierRow>> CachedModifierRowsBySource;
	
	TMap<FGameplayTag, TMap<FGameplayTag, FNSCombatStatModifierSum>> ActiveModifiersByAbility;
	
	TWeakObjectPtr<UNSAugmentInventoryComponent> CachedAugmentInventory;
};
