// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Data/Augment/NSAugmentTypes.h"
#include "NSAugmentSelectionComponent.generated.h"

struct FNSAugmentRarityRule;
class UNSAugmentDefinition;
class UNSDataSubsystem;

/**
 * 증강 카드 후보 하나의 런타임 선택 정보.
 *
 * 같은 AugmentTag를 가진 여러 증강 효과 행은 하나의 후보로 그룹핑합니다.
 * DefId는 카드 UI 전달과 Inventory 보유 데이터 식별에 유지합니다.
 */
struct FNSAugmentCandidate
{
	FPrimaryAssetId DefId;
	FGameplayTag AugmentTag;
	FGameplayTag OwnerCharacterTag;
	ENSAugmentRarity Rarity = ENSAugmentRarity::Common;
	int32 SelectionWeight = 1;
	int32 MaxStacks = 1;
	bool bCountsAsLegendarySlot = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAugmentOfferPresented, const TArray<FNSAugmentSelectionCard>&, Cards, int32, RerollCost);
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
	void EnqueueOffer(FGameplayTag RewardTriggerTag);

	// 클라이언트 트리거(보물상자 등)에서 서버로 적재 요청할 때 호출
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "NS|Augment")
	void Server_EnqueueOffer(FGameplayTag RewardTriggerTag);

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

	// 서버 권한, Seamless Travel 시 이전 PC의 추첨 대기열·카운트를 이관
	void CopyRunStateFrom(const UNSAugmentSelectionComponent* Source);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(EditDefaultsOnly, Category = "NS|Augment", meta = (ClampMin = "1"))
	int32 CardsCount = 3;

private:
	UFUNCTION(Client, Reliable)
	void Client_PresentOffer(const TArray<FNSAugmentSelectionCard>& Cards, int32 RerollCost);

	UFUNCTION(Client, Reliable)
	void Client_CloseOffer();

	// 대기열에 오퍼가 새로 적재됐을 때 클라이언트 패널 자동 오픈 지시
	UFUNCTION(Client, Reliable)
	void Client_AutoOpenPanel();

	UFUNCTION()
	void OnRep_PendingCount();

	// 서버에서 대기 카운트 변경 + (호스트용) 즉시 브로드캐스트
	void SetPendingCount(int32 NewCount);
	
	// Queue Front를 카드로 제시. 후보가 없으면 해당 트리거를 소비하고 다음 Front를 계속 확인.
	void PresentFront(bool bReroll = false);
	
	/**
	 * 현재 Front 오퍼를 소비하고 카드 추첨 상태를 초기화.
	 * 카드 선택 완료 또는 현재 트리거에서 후보를 만들 수 없을 때 호출.
	 */
	void ConsumeFrontOffer();
	
	// 같은 AugmentTag 그룹의 공통 선택 메타데이터와 Definition 식별자 ↔ AugmentTag 연결 무결성을 검사.
	void ValidateAugmentDefinitionGroups(UNSDataSubsystem* Data) const;

	bool TryFindRarityRule(
		UNSDataSubsystem* Data,
		const FGameplayTag& RewardTriggerTag, 
		FNSAugmentRarityRule& OutRule
	) const;

	// 현재 보유 증강 상태를 반영해 선택 가능한 후보를 희귀도별로 구성하고, 카드 슬롯별 선택 결과를 생성.
	TArray<FNSAugmentSelectionCard> RollCards(const FNSAugmentRarityRule& RarityRule, int32 N) const;

	UNSAugmentDefinition* ResolveDefinition(
		UNSDataSubsystem* Data,
		const TSoftObjectPtr<UNSAugmentDefinition>& SoftDef) const;
	
	// Pawn 대신 PlayerState의 CharacterData를 기준으로 현재 런에서 선택한 캐릭터 태그를 가져옴.
	bool TryGetOwnerCharacterTag(FGameplayTag& OutCharacterTag) const;
	
	/**
 	 * DT Row를 카드 후보 생성과 보유 증강 판정에 사용할 런타임 후보 데이터로 변환.
 	 *
 	 * AugmentTag는 같은 증강의 효과 행을 그룹핑하고, DefId는 Definition DA 조회와 보유 증강 식별에 사용.
 	 */
	bool TryCreateCandidate(
		UNSDataSubsystem* Data,
		const FNSAugmentDefinitionRow& Row,
		FNSAugmentCandidate& OutCandidate
	) const;
	
 	// 기존 DefId 기반 보유 데이터를 DT 후보 메타 정보에 연결.
	bool TryFindCandidateByDefinitionId(
		UNSDataSubsystem* Data,
		const FPrimaryAssetId& DefId,
		FNSAugmentCandidate& OutCandidate
	) const;

	void CollectInventoryFilter(
		bool& bOutLegendaryFull,
		TSet<FPrimaryAssetId>& OutOwnedLegendarySlotIds,
		TSet<FPrimaryAssetId>& OutStackFullIds) const;

	/**
 	 * DT_AugmentDefinition에서 현재 캐릭터가 선택할 수 있는 증강 후보를 희귀도별로 구성.
 	 *
 	 * 같은 AugmentTag를 가진 여러 Modifier Row는 하나의 카드 후보로 통합.
 	 * DefId는 카드 후보 전송과 보유 증강 식별을 위해 유지.
 	 */
	void BuildRarityBuckets(
		UNSDataSubsystem* Data,
		bool bLegendaryFull,
		const TSet<FPrimaryAssetId>& OwnedLegendarySlotIds,
		const TSet<FPrimaryAssetId>& StackFullIds,
		const TSet<FPrimaryAssetId>& ExcludedIds,
		TMap<ENSAugmentRarity, TArray<FNSAugmentCandidate>>& OutByRarity
	) const;

	/**
 	 * 카드 슬롯마다 희귀도와 후보를 독립적으로 선택.
 	 *
 	 * 카드가 선택될 때마다 해당 후보는 RemainingByRarity에서 제거된다.
 	 * 이후 슬롯은 남은 후보가 있는 희귀도만 대상으로 확률을 다시 계산.
 	 */
	TArray<FNSAugmentSelectionCard> DrawCards(
		const FNSAugmentRarityRule& RarityRule,
		const TMap<ENSAugmentRarity, TArray<FNSAugmentCandidate>>& ByRarity,
		int32 N
	) const;
	
	// 서버 전용: 보상 트리거 FIFO 큐, Front는 카드 선택 완료 또는 선택 가능한 후보가 없을 때 소비.
	TArray<FGameplayTag> RewardTriggerQueue;

	// front 오퍼가 이미 추첨되어 캐싱됐는지 (패널 재오픈 시 재추첨 방지, 리롤로만 재추첨)
	bool bFrontRolled = false;

	TArray<FNSAugmentSelectionCard> PendingOffer;

	int32 CurrentRerollCost = 0;

	// 대기 중인 증강 선택권 수 (오너에게만 레플리케이션, UI 뱃지용)
	UPROPERTY(ReplicatedUsing = OnRep_PendingCount)
	int32 PendingCount = 0;
	
	// 현재 런에서 증강 정의 그룹 무결성을 검사 했는지 여부.
	bool bHasValidatedAugmentDefinitionGroups = false;
};
