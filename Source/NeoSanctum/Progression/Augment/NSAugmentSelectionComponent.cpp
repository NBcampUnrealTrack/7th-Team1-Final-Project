// Copyright 2026 One Team. All rights reserved.


#include "NSAugmentSelectionComponent.h"
#include "NSAugmentInventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "NeoSanctum/Core/Component/NSRunGameDataComponent.h"
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

void UNSAugmentSelectionComponent::Server_Reroll_Implementation()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	if (!CurrentPool)
	{
		return;
	}
	
	// TODO : 리롤 비용 추후에 수정
	CurrentRerollCost++;
	RollAndPresent();
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
	
	PendingOffer.Reset();
	CurrentPool = nullptr;
	CurrentRerollCost = 0;
	Client_CloseOffer();
}

// 카드 목록을 뽑아 PendingOffer에 저장하고 클라에 전송
void UNSAugmentSelectionComponent::RollAndPresent()
{
	if (!CurrentPool)
	{
		return;
	}
	PendingOffer = RollCards(CurrentPool, CardsCount);
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
	UNSRunGameDataComponent* Run = UNSRunGameDataComponent::Get(this);
	if (!Run || !Run->IsReady())
	{
		return nullptr;
	}
	
	TArray<UNSAugmentPoolDefinition*> Pools = 
		Run->GetAllDataOfType<UNSAugmentPoolDefinition>(UNSRunGameDataComponent::AugmentPoolAssetType);
	for (UNSAugmentPoolDefinition* Pool : Pools)
	{
		if (Pool && Pool->PoolTag == PoolTag)
		{
			return Pool;
		}
	}
	return nullptr;
}

TArray<FPrimaryAssetId> UNSAugmentSelectionComponent::RollCards(UNSAugmentPoolDefinition* Pool, int32 N) const
{
	if (!Pool)
	{
		return {};
	}
	UNSRunGameDataComponent* Run = UNSRunGameDataComponent::Get(this);
	if (!Run || !Run->IsReady())
	{
		return {};
	}

	bool bLegendaryFull = false;
	TSet<FPrimaryAssetId> OwnedMechanicLegendaryIds;
	CollectInventoryFilter(bLegendaryFull, OwnedMechanicLegendaryIds);

	TMap<ENSAugmentRarity, TArray<UNSAugmentDefinition*>> ByRarity;
	BuildRarityBuckets(Run, Pool, bLegendaryFull, OwnedMechanicLegendaryIds, ByRarity);

	return DrawCards(Pool, ByRarity, N);
}

// 소프트 포인터의 이름으로 FPrimaryAssetId 만들어서 데이터 컴포넌트에서 캐시 가져오기 
UNSAugmentDefinition* UNSAugmentSelectionComponent::ResolveDefinition(
	UNSRunGameDataComponent* Run,
	const TSoftObjectPtr<UNSAugmentDefinition>& SoftDef) const
{
	if (!Run || SoftDef.IsNull())
	{
		return nullptr;
	}
	const FPrimaryAssetId Id(UNSRunGameDataComponent::AugmentAssetType, FName(*SoftDef.GetAssetName()));
	return Run->GetData<UNSAugmentDefinition>(Id);
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

	// 기믹변경 최대개수 채웠는지 확인
	bOutLegendaryFull = NSInvComp->IsLegendaryFull();
	for (const FNSAugmentInstance& Inst : NSInvComp->GetOwned())
	{
		if (Inst.bCountsAsLegendarySlot)
		{
			OutOwnedMechanicIds.Add(Inst.DefId);
		}
	}
}

