// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemComponent.h"
#include "NeoSanctum/Type/NSCharacterStatsMessageTypes.h"
#include "NSCharacterStatsBridgeSubsystem.generated.h"

class ANSPlayerState;
class UAbilitySystemComponent;

/**
 * 캐릭터 스텟값을 UI 메시지로 변환해서 전달하는 다리
 * UI와 Attribute/CombatStat 시스템이 서로에게 의존하지 않음
 */
struct FNSObservedCharacterStatAttribute
{
	FGameplayAttribute Attribute;
	FDelegateHandle Handle;
};

UCLASS()
class NEOSANCTUM_API UNSCharacterStatsBridgeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
	
public:
	UFUNCTION(BlueprintCallable, Category = "UI|CharacterStats")
	void BroadcastCharacterStats(APlayerController* OwningPlayer);
	
	UFUNCTION(BlueprintCallable, Category = "UI|CharacterStats")
	void StopBroadcastCharacterStats();
	
private:
	void AddAttributeStat(
		TArray<struct FNSCharacterStatViewData>& OutStats,
		const UAbilitySystemComponent* ASC,
		const FGameplayAttribute& Attribute,
		const FGameplayTag& StatTag,
		const FText& DisplayName,
		ENSCharacterStatDisplayType DisplayType = ENSCharacterStatDisplayType::Number) const;
	
	void BindCharacterStats(APlayerController* OwningPlayer);

	void UnbindCharacterStats();

	void HandleObservedAttributeChanged(const FOnAttributeChangeData& Data);

	void BroadcastCachedCharacterStats();

	TWeakObjectPtr<APlayerController> CachedOwningPlayer;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	TArray<FNSObservedCharacterStatAttribute> ObservedAttributes;
};
