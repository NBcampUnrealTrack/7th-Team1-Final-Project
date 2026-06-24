// Copyright 2026 One Team. All rights reserved.


#include "NSAugmentSelectionComponent.h"
#include "NSAugmentInventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/GameInstance.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/Augment/NSAugmentDefinition.h"
#include "NeoSanctum/Data/Augment/NSAugmentPoolDefinition.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_Player.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"

UNSAugmentSelectionComponent::UNSAugmentSelectionComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSAugmentSelectionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UNSAugmentSelectionComponent, PendingCount, COND_OwnerOnly);
}

// 서버 트리거(레벨업/엘리트처치/보스처치/보물상자) → 대기열 적재 후 패널 오픈
void UNSAugmentSelectionComponent::EnqueueOffer(FGameplayTag PoolTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!PoolTag.IsValid())
	{
		return;
	}

	PoolQueue.Add(PoolTag);
	SetPendingCount(PoolQueue.Num());

	// 클라이언트 패널 UI 열기
	Client_AutoOpenPanel();

	// 현재 표시 중인 오퍼가 없으면 새 front를 즉시 추첨/표시 (이미 표시 중이면 큐에 쌓아두고 대기)
	if (!bFrontRolled)
	{
		PresentFront();
	}
}

// 클라이언트 트리거(보물상자 등)에서 서버로 적재 요청
void UNSAugmentSelectionComponent::Server_EnqueueOffer_Implementation(FGameplayTag PoolTag)
{
	EnqueueOffer(PoolTag);
}

// Tab으로 패널을 열 때 → 대기열 front 표시
void UNSAugmentSelectionComponent::Server_OpenPanel_Implementation()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	// 대기 오퍼가 없으면 카드 없이 보유 목록만 표시(클라에서 처리) → 서버는 할 일 없음
	PresentFront();
}

void UNSAugmentSelectionComponent::Server_RerollCard_Implementation()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	// 표시 중인 오퍼가 없으면 리롤 불가
	if (!bFrontRolled || !CurrentPool)
	{
		return;
	}

	// TODO : 리롤 비용 차감 (재화 시스템 연동 후). 현재는 카운터만 증가.
	CurrentRerollCost++;

	// front 강제 재추첨 (Rarity도 다시 결정) → Client_PresentOffer까지 PresentFront가 처리
	PresentFront(true);
}

// 증강 골랐을때
void UNSAugmentSelectionComponent::Server_Choose_Implementation(int32 Index)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	// 표시 중인 오퍼만 선택 가능
	if (!bFrontRolled || !PendingOffer.IsValidIndex(Index))
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

	// front 오퍼 소비
	if (PoolQueue.Num() > 0)
	{
		PoolQueue.RemoveAt(0);
	}
	bFrontRolled = false;
	PendingOffer.Reset();
	CurrentRerollCost = 0;
	CurrentPool = nullptr;
	SetPendingCount(PoolQueue.Num());

	// 남은 대기가 있으면 다음 카드 자동 표시, 없으면 카드 영역만 닫기 (패널은 유지)
	if (PoolQueue.Num() > 0)
	{
		PresentFront();
	}
	else
	{
		Client_CloseOffer();
	}
}

void UNSAugmentSelectionComponent::CopyRunStateFrom(const UNSAugmentSelectionComponent* Source)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!Source)
	{
		return;
	}

	PoolQueue = Source->PoolQueue;
	bFrontRolled = Source->bFrontRolled;
	PendingOffer = Source->PendingOffer;
	CurrentRerollCost = Source->CurrentRerollCost;
	CurrentPool = Source->CurrentPool;
	SetPendingCount(PoolQueue.Num());
}

void UNSAugmentSelectionComponent::Reset()
{
	if (bFrontRolled)
	{
		Client_CloseOffer();
	}
	PoolQueue.Reset();
	PendingOffer.Reset();
	bFrontRolled = false;
	CurrentPool = nullptr;
	CurrentRerollCost = 0;
	SetPendingCount(0);
}

// 대기열 front를 클라에 표시
void UNSAugmentSelectionComponent::PresentFront(bool bReroll)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (PoolQueue.Num() == 0)
	{
		return;
	}

	UNSAugmentPoolDefinition* Pool = FindPool(PoolQueue[0]);
	if (!Pool)
	{
		return;
	}
	CurrentPool = Pool;

	// 미추첨이면 첫 추첨, 리롤 요청이면 강제 재추첨. 둘 다 아니면 캐시 그대로 재전송(재오픈 시 동일 카드 유지)
	if (!bFrontRolled || bReroll)
	{
		// 첫 추첨일 때만 리롤 비용 초기화 (리롤은 호출부에서 비용을 올린 뒤 들어옴)
		if (!bFrontRolled)
		{
			CurrentRerollCost = 0;
		}
		// RollCards는 결정된 Rarity를 OutRarity로 돌려주지만 현재 사용처 없음 (로컬로만 받음)
		ENSAugmentRarity RolledRarity = ENSAugmentRarity::Common;
		PendingOffer = RollCards(CurrentPool, CardsCount, RolledRarity);
		bFrontRolled = true;
	}

	Client_PresentOffer(PendingOffer, CurrentRerollCost);
}

