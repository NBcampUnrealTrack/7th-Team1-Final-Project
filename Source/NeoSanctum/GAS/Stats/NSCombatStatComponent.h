// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "NSCombatStatComponent.generated.h"

class UDataTable;

/**
 * DataTable Row를 런타임 조회용으로 가볍게 캐싱한 값
 */
struct FNSCachedAbilityBaseStat
{
	float BaseValue = 0.0f;
	bool bModifiable = false;
};

/**
 * 전투 능력치를 관리하고 처리하는 컴포넌트
 * 전투 관련 능력치의 추적, 수정, 조회와 관련된 기능을 캡슐화하는 데 사용
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSCombatStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSCombatStatComponent();
	
	bool TryGetBaseAbilitySet(
		const FGameplayTag& AbilityTag,
		const FGameplayTag& StatTag,
		float& OutValue
	) const;
	
	bool IsAbilityStatModifiable(const FGameplayTag& AbilityTag, const FGameplayTag& StatTag) const;

protected:
	virtual void BeginPlay() override;
	
	// DataTable은 편집용 원본이고, 실제 Ability 실행 중에는 캐시를 조회
	void RebuildBaseStatCache();
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (RequiredAssetDataTags = "RowStructure=/Script/NeoSanctum.NSAbilityBaseStatRow"))
	TObjectPtr<UDataTable> AbilityBaseStatTable;

private:
	TMap<FGameplayTag, TMap<FGameplayTag, FNSCachedAbilityBaseStat>> CachedBaseStatsByAbility;
};
