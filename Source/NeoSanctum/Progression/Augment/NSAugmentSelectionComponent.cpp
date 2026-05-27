// Copyright 2026 One Team. All rights reserved.


#include "NSAugmentSelectionComponent.h"
#include "NSAugmentInventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/Augment/NSAugmentDefinition.h"
#include "NeoSanctum/Data/Augment/NSAugmentPoolDefinition.h"

UNSAugmentSelectionComponent::UNSAugmentSelectionComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSAugmentSelectionComponent::Server_RequestOffer_Implementation(FGameplayTag PoolTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	UNSAugmentPoolDefinition* Pool = FindPool(PoolTag);
	if (!Pool)
	{
		return;
	}

	CurrentPool = Pool;
	CurrentRerollCost = 0;
	RollAndPresent();
}

void UNSAugmentSelectionComponent::Server_RerollCard_Implementation(int32 Index)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!CurrentPool)
	{
		return;
	}
	if (!PendingOffer.IsValidIndex(Index))
	{
		return;
	}

	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Data || !Data->IsRunReady())
	{
		return;
	}

	// 인벤토리 필터 + 현재 오퍼 카드 전체를 제외 셋으로 (새 카드는 남은 2장과도 달라야 함)
	bool bLegendaryFull = false;
	TSet<FPrimaryAssetId> OwnedMechanicIds;
	CollectInventoryFilter(bLegendaryFull, OwnedMechanicIds);

	const TSet<FPrimaryAssetId> ExcludedIds(PendingOffer);

	TMap<ENSAugmentRarity, TArray<UNSAugmentDefinition*>> ByRarity;
	BuildRarityBuckets(Data, CurrentPool, bLegendaryFull, OwnedMechanicIds, ExcludedIds, ByRarity);

	// 현재 오퍼의 Rarity 버킷에서만 추첨 (오퍼 내 Rarity 일관성 유지)
	const TArray<UNSAugmentDefinition*>* Bucket = ByRarity.Find(CurrentOfferRarity);
	if (!Bucket || Bucket->Num() == 0)
	{
		// 후보 없음 → 리롤 실패, 오퍼 상태 유지
		return;
	}

	const int32 PickIdx = FMath::RandRange(0, Bucket->Num() - 1);
	UNSAugmentDefinition* Picked = (*Bucket)[PickIdx];

	// TODO : 리롤 비용 추후에 수정
	CurrentRerollCost++;
	PendingOffer[Index] = Picked->GetPrimaryAssetId();

	UE_LOG(LogTemp, Warning, TEXT("[RerollCard] Index=%d Rarity=%d → %s (%s)"),
		Index, (int32)CurrentOfferRarity, *Picked->GetName(), *Picked->DisplayName.ToString());

	Client_PresentOffer(PendingOffer, CurrentRerollCost);
}

void UNSAugmentSelectionComponent::Server_Choose_Implementation(int32 Index)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!PendingOffer.IsValidIndex(Index))
	{
		return;
	}

	const FPrimaryAssetId Chosen = PendingOffer[Index];

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}

	APlayerState* PS = PC->PlayerState;
	if (!PS)
	{
		return;
	}

	UNSAugmentInventoryComponent* NSInvComp = PS->FindComponentByClass<UNSAugmentInventoryComponent>();
	if (!NSInvComp)
	{
		return;
	}
	NSInvComp->ApplyAugment(Chosen);

	Reset();
}

void UNSAugmentSelectionComponent::Reset()
{
	if (PendingOffer.Num() > 0)
	{
		Client_CloseOffer();
	}
	PendingOffer.Reset();
	CurrentPool = nullptr;
	CurrentRerollCost = 0;
	CurrentOfferRarity = ENSAugmentRarity::Common;
}

// 새 오퍼 추첨 - Rarity를 새로 결정하고 N장 추첨, 결과를 PendingOffer에 저장
void UNSAugmentSelectionComponent::RollAndPresent()
{
	if (!CurrentPool)
	{
		return;
	}

	ENSAugmentRarity RolledRarity = ENSAugmentRarity::Common;
	PendingOffer = RollCards(CurrentPool, CardsCount, RolledRarity);
	CurrentOfferRarity = RolledRarity;

	Client_PresentOffer(PendingOffer, CurrentRerollCost);
}

void UNSAugmentSelectionComponent::Client_PresentOffer_Implementation(const TArray<FPrimaryAssetId>& OfferIds, int32 RerollCost)
{
	OnOfferPresented.Broadcast(OfferIds, RerollCost);
}