void UNSAugmentSelectionComponent::SetPendingCount(int32 NewCount)
{
	PendingCount = NewCount;
	// 리슨서버 호스트는 OnRep이 호출되지 않으므로 서버에서 직접 브로드캐스트
	OnPendingCountChanged.Broadcast(PendingCount);
}

void UNSAugmentSelectionComponent::OnRep_PendingCount()
{
	OnPendingCountChanged.Broadcast(PendingCount);
}

void UNSAugmentSelectionComponent::Client_PresentOffer_Implementation(const TArray<FPrimaryAssetId>& OfferIds, int32 RerollCost)
{
	OnOfferPresented.Broadcast(OfferIds, RerollCost);
}

void UNSAugmentSelectionComponent::Client_CloseOffer_Implementation()
{
	OnOfferClosed.Broadcast();
}

void UNSAugmentSelectionComponent::Client_AutoOpenPanel_Implementation()
{
	// 클라이언트 패널 UI만 연다
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (!GameInstance)
	{
		return;
	}
	if (UNSUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UNSUIManagerSubsystem>())
	{
		UIManager->OpenAugmentationPanel();
	}
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

TArray<FPrimaryAssetId> UNSAugmentSelectionComponent::RollCards(
	UNSAugmentPoolDefinition* Pool, int32 N, ENSAugmentRarity& OutRarity) const
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
	TSet<FPrimaryAssetId> StackFullIds;
	CollectInventoryFilter(bLegendaryFull, OwnedMechanicLegendaryIds, StackFullIds);

	// 새 오퍼는 제외 셋 없음
	const TSet<FPrimaryAssetId> EmptyExcluded;

	TMap<ENSAugmentRarity, TArray<FNSAugmentCandidate>> ByRarity;
	BuildRarityBuckets(Data, Pool, bLegendaryFull, OwnedMechanicLegendaryIds, StackFullIds, EmptyExcluded, ByRarity);

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

bool UNSAugmentSelectionComponent::TryGetOwnerCharacterTag(FGameplayTag& OutCharacterTag) const
{
	OutCharacterTag = FGameplayTag();
	
	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!IsValid(PlayerController))
	{
		return false;
	}
	
	const ANSPlayerState* PlayerState = Cast<ANSPlayerState>(PlayerController->PlayerState); 
	if (!IsValid(PlayerState))
	{
		return false;
	}
	
	const UNSCharacterData* CharacterData = PlayerState->GetCurrentCharacterData();
	if (!IsValid(CharacterData) || !CharacterData->CharacterTag.IsValid())
	{
		return false;
	}
	
	OutCharacterTag = CharacterData->CharacterTag;
	return true;
}

bool UNSAugmentSelectionComponent::TryCreateCandidate(
	UNSDataSubsystem* Data, 
	const FNSAugmentDefinitionRow& Row,
	FNSAugmentCandidate& OutCandidate) const
{
	OutCandidate = FNSAugmentCandidate();
	
	if (!Row.AugmentTag.IsValid()
		|| !Row.OwnerCharacterTag.IsValid()
		|| Row.Definition.IsNull()
		|| Row.SelectionWeight <= 0
		|| Row.MaxStack <= 0)
	{
		return false;
	}
	
	UNSAugmentDefinition* Definition = ResolveDefinition(Data, Row.Definition);
	if (!IsValid(Definition))
	{
		return false;
	}
	
	const FPrimaryAssetId DefId = Definition->GetPrimaryAssetId();
	if (!DefId.IsValid())
	{
		return false;
	}
	
	OutCandidate.DefId = DefId;
	OutCandidate.AugmentTag = Row.AugmentTag;
	OutCandidate.OwnerCharacterTag = Row.OwnerCharacterTag;
	OutCandidate.Rarity = Row.Rarity;
	OutCandidate.SelectionWeight = Row.SelectionWeight;
	OutCandidate.MaxStacks = Row.MaxStack;
	
	return true;
}

