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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAugmentPendingCountChanged, int32, NewCount);

UCLASS(ClassGroup=(NeoSanctum), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSAugmentSelectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSAugmentSelectionComponent();

	UPROPERTY(BlueprintAssignable, Category = "NS|Augment")
	FOnAugmentOfferPresented OnOfferPresented;

	UPROPERTY(BlueprintAssignable, Category = "NS|Augment")
	FOnAugmentOfferClosed OnOfferClosed;

	// 대기 카운트 변경 알림
	UPROPERTY(BlueprintAssignable, Category = "NS|Augment")
	FOnAugmentPendingCountChanged OnPendingCountChanged;

	// 서버 권한 트리거(레벨업/엘리트킬/보스처치)에서 직접 호출 → 대기열에 적재 후 패널 자동 오픈
	UFUNCTION(BlueprintCallable, Category = "NS|Augment")
	void EnqueueOffer(FGameplayTag PoolTag);

	// 클라이언트 트리거(보물상자 등)에서 서버로 적재 요청할 때 호출
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "NS|Augment")
	void Server_EnqueueOffer(FGameplayTag PoolTag);

	// Tab으로 패널을 열 때 호출 → 대기열 front 오퍼를 클라에 표시
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "NS|Augment")
	void Server_OpenPanel();

	// 증강 리롤 (카드 3개 전부 새로 추첨)
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "NS|Augment")
	void Server_RerollCard();

	// 증강 골랐을때
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "NS|Augment")
	void Server_Choose(int32 Index);

	// 현재 대기 중인 증강 선택권 수
	UFUNCTION(BlueprintPure, Category = "NS|Augment")
	int32 GetPendingCount() const { return PendingCount; }

	// 선택 상태 초기화 (인런 종료 등)
	void Reset();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, Category = "NS|Augment", meta = (ClampMin = "1"))
	int32 CardsCount = 3;

private:
	UFUNCTION(Client, Reliable)
	void Client_PresentOffer(const TArray<FPrimaryAssetId>& OfferIds, int32 RerollCost);

	UFUNCTION(Client, Reliable)
	void Client_CloseOffer();

	// 대기열에 오퍼가 새로 적재됐을 때 클라이언트 패널 자동 오픈 지시
	UFUNCTION(Client, Reliable)
	void Client_AutoOpenPanel();

	UFUNCTION()
	void OnRep_PendingCount();

	// 서버에서 대기 카운트 변경 + (호스트용) 즉시 브로드캐스트
	void SetPendingCount(int32 NewCount);

	// 대기열 front를 클라에 표시. bReroll=true면 강제 재추첨, 아니면 미추첨 시에만 추첨(캐시 유지)
	void PresentFront(bool bReroll = false);

	UNSAugmentPoolDefinition* FindPool(const FGameplayTag& PoolTag) const;

	// 증강 Rarity 추첨 -> 해당 Rarity내 나올 수 있는 증강 추첨 (오퍼)
	TArray<FPrimaryAssetId> RollCards(UNSAugmentPoolDefinition* Pool, int32 N, ENSAugmentRarity& OutRarity) const;

	UNSAugmentDefinition* ResolveDefinition(
		UNSDataSubsystem* Data,
		const TSoftObjectPtr<UNSAugmentDefinition>& SoftDef) const;

	void CollectInventoryFilter(
		bool& bOutLegendaryFull,
		TSet<FPrimaryAssetId>& OutOwnedMechanicIds,
		TSet<FPrimaryAssetId>& OutStackFullIds) const;

	// Pool->Entries로부터 Rarity별 후보 버킷(나올 수 있는 후보 목록) 생성, ExcludedIds/StackFullIds에 있는 Def는 제외, 중복 등록 방지
	void BuildRarityBuckets(
		UNSDataSubsystem* Data,
		const UNSAugmentPoolDefinition* Pool,
		bool bLegendaryFull,
		const TSet<FPrimaryAssetId>& OwnedMechanicIds,
		const TSet<FPrimaryAssetId>& StackFullIds,
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

	// 서버 전용: 추첨 대기 중인 풀 태그 FIFO 큐 (front가 현재 표시 대상, RemoveAt(0)으로 소비)
	TArray<FGameplayTag> PoolQueue;

	// front 오퍼가 이미 추첨되어 캐싱됐는지 (패널 재오픈 시 재추첨 방지, 리롤로만 재추첨)
	bool bFrontRolled = false;

	TArray<FPrimaryAssetId> PendingOffer;

	int32 CurrentRerollCost = 0;

	// 대기 중인 증강 선택권 수 (오너에게만 레플리케이션, UI 뱃지용)
	UPROPERTY(ReplicatedUsing = OnRep_PendingCount)
	int32 PendingCount = 0;
};