void UNSAugmentSelectionComponent::Client_CloseOffer_Implementation()
{
	OnOfferClosed.Broadcast();
}

// PoolTag가 일치하는 PoolDefinition리턴
UNSAugmentPoolDefinition* UNSAugmentSelectionComponent::FindPool(const FGameplayTag& PoolTag) const
{
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Data || !Data->IsRunReady())
	{
		return nullptr;
	}

	TArray<UNSAugmentPoolDefinition*> Pools =
		Data->GetAllDataOfType<UNSAugmentPoolDefinition>(UNSDataSubsystem::AugmentPoolAssetType);
	for (UNSAugmentPoolDefinition* Pool : Pools)
	{
		if (Pool && Pool->PoolTag == PoolTag)
		{
			return Pool;
		}
	}
	return nullptr;
}

TArray<FPrimaryAssetId> UNSAugmentSelectionComponent::RollCards(UNSAugmentPoolDefinition* Pool, int32 N, ENSAugmentRarity& OutRarity) const
{
	OutRarity = ENSAugmentRarity::Common;
	if (!Pool)
	{
		return {};
	}
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Data || !Data->IsRunReady())
	{
		return {};
	}

	bool bLegendaryFull = false;
	TSet<FPrimaryAssetId> OwnedMechanicLegendaryIds;
	CollectInventoryFilter(bLegendaryFull, OwnedMechanicLegendaryIds);

	// 새 오퍼는 제외 셋 없음
	const TSet<FPrimaryAssetId> EmptyExcluded;

	TMap<ENSAugmentRarity, TArray<UNSAugmentDefinition*>> ByRarity;
	BuildRarityBuckets(Data, Pool, bLegendaryFull, OwnedMechanicLegendaryIds, EmptyExcluded, ByRarity);

	return DrawCards(Pool, ByRarity, N, OutRarity);
}

// 소프트 포인터의 이름으로 FPrimaryAssetId 만들어서 데이터 서브시스템에서 캐시 가져오기
UNSAugmentDefinition* UNSAugmentSelectionComponent::ResolveDefinition(
	UNSDataSubsystem* Data,
	const TSoftObjectPtr<UNSAugmentDefinition>& SoftDef) const
{
	if (!Data || SoftDef.IsNull())
	{
		return nullptr;
	}
	const FPrimaryAssetId Id(UNSDataSubsystem::AugmentAssetType, FName(*SoftDef.GetAssetName()));
	return Data->GetData<UNSAugmentDefinition>(Id);
}

// 레전더리 슬롯이 꽉찼는지, 기믹 변경 레전더리 Id 목록 저장
void UNSAugmentSelectionComponent::CollectInventoryFilter(
	bool& bOutLegendaryFull,
	TSet<FPrimaryAssetId>& OutOwnedMechanicIds) const
{
	bOutLegendaryFull = false;
	OutOwnedMechanicIds.Reset();

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}
	APlayerState* PS = PC->PlayerState;
	if (!PS)
	{
		return;
	}
	UNSAugmentInventoryComponent* NSInvComp = PS->FindComponentByClass<UNSAugmentInventoryComponent>();
	if (!NSInvComp)
	{
		return;
	}

	bOutLegendaryFull = NSInvComp->IsLegendaryFull();
	for (const FNSAugmentInstance& Inst : NSInvComp->GetOwned())
	{
		if (Inst.bCountsAsLegendarySlot)
		{
			OutOwnedMechanicIds.Add(Inst.DefId);
		}
	}
}

/**
 * 풀에있는 증강후보를 희귀도 기준으로 분류
 * ExcludedIds에 있는 Def는 스킵 (오퍼 내 중복 방지 / 카드별 리롤시 기존 카드 제외)
 * 같은 Def가 Entries에 여러 번 들어가도 한 번만 등록 (TSet으로 dedupe)
 * Legendary 슬롯 풀이면 기믹 Legendary 제외, LegendaryStatEntries로 대체 투입
 */
