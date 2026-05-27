// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Data/Augment/NSAugmentTypes.h"
#include "NSAugmentSelectionComponent.generated.h"

class UNSAugmentPoolDefinition;
class UNSAugmentDefinition;
class UNSDataSubsystem;

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

	// 증강 리롤
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "NS|Augment")
	void Server_RerollCard(int32 Index);

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

	// 증강 추첨 및 클라 전송
	void RollAndPresent();

	UNSAugmentPoolDefinition* FindPool(const FGameplayTag& PoolTag) const;

	// 증강 Rarity 추첨 -> 해당 Rarity내 나올 수 있는 증강 추첨 (오퍼)
	TArray<FPrimaryAssetId> RollCards(UNSAugmentPoolDefinition* Pool, int32 N, ENSAugmentRarity& OutRarity) const;

	UNSAugmentDefinition* ResolveDefinition(
		UNSDataSubsystem* Data,
		const TSoftObjectPtr<UNSAugmentDefinition>& SoftDef) const;

	void CollectInventoryFilter(
		bool& bOutLegendaryFull,
		TSet<FPrimaryAssetId>& OutOwnedMechanicIds) const;

	// Pool->Entries로부터 Rarity별 후보 버킷(나올 수 있는 후보 목록) 생성, ExcludedIds에 있는 Def는 제외, 중복 등록 방지
	void BuildRarityBuckets(
		UNSDataSubsystem* Data,
		const UNSAugmentPoolDefinition* Pool,
		bool bLegendaryFull,
		const TSet<FPrimaryAssetId>& OwnedMechanicIds,
		const TSet<FPrimaryAssetId>& ExcludedIds,
		TMap<ENSAugmentRarity, TArray<UNSAugmentDefinition*>>& OutByRarity) const;

	// 가중치 룰렛으로 Rarity 1회 결정 → 해당 버킷에서 N장 균등 추첨 -> OutRarity에 결정된 Rarity 반환
	TArray<FPrimaryAssetId> DrawCards(
		const UNSAugmentPoolDefinition* Pool,
		const TMap<ENSAugmentRarity, TArray<UNSAugmentDefinition*>>& ByRarity,
		int32 N,
		ENSAugmentRarity& OutRarity) const;

	UPROPERTY()
	TObjectPtr<UNSAugmentPoolDefinition> CurrentPool;

	TArray<FPrimaryAssetId> PendingOffer;

	// 현재 오퍼의 Rarity (개별 카드 리롤 시 동일 Rarity 유지용)
	ENSAugmentRarity CurrentOfferRarity = ENSAugmentRarity::Common;

	int32 CurrentRerollCost = 0;
};