bool UNSAugmentSelectionComponent::TryFindCandidateByDefinitionId(
	UNSDataSubsystem* Data, 
	const FPrimaryAssetId& DefId,
	FNSAugmentCandidate& OutCandidate) const
{
	OutCandidate = FNSAugmentCandidate();
	
	if (!IsValid(AugmentDefinitionTable) || !DefId.IsValid())
	{
		return false;
	}
	
	const FString ContextString = TEXT("AugmentDefinitionLookup");
	
	for (const FName& RowName : AugmentDefinitionTable->GetRowNames())
	{
		const FNSAugmentDefinitionRow* Row = 
			AugmentDefinitionTable->FindRow<FNSAugmentDefinitionRow>(RowName, ContextString, false);
		
		if (!Row || !Row->bEnabled)
		{
			continue;
		}
		
		FNSAugmentCandidate Candidate;
		if (!TryCreateCandidate(Data, *Row, Candidate))
		{
			continue;
		}
		
		if (Candidate.DefId == DefId)
		{
			OutCandidate = Candidate;
			return true;
		}
	}
	
	return false;
}

// 레전더리 슬롯이 꽉찼는지, 기믹 변경 레전더리 Id 목록, MaxStack 도달한 Id 목록 저장
void UNSAugmentSelectionComponent::CollectInventoryFilter(
	bool& bOutLegendaryFull,
	TSet<FPrimaryAssetId>& OutOwnedMechanicIds,
	TSet<FPrimaryAssetId>& OutStackFullIds) const
{
	bOutLegendaryFull = false;
	OutOwnedMechanicIds.Reset();
	OutStackFullIds.Reset();

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

	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);

	bOutLegendaryFull = NSInvComp->IsLegendaryFull();
	for (const FNSAugmentInstance& Inst : NSInvComp->GetOwned())
	{
		if (Inst.bCountsAsLegendarySlot)
		{
			OutOwnedMechanicIds.Add(Inst.DefId);
		}

		// 보유 스택이 증강 효과 정의 DT의 MaxStack에 도달하면 후보에서 제외합니다.
		if (Data)
		{
			const UNSAugmentDefinition* Def = Data->GetData<UNSAugmentDefinition>(Inst.DefId);
			if (Def && Inst.Stacks >= Def->MaxStack)
			{
				OutStackFullIds.Add(Inst.DefId);
			}
		}
	}
}

/**
 * 풀에있는 증강후보를 희귀도 기준으로 분류
 * ExcludedIds에 있는 Def는 스킵 (오퍼 내 중복 방지 / 카드별 리롤시 기존 카드 제외)
 * StackFullIds에 있는 Def는 스킵 (MaxStack 도달 → 더 이상 등장 안 함)
 * 같은 Def가 Entries에 여러 번 들어가도 한 번만 등록 (TSet으로 dedupe)
 * Legendary 슬롯 풀이면 기믹 Legendary 제외, LegendaryStatEntries로 대체 투입
 */