// 풀에있는 증강후보를 희귀도 기준으로 분류, 기믹변경 최대 횟수 넘으면 레전더리도 스텟증가로 대체
void UNSAugmentSelectionComponent::BuildRarityBuckets(
	UNSRunGameDataComponent* Run,
	const UNSAugmentPoolDefinition* Pool,
	bool bLegendaryFull,
	const TSet<FPrimaryAssetId>& OwnedMechanicIds,
	TMap<ENSAugmentRarity, TArray<UNSAugmentDefinition*>>& OutByRarity) const
{
	OutByRarity.Reset();
	if (!Run || !Pool)
	{
		return;
	}

	for (const TSoftObjectPtr<UNSAugmentDefinition>& SoftDef : Pool->Entries)
	{
		UNSAugmentDefinition* Def = ResolveDefinition(Run, SoftDef);
		if (!Def)
		{
			continue;
		}

		if (Def->Rarity == ENSAugmentRarity::Legendary)
		{
			if (bLegendaryFull)
			{
				continue;
			}
			if (OwnedMechanicIds.Contains(Def->GetPrimaryAssetId()))
			{
				continue;
			}
		}

		OutByRarity.FindOrAdd(Def->Rarity).Add(Def);
	}

	if (bLegendaryFull)
	{
		for (const TSoftObjectPtr<UNSAugmentDefinition>& SoftDef : Pool->LegendaryStatEntries)
		{
			UNSAugmentDefinition* Def = ResolveDefinition(Run, SoftDef);
			if (!Def)
			{
				continue;
			}
			OutByRarity.FindOrAdd(ENSAugmentRarity::Legendary).Add(Def);
		}
	}
}

// 후보가 남아있는 등급으로만 가중치 룰렛을 돌리고, 뽑은 카드는 버킷에서 즉시 제거
TArray<FPrimaryAssetId> UNSAugmentSelectionComponent::DrawCards(
	const UNSAugmentPoolDefinition* Pool,
	const TMap<ENSAugmentRarity, TArray<UNSAugmentDefinition*>>& ByRarity,
	int32 N) const
{
	TArray<FPrimaryAssetId> Result;
	if (!Pool || N <= 0)
	{
		return Result;
	}

	// 뽑은 카드는 로컬 버킷에서 즉시 제거하기 위해 새로 저장
	TMap<ENSAugmentRarity, TArray<UNSAugmentDefinition*>> Buckets = ByRarity;

	Result.Reserve(N);
	while (Result.Num() < N)
	{
		// 후보가 남아있는 등급에만 가중치 합산 (빈 버킷에 헛돌지 않음)
		float TotalWeight = 0.f;
		for (const TPair<ENSAugmentRarity, float>& RarityWeight : Pool->RarityWeights)
		{
			const TArray<UNSAugmentDefinition*>* Bucket = Buckets.Find(RarityWeight.Key);
			if (Bucket && Bucket->Num() > 0)
			{
				TotalWeight += FMath::Max(0.f, RarityWeight.Value);
			}
		}
		if (TotalWeight <= 0.f)
		{
			// 남은 후보가 없거나 모든 가중치가 0 -> 더 못 뽑음
			break;
		}

		// 가중치 룰렛 (빈 버킷은 건너뜀)
		const float Pick = FMath::FRandRange(0.f, TotalWeight);
		float CumulativeSum = 0.f;
		ENSAugmentRarity ChosenRarity = ENSAugmentRarity::Common;
		for (const TPair<ENSAugmentRarity, float>& RarityWeight : Pool->RarityWeights)
		{
			const TArray<UNSAugmentDefinition*>* Bucket = Buckets.Find(RarityWeight.Key);
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

		// 선택된 버킷에서 카드 한 장 균등 추첨 후 제거 (RemoveAtSwap = O(1))
		TArray<UNSAugmentDefinition*>& Bucket = Buckets.FindChecked(ChosenRarity);
		const int32 PickIdx = FMath::RandRange(0, Bucket.Num() - 1);
		UNSAugmentDefinition* Picked = Bucket[PickIdx];
		Bucket.RemoveAtSwap(PickIdx);

		Result.Add(Picked->GetPrimaryAssetId());
	}

	return Result;
}