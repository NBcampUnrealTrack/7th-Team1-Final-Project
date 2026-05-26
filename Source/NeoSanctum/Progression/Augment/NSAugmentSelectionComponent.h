// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Data/Augment/NSAugmentTypes.h"
#include "NSAugmentSelectionComponent.generated.h"

class UNSAugmentPoolDefinition;
class UNSAugmentDefinition;
class UNSRunGameDataComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAugmentOfferPresented, const TArray<FPrimaryAssetId>&, OfferIds, int32, RerollCost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAugmentOfferClosed);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSAugmentSelectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSAugmentSelectionComponent();

	UPROPERTY(BlueprintAssignable, Category = "NS|Augment")
	FOnAugmentOfferPresented OnOfferPresented;

	UPROPERTY(BlueprintAssignable, Category = "NS|Augment")
	FOnAugmentOfferClosed OnOfferClosed;

	// 트리거(레벨업/엘리트킬/보물상자)에서 호출
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "NS|Augment")
	void Server_RequestOffer(FGameplayTag PoolTag);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "NS|Augment")
	void Server_Reroll();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "NS|Augment")
	void Server_Choose(int32 Index);

	// 선택 상태 초기화 (인런 종료, 선택 완료 등)
	void Reset();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "NS|Augment", meta = (ClampMin = "1"))
	int32 CardsCount = 3;
	
private:
	UFUNCTION(Client, Reliable)
	void Client_PresentOffer(const TArray<FPrimaryAssetId>& OfferIds, int32 RerollCost);

	UFUNCTION(Client, Reliable)
	void Client_CloseOffer();
	
	void RollAndPresent();
	UNSAugmentPoolDefinition* FindPool(const FGameplayTag& PoolTag) const;
	TArray<FPrimaryAssetId> RollCards(UNSAugmentPoolDefinition* Pool, int32 N) const;

	UNSAugmentDefinition* ResolveDefinition(
		UNSRunGameDataComponent* Run,
		const TSoftObjectPtr<UNSAugmentDefinition>& SoftDef) const;

	void CollectInventoryFilter(
		bool& bOutLegendaryFull,
		TSet<FPrimaryAssetId>& OutOwnedMechanicIds) const;

	void BuildRarityBuckets(
		UNSRunGameDataComponent* Run,
		const UNSAugmentPoolDefinition* Pool,
		bool bLegendaryFull,
		const TSet<FPrimaryAssetId>& OwnedMechanicIds,
		TMap<ENSAugmentRarity, TArray<UNSAugmentDefinition*>>& OutByRarity) const;

	TArray<FPrimaryAssetId> DrawCards(
		const UNSAugmentPoolDefinition* Pool,
		const TMap<ENSAugmentRarity, TArray<UNSAugmentDefinition*>>& ByRarity,
		int32 N) const;
	
	UPROPERTY()
	TObjectPtr<UNSAugmentPoolDefinition> CurrentPool;

	TArray<FPrimaryAssetId> PendingOffer;

	int32 CurrentRerollCost = 0;
};