void UNSAugmentSelectionComponent::BuildRarityBuckets(
	UNSDataSubsystem* Data,
	const UNSAugmentPoolDefinition* Pool,
	bool bLegendaryFull,
	const TSet<FPrimaryAssetId>& OwnedMechanicIds,
	const TSet<FPrimaryAssetId>& ExcludedIds,
	TMap<ENSAugmentRarity, TArray<UNSAugmentDefinition*>>& OutByRarity) const
{
	OutByRarity.Reset();
	if (!Data || !Pool)
	{
		return;
	}

	TSet<FPrimaryAssetId> SeenIds;

	for (const TSoftObjectPtr<UNSAugmentDefinition>& SoftDef : Pool->Entries)
	{
		UNSAugmentDefinition* Def = ResolveDefinition(Data, SoftDef);
		if (!Def)
		{
			continue;
		}

		const FPrimaryAssetId DefId = Def->GetPrimaryAssetId();
		if (SeenIds.Contains(DefId))
		{
			continue;
		}
		if (ExcludedIds.Contains(DefId))
		{
			continue;
		}

		if (Def->Rarity == ENSAugmentRarity::Legendary)
		{
			if (bLegendaryFull)
			{
				continue;
			}
			if (OwnedMechanicIds.Contains(DefId))
			{
				continue;
			}
		}

		OutByRarity.FindOrAdd(Def->Rarity).Add(Def);
		SeenIds.Add(DefId);
	}

	if (bLegendaryFull)
	{
		for (const TSoftObjectPtr<UNSAugmentDefinition>& SoftDef : Pool->LegendaryStatEntries)
		{
			UNSAugmentDefinition* Def = ResolveDefinition(Data, SoftDef);
			if (!Def)
			{
				continue;
			}
			const FPrimaryAssetId DefId = Def->GetPrimaryAssetId();
			if (SeenIds.Contains(DefId))
			{
				continue;
			}
			if (ExcludedIds.Contains(DefId))
			{
				continue;
			}
			OutByRarity.FindOrAdd(ENSAugmentRarity::Legendary).Add(Def);
			SeenIds.Add(DefId);
		}
	}
}

// 가중치 룰렛으로 Rarity 1회 결정 → 해당 Rarity 버킷에서 N장 중복 없이 균등 추첨
TArray<FPrimaryAssetId> UNSAugmentSelectionComponent::DrawCards(
	const UNSAugmentPoolDefinition* Pool,
	const TMap<ENSAugmentRarity, TArray<UNSAugmentDefinition*>>& ByRarity,
	int32 N,
	ENSAugmentRarity& OutRarity) const
{
	OutRarity = ENSAugmentRarity::Common;
	TArray<FPrimaryAssetId> Result;
	if (!Pool || N <= 0)
	{
		return Result;
	}

	// 후보가 있는 Rarity들의 가중치만 합산 (빈 버킷에 헛돌지 않음)
	float TotalWeight = 0.f;
	for (const TPair<ENSAugmentRarity, float>& RarityWeight : Pool->RarityWeights)
	{
		const TArray<UNSAugmentDefinition*>* Bucket = ByRarity.Find(RarityWeight.Key);
		if (Bucket && Bucket->Num() > 0)
		{
			TotalWeight += FMath::Max(0.f, RarityWeight.Value);
		}
	}
	if (TotalWeight <= 0.f)
	{
		return Result;
	}

	// 가중치 룰렛으로 이번 오퍼의 Rarity 1번만 결정
	const float Pick = FMath::FRandRange(0.f, TotalWeight);
	float CumulativeSum = 0.f;
	ENSAugmentRarity ChosenRarity = ENSAugmentRarity::Common;
	for (const TPair<ENSAugmentRarity, float>& RarityWeight : Pool->RarityWeights)
	{
		const TArray<UNSAugmentDefinition*>* Bucket = ByRarity.Find(RarityWeight.Key);
		if (!Bucket || Bucket->Num() == 0)
		{
			continue;
		}
		CumulativeSum += FMath::Max(0.f, RarityWeight.Value);
		if (Pick <= CumulativeSum)
		{
			ChosenRarity = RarityWeight.Key;
			break;
		}
	}
	OutRarity = ChosenRarity;

	// 선택된 Rarity 버킷만 복사해서 중복 없이 N장 추첨 (RemoveAtSwap = O(1))
	TArray<UNSAugmentDefinition*> Bucket = ByRarity.FindChecked(ChosenRarity);
	const int32 DrawCount = FMath::Min(N, Bucket.Num());
	Result.Reserve(DrawCount);
	for (int32 i = 0; i < DrawCount; ++i)
	{
		const int32 PickIdx = FMath::RandRange(0, Bucket.Num() - 1);
		UNSAugmentDefinition* Picked = Bucket[PickIdx];
		Bucket.RemoveAtSwap(PickIdx);

		UE_LOG(LogTemp, Warning, TEXT("[DrawCards] Rarity=%d → %s (%s)"),
			(int32)ChosenRarity, *Picked->GetName(), *Picked->DisplayName.ToString());
		Result.Add(Picked->GetPrimaryAssetId());
	}

	return Result;
}
