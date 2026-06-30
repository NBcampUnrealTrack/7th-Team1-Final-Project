// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "TimerManager.h"
#include "NeoSanctum/Data/Augment/NSAugmentTypes.h"
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

// 일정 시간 동안 적용되는 CombatStat Modifier
struct FNSTemporaryCombatStatModifier
{
	// 등록된 버프 Modifier를 식별하기 위한 식별자 : Duration이 지난 후에 관련된 버프를 정확하게 제거하기 위함
	FGuid Handle;

	// 수정할 대상 Ability 태그
	FGameplayTag TargetAbilityTag;

	// 수정할 대상 CombatStat 태그
	FGameplayTag StatTag;

	// CombatStat 수정 계산방식 : Buff GA에서 따로 설정할 수 있도록 하는 것이 필요함
	ENSCombatStatModifierOperation Operation = ENSCombatStatModifierOperation::Add;

	// 적용할 보정 값
	float Value = 0.0f;

	// Duration 만료 제거 타이머
	FTimerHandle ExpireTimerHandle;
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

	// TemporaryModifier 등록
	FGuid AddTemporaryCombatStatModifier(
		const FGameplayTag& TargetAbilityTag,
		const FGameplayTag& StatTag,
		ENSCombatStatModifierOperation Operation,
		float Value,
		float Duration
	);

	// TemporaryModifier 제거
	void RemoveTemporaryCombatStatModifier(FGuid Handle);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	// CommonDataConfig에서 공용 AbilityBaseStatTable을 받아옴.
	// 데이터가 아직 준비되지 않았다면 OnCommonDataReady 이후 다시 시도.
	bool TryResolveAbilityBaseStatTableFromDataSubsystem();
	
	// RunConfig에서 현재 인런 증강 후보 테이블을 받아옴.
	// 인런 데이터가 아직 준비되지 않았다면 OnRunGameDataReady 이후 다시 시도.
	bool TryResolveAugmentDefinitionTableFromDataSubsystem();
	
	UFUNCTION()
	void HandleCommonDataReady();
	
	UFUNCTION()
	void HandleRunGameDataReady();
	
	// DataTable은 편집용 원본이고, 실제 Ability 실행 중에는 캐시를 조회
	void RebuildBaseStatCache();
	
	void BindAugmentInventory();
	
	UFUNCTION()
	void HandleAugmentInventoryChanged();
	
	/**
	  * DT_AugmentDefinition의 Modifier Row를 Definition DA의 DefId 기준으로 캐싱.
	  *
	  * 보유 증강은 DefId로 저장되므로, 활성 Modifier 계산 중 DataTable 전체를 순회하지 않도록 함.
	  */
	void RebuildAugmentSourceCache();
	
	/**
 	 * 현재 보유 중인 증강과 스택 수를 기준으로 활성 Modifier 캐시를 다시 만듭니다.
 	 *
 	 * AugmentInventory가 변경될 때 호출되며,
 	 * Definition DA의 DefId로 Modifier 원본 캐시를 찾아
 	 * 최종 스탯 계산에 사용할 Add/Multiply 값을 누적합니다.
 	 */
	void RebuildActiveModifierCache();
	
	void ApplyModifierRow(const FNSAugmentDefinitionRow& ModifierRow, int32 Stacks);

	// 활성화 TemporaryModifier 캐시를 갱신하는 메서드
	void RebuildTemporaryModifierCache();

	// Ability/Stat 조합의 TemporaryModifier 조회
	const FNSCombatStatModifierSum* FindTemporaryModifierSum(
		const FGameplayTag& AbilityTag,
		const FGameplayTag& StatTag
	) const;

	/** 
	 * 기존 Final 값에 TemporaryModifier 보정을 적용 
	 * 일단 최종값에 적용하도록 만들었으나 밸런스상 위험할 가능성이 있다고 생각함
	 */
	void ApplyTemporaryModifierToFinalValue(
		const FNSCombatStatModifierSum& ModifierSum,
		float& InOutValue
	) const;
	
protected:
	// CommonDataConfig가 가진 스킬 기본 스탯 테이블.
	// UNSCombatStatComponent는 이 테이블을 받아 BeginPlay 또는 CommonData 로드 완료 시 캐싱.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (RequiredAssetDataTags = "RowStructure=/Script/NeoSanctum.NSAbilityBaseStatRow"))
	TObjectPtr<UDataTable> AbilityBaseStatTable;
	
	// 현재 런에서 사용하는 증강 후보 테이블 캐시. 원본은 NSRunConfig이고 NSDataSubsystem에서 받아옴.
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> AugmentDefinitionTable;

private:
	TMap<FGameplayTag, TMap<FGameplayTag, FNSCachedAbilityBaseStat>> CachedBaseStatsByAbility;
	
	TMap<FPrimaryAssetId, TArray<FNSAugmentDefinitionRow>> CachedModifierRowsByDefId;
	
	TMap<FGameplayTag, TMap<FGameplayTag, FNSCombatStatModifierSum>> ActiveModifiersByAbility;

	// 등록된 TemporaryModifier
	TMap<FGuid, FNSTemporaryCombatStatModifier> TemporaryModifiersByHandle;

	// 태그로 검색할 수 있도록 한 최종 계산용 TemporaryModifier 맵
	TMap<FGameplayTag, TMap<FGameplayTag, FNSCombatStatModifierSum>> TemporaryModifiersByAbility;
	
	TWeakObjectPtr<UNSAugmentInventoryComponent> CachedAugmentInventory;
};