void UNSAugmentSelectionComponent::BuildRarityBuckets(
	UNSDataSubsystem* Data,
	const UNSAugmentPoolDefinition* Pool,
	bool bLegendaryFull,
	const TSet<FPrimaryAssetId>& OwnedMechanicIds,
	const TSet<FPrimaryAssetId>& StackFullIds,
	const TSet<FPrimaryAssetId>& ExcludedIds,
	TMap<ENSAugmentRarity, TArray<FNSAugmentCandidate>>& OutByRarity) const
{
	OutByRarity.Reset();
	
	if (!Data || !Pool || !IsValid(AugmentDefinitionTable))
	{
		NS_OBJ_LOG(LogNS, Warning, "증강 효과 정의 DataTable이 설정되지 않았습니다.");
		return;
	}
	
	FGameplayTag OwnerCharacterTag;
	if (!TryGetOwnerCharacterTag(OwnerCharacterTag))
	{
		NS_OBJ_LOG(LogNS, Warning, "현재 캐릭터 태그를 찾지 못했습니다.");
		return;
	}
	
	const FGameplayTag CommonCharacterTag = NSGameplayTags::Character_Common;
	const FString ContextString = TEXT("AugmentDefinitionCandidates");
	
	TMap<FGameplayTag, FNSAugmentCandidate> CandidatesByTags;
	
	for (const FName& RowName : AugmentDefinitionTable->GetRowNames())
	{
		const FNSAugmentDefinitionRow* Row = 
			AugmentDefinitionTable->FindRow<FNSAugmentDefinitionRow>(RowName, ContextString, false);
		
		if (!Row || !Row->bEnabled)
		{
			continue;
		}
		
		if (Row->OwnerCharacterTag != OwnerCharacterTag && Row->OwnerCharacterTag != CommonCharacterTag)
		{
			continue;
		}
		
		FNSAugmentCandidate Candidate;
		if (!TryCreateCandidate(Data, *Row, Candidate))
		{
			NS_OBJ_LOG(LogNS, Warning,
				"유효하지 않은 증강 효과 정의 Row를 제외합니다. RowName={RowName}",
				("RowName", RowName.ToString())
			);
			continue;
		}
		
		if (ExcludedIds.Contains(Candidate.DefId) || StackFullIds.Contains(Candidate.DefId))
		{
			continue;
		}
		
		if (Candidate.Rarity == ENSAugmentRarity::Legendary)
		{
			if (bLegendaryFull || OwnedMechanicIds.Contains(Candidate.DefId))
			{
				continue;
			}
		}
		
		// 같은 AugmentTag의 여러 효과 행은 카드 후보 하나로 취급.
		CandidatesByTags.FindOrAdd(Candidate.AugmentTag, Candidate);
	}
	
	TSet<FGameplayTag> AddedAugmentTags;
	
	for (const TPair<FGameplayTag, FNSAugmentCandidate>& Pair : CandidatesByTags)
	{
		OutByRarity.FindOrAdd(Pair.Value.Rarity).Add(Pair.Value);
		AddedAugmentTags.Add(Pair.Key);
	}
	
	// Legendary 슬롯이 가득 찬 경우에는 Pool에 지정한 수치형 Legendary만 추가.
	if (!bLegendaryFull)
	{
		return;
	}
	
	for (const TSoftObjectPtr<UNSAugmentDefinition>& SoftDefinition : Pool->LegendaryStatEntries)
	{
		UNSAugmentDefinition* Definition = ResolveDefinition(Data, SoftDefinition);
		if (!IsValid(Definition))
		{
			continue;
		}
		
		const FPrimaryAssetId DefId = Definition->GetPrimaryAssetId();
		if (ExcludedIds.Contains(DefId) || StackFullIds.Contains(DefId))
		{
			continue;
		}
		
		FNSAugmentCandidate Candidate;
		if (!TryFindCandidateByDefinitionId(Data, DefId, Candidate))
		{
			NS_OBJ_LOG(LogNS, Warning,
				"LegendaryStatEntries에 등록된 증강의 효과 정의 Row를 찾지 못했습니다. DefId{DefId}",
				("DefId", DefId.ToString())
			);
			continue;
		}
		
		if (Candidate.Rarity != ENSAugmentRarity::Legendary)
		{
			NS_OBJ_LOG(LogNS, Warning,
				"LegendaryStatEntries의 증강 희귀도가 Legendary가 아닙니다. DefId={DefId}",
				("DefId", DefId.ToString())
			);
			continue;
		}
		
		if (Candidate.OwnerCharacterTag != OwnerCharacterTag && Candidate.OwnerCharacterTag != CommonCharacterTag)
		{
			continue;
		}
		
		if (AddedAugmentTags.Contains(Candidate.AugmentTag))
		{
			continue;
		}
		
		OutByRarity.FindOrAdd(ENSAugmentRarity::Legendary).Add(Candidate);
		AddedAugmentTags.Add(Candidate.AugmentTag);
	}
}

// 가중치 룰렛으로 Rarity 1회 결정 → 해당 Rarity 버킷에서 N장 중복 없이 균등 추첨
TArray<FPrimaryAssetId> UNSAugmentSelectionComponent::DrawCards(
	const UNSAugmentPoolDefinition* Pool,
	const TMap<ENSAugmentRarity, TArray<FNSAugmentCandidate>>& ByRarity,
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
		const TArray<FNSAugmentCandidate>* Bucket = ByRarity.Find(RarityWeight.Key);
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
		const TArray<FNSAugmentCandidate>* Bucket = ByRarity.Find(RarityWeight.Key);
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
	TArray<FNSAugmentCandidate> Bucket = ByRarity.FindChecked(ChosenRarity);
	const int32 DrawCount = FMath::Min(N, Bucket.Num());
	Result.Reserve(DrawCount);
	for (int32 i = 0; i < DrawCount; ++i)
	{
		const int32 PickIndex = FMath::RandRange(0, Bucket.Num() - 1);
		const FNSAugmentCandidate Picked = Bucket[PickIndex];
		Bucket.RemoveAtSwap(PickIndex);
		
		NS_OBJ_LOG(LogNS, Log,
			"증강 카드 후보를 선택했습니다. Rarity={Rarity}, DefId={DefId}",
			("Rarity", static_cast<int32>(ChosenRarity)),
			("DefId", Picked.DefId.ToString())
		);
		Result.Add(Picked.DefId);
	}

	return Result;
}
