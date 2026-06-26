// Copyright 2026 One Team. All rights reserved.


#include "NSAugmentSelectionComponent.h"
#include "NSAugmentInventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/Augment/NSAugmentDefinition.h"
#include "NeoSanctum/Data/Augment/NSAugmentRarityRuleSet.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
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
void UNSAugmentSelectionComponent::EnqueueOffer(FGameplayTag RewardTriggerTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!RewardTriggerTag.IsValid())
	{
		return;
	}
	
	FNSAugmentRarityRule RarityRule;
	if (!TryFindRarityRule(RewardTriggerTag, RarityRule))
	{
		return;
	}

	RewardTriggerQueue.Add(RewardTriggerTag);
	SetPendingCount(RewardTriggerQueue.Num());

	// 클라이언트 패널 UI 열기
	Client_AutoOpenPanel();

	// 현재 표시 중인 오퍼가 없으면 새 front를 즉시 추첨/표시 (이미 표시 중이면 큐에 쌓아두고 대기)
	if (!bFrontRolled)
	{
		PresentFront();
	}
}

// 클라이언트 트리거(보물상자 등)에서 서버로 적재 요청
void UNSAugmentSelectionComponent::Server_EnqueueOffer_Implementation(FGameplayTag RewardTriggerTag)
{
	EnqueueOffer(RewardTriggerTag);
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
	if (!bFrontRolled || RewardTriggerQueue.IsEmpty())
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

	const FNSAugmentSelectionCard& ChosenCard = PendingOffer[Index];

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
	NSInvComp->ApplyAugment(ChosenCard.DefId);

	// front 오퍼 소비
	ConsumeFrontOffer();

	// 남은 대기가 있으면 다음 카드 자동 표시, 없으면 카드 영역만 닫기 (패널은 유지)
	if (RewardTriggerQueue.Num() > 0)
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
	
	RewardTriggerQueue = Source->RewardTriggerQueue;
	bFrontRolled = Source->bFrontRolled;
	PendingOffer = Source->PendingOffer;
	CurrentRerollCost = Source->CurrentRerollCost;
	
	SetPendingCount(RewardTriggerQueue.Num());
}

void UNSAugmentSelectionComponent::Reset()
{
	if (bFrontRolled)
	{
		Client_CloseOffer();
	}
	RewardTriggerQueue.Reset();
	PendingOffer.Reset();
	bFrontRolled = false;
	CurrentRerollCost = 0;
	bHasValidatedAugmentDefinitionGroups = false;
	SetPendingCount(0);
}

// 대기열 front를 클라에 표시
void UNSAugmentSelectionComponent::PresentFront(bool bReroll)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (RewardTriggerQueue.IsEmpty())
	{
		return;
	}

	// 런 데이터 또는 설정 오류는 후보 소진과 구분한다.
	// 구성 문제로 증강 선택 기회가 자동 소비되지 않도록 사전에 중단한다.
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Data || !Data->IsRunReady())
	{
		NS_OBJ_LOG(LogNS, Warning, "증강 데이터가 준비되지 않아 오퍼를 생성할 수 없습니다.");
		return;
	}
	
	if (!IsValid(AugmentDefinitionTable))
	{
		NS_OBJ_LOG(LogNS, Warning, "증강 효과 정의 DataTable이 설정되지 않았습니다.");
		return;
	}
	
	if (!bHasValidatedAugmentDefinitionGroups)
	{
		ValidateAugmentDefinitionGroups();
		bHasValidatedAugmentDefinitionGroups = true;
	}
	
	FGameplayTag OwnerCharacterTag;
	if (!TryGetOwnerCharacterTag(OwnerCharacterTag))
	{
		NS_OBJ_LOG(LogNS, Warning, "현재 캐릭터 태그를 찾지 못해 증강 오퍼를 생성할 수 없습니다.");
		return;
	}
	
	while (!RewardTriggerQueue.IsEmpty())
	{
		FNSAugmentRarityRule RarityRule;
		if (!TryFindRarityRule(RewardTriggerQueue[0], RarityRule))
		{
			return;
		}
		
		if (!bFrontRolled || bReroll)
		{
			if (!bFrontRolled)
			{
				CurrentRerollCost = 0;
			}
			
			PendingOffer = RollCards(RarityRule, CardsCount);
			bFrontRolled = !PendingOffer.IsEmpty();
		}
		
		// 현재 트리거가 허용하는 희귀도에서 후보를 만들 수 없으면
		// 해당 선택 기회를 소비하고 다음 트리거를 확인.
		if (PendingOffer.IsEmpty())
		{
			NS_OBJ_LOG(LogNS, Log,
				"선택 가능한 증강 후보가 없어 현재 오퍼를 건너뜁니다. RewardTriggerTag={RewardTriggerTag}",
				("RewardTriggerTag", RewardTriggerQueue[0].ToString())
			);

			ConsumeFrontOffer();
			continue;
		}
		
		Client_PresentOffer(PendingOffer, CurrentRerollCost);
		return;
	}
	
	Client_CloseOffer();
}

void UNSAugmentSelectionComponent::ValidateAugmentDefinitionGroups() const
{
	if (!IsValid(AugmentDefinitionTable))
	{
		return;
	}
	
	if (AugmentDefinitionTable->GetRowStruct() != FNSAugmentDefinitionRow::StaticStruct())
	{
		NS_OBJ_LOG(LogNS, Warning,
			"증강 효과 정의 DataTable의 Row Struct가 올바르지 않아 그룹 검증을 건너뜁니다. Table={Table}",
			("Table", AugmentDefinitionTable->GetName())
		);
		return;
	}

	struct FNSAugmentGroupMeta
	{
		FName FirstRowName;
		ENSAugmentRarity Rarity = ENSAugmentRarity::Common;
		int32 MaxStack = 1;
		int32 SelectionWeight = 1;
	};
	
	TMap<FGameplayTag, FNSAugmentGroupMeta> GroupMetas;
	const FString ContextString = TEXT("AugmentDefinitionGroupValidation");
	
	// 비활성 Row도 이후 활성화 시 그룹 규칙을 깨뜨리지 않도록 함께 검사.
	for (const FName& RowName : AugmentDefinitionTable->GetRowNames())
	{
		const FNSAugmentDefinitionRow* Row = 
			AugmentDefinitionTable->FindRow<FNSAugmentDefinitionRow>(RowName, ContextString, false);
		
		if (!Row)
		{
			continue;
		}
		
		if (!Row->AugmentTag.IsValid())
		{
			NS_OBJ_LOG(LogNS, Warning,
				"증강 정의 Row의 AugmentTag가 유효하지 않습니다. RowName={RowName}",
				("RowName", RowName.ToString())
			);
			continue;
		}
		
		const FNSAugmentGroupMeta* ExistingGroup = GroupMetas.Find(Row->AugmentTag);
		
		if (!ExistingGroup)
		{
			FNSAugmentGroupMeta NewGroup;
			NewGroup.FirstRowName = RowName;
			NewGroup.Rarity = Row->Rarity;
			NewGroup.MaxStack = Row->MaxStack;
			NewGroup.SelectionWeight = Row->SelectionWeight;
			
			GroupMetas.Add(Row->AugmentTag, NewGroup);
			continue;
		}
		
		if (ExistingGroup->Rarity != Row->Rarity)
		{
			NS_OBJ_LOG(LogNS, Warning,
				"같은 AugmentTag 그룹의 Rarity가 일치하지 않습니다. AugmentTag={AugmentTag}, 기준 Row={BaseRowName}, 불일치 Row={RowName}",
				("AugmentTag", Row->AugmentTag.ToString()),
				("BaseRowName", ExistingGroup->FirstRowName.ToString()),
				("RowName", RowName.ToString())
			);
		}
		
		if (ExistingGroup->MaxStack != Row->MaxStack)
		{
			NS_OBJ_LOG(LogNS, Warning,
				"같은 AugmentTag 그룹의 MaxStack이 일치하지 않습니다. AugmentTag={AugmentTag}, 기준 Row={BaseRowName}, 기준 MaxStack={BaseMaxStack}, 불일치 Row={RowName}, 불일치 MaxStack={MaxStack}",
				("AugmentTag", Row->AugmentTag.ToString()),
				("BaseRowName", ExistingGroup->FirstRowName.ToString()),
				("BaseMaxStack", ExistingGroup->MaxStack),
				("RowName", RowName.ToString()),
				("MaxStack", Row->MaxStack)
			);
		}
		
		if (ExistingGroup->SelectionWeight != Row->SelectionWeight)
		{
			NS_OBJ_LOG(LogNS, Warning,
				"같은 AugmentTag 그룹의 SelectionWeight가 일치하지 않습니다. AugmentTag={AugmentTag}, 기준 Row={BaseRowName}, 기준 Weight={BaseWeight}, 불일치 Row={RowName}, 불일치 Weight={Weight}",
				("AugmentTag", Row->AugmentTag.ToString()),
				("BaseRowName", ExistingGroup->FirstRowName.ToString()),
				("BaseWeight", ExistingGroup->SelectionWeight),
				("RowName", RowName.ToString()),
				("Weight", Row->SelectionWeight)
			);
		}
	}
}

void UNSAugmentSelectionComponent::ConsumeFrontOffer()
{
	if (!RewardTriggerQueue.IsEmpty())
	{
		RewardTriggerQueue.RemoveAt(0);
	}
	
	bFrontRolled = false;
	PendingOffer.Reset();
	CurrentRerollCost = 0;
	
	SetPendingCount(RewardTriggerQueue.Num());
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

void UNSAugmentSelectionComponent::Client_PresentOffer_Implementation(
	const TArray<FNSAugmentSelectionCard>& Cards, int32 RerollCost)
{
	OnOfferPresented.Broadcast(Cards, RerollCost);
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

bool UNSAugmentSelectionComponent::TryFindRarityRule(
	const FGameplayTag& RewardTriggerTag, FNSAugmentRarityRule& OutRule) const
{
	OutRule = FNSAugmentRarityRule();
	
	if (!IsValid(AugmentRarityRuleSet))
	{
		NS_OBJ_LOG(LogNS, Warning, "증강 희귀도 규칙 세트가 설정되지 않았습니다.");
		return false;
	}
	
	if (!RewardTriggerTag.IsValid())
	{
		return false;
	}
	
	for (const FNSAugmentRarityRule& Rule : AugmentRarityRuleSet->RarityRules)
	{
		if (Rule.RewardTriggerTag == RewardTriggerTag)
		{
			OutRule = Rule;
			return true;
		}
	}
	
	NS_OBJ_LOG(LogNS, Warning,
		"증강 희귀도 규칙을 찾지 못했습니다. RewardTriggerTag={RewardTriggerTag}",
		("RewardTriggerTag", RewardTriggerTag.ToString())
	);
	
	return false;
}

TArray<FNSAugmentSelectionCard> UNSAugmentSelectionComponent::RollCards(
	const FNSAugmentRarityRule& RarityRule, int32 N) const
{
	if (N <= 0)
	{
		return {};
	}
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Data || !Data->IsRunReady())
	{
		return {};
	}

	bool bLegendaryFull = false;
	TSet<FPrimaryAssetId> OwnedLegendarySlotIds;
	TSet<FPrimaryAssetId> StackFullIds;
	CollectInventoryFilter(bLegendaryFull, OwnedLegendarySlotIds, StackFullIds);

	// 새 오퍼는 제외 셋 없음
	const TSet<FPrimaryAssetId> EmptyExcluded;

	TMap<ENSAugmentRarity, TArray<FNSAugmentCandidate>> ByRarity;
	BuildRarityBuckets(Data, bLegendaryFull, OwnedLegendarySlotIds, StackFullIds, EmptyExcluded, ByRarity);

	return DrawCards(RarityRule, ByRarity, N);
}

// Definition SoftPtr를 직접 로드하지 않고 DataSubsystem 캐시에서 해석.
// 런타임 후보는 기존 카드 후보 전송과 보유 증강 흐름에 사용할 DefId를 여기서 얻음.
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
	
	// 선택 규칙은 DT Row를 기준으로 사용하고,
	// Definition DA는 기존 DefId 기반 카드 제시와 Inventory 보유 흐름을 유지하기 위해 해석.
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
	OutCandidate.bCountsAsLegendarySlot = Row.bCountAsLegendarySlot;
	
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
	
	// 기존 Inventory가 보유하는 DefId를 DT 후보 메타 정보에 연결하기 위해 Row를 순회.
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

// 보유 인벤토리에서 레전더리 상태와 DT 기준 MaxStack에 도달한 DefId를 수집.
void UNSAugmentSelectionComponent::CollectInventoryFilter(
	bool& bOutLegendaryFull,
	TSet<FPrimaryAssetId>& OutOwnedLegendarySlotIds,
	TSet<FPrimaryAssetId>& OutStackFullIds) const
{
	bOutLegendaryFull = false;
	OutOwnedLegendarySlotIds.Reset();
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
			OutOwnedLegendarySlotIds.Add(Inst.DefId);
		}

		// 보유 스택이 증강 효과 정의 DT의 MaxStack에 도달하면 후보에서 제외합니다.
		if (Data)
		{
			// Inventory는 DefId만 보유하므로 DT 후보 정보를 다시 찾아 현재 MaxStack 기준으로 후보 제외 여부를 판정.
			FNSAugmentCandidate Candidate;
			if (TryFindCandidateByDefinitionId(Data, Inst.DefId, Candidate) && Inst.Stacks >= Candidate.MaxStacks)
			{
				OutStackFullIds.Add(Inst.DefId);
			}
		}
	}
}

/**
 * DT_AugmentDefinition에서 현재 캐릭터가 선택할 수 있는 증강 후보를 희귀도별로 구성.
 *
 * 같은 AugmentTag를 가진 여러 Modifier Row는 하나의 카드 후보로 통합.
 * DefId는 카드 후보 전송과 보유 증강 식별을 위해 유지.
 */
void UNSAugmentSelectionComponent::BuildRarityBuckets(
	UNSDataSubsystem* Data,
	bool bLegendaryFull,
	const TSet<FPrimaryAssetId>& OwnedLegendarySlotIds,
	const TSet<FPrimaryAssetId>& StackFullIds,
	const TSet<FPrimaryAssetId>& ExcludedIds,
	TMap<ENSAugmentRarity, TArray<FNSAugmentCandidate>>& OutByRarity) const
{
	OutByRarity.Reset();
	
	if (!Data || !IsValid(AugmentDefinitionTable))
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
		
		// 현재 선택 캐릭터 전용 증강과 Character.Common 증강만 후보로 허용.
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
		
		if (Candidate.Rarity == ENSAugmentRarity::Legendary
			&& Candidate.bCountsAsLegendarySlot
			&& (bLegendaryFull || OwnedLegendarySlotIds.Contains(Candidate.DefId)))
		{
			continue;
		}
		
		// 같은 증강의 다중 Modifier Row는 카드 한 장으로 묶음.
		// 그룹 내 메타 정보 일관성은 데이터 검증 단계에서 보장.
		CandidatesByTags.FindOrAdd(Candidate.AugmentTag, Candidate);
	}
	
	for (const TPair<FGameplayTag, FNSAugmentCandidate>& Pair : CandidatesByTags)
	{
		OutByRarity.FindOrAdd(Pair.Value.Rarity).Add(Pair.Value);
	}
}

// 카드 슬롯마다 희귀도를 독립적으로 결정한 뒤, 해당 희귀도 버킷에서 가중치 기반으로 후보를 하나 선택.
TArray<FNSAugmentSelectionCard> UNSAugmentSelectionComponent::DrawCards(
	const FNSAugmentRarityRule& RarityRule,
	const TMap<ENSAugmentRarity, TArray<FNSAugmentCandidate>>& ByRarity,
	int32 N) const
{
	TArray<FNSAugmentSelectionCard> Result;
	
	if (N <= 0)
	{
		return Result;
	}

	// 카드 선택 후 같은 후보가 다시 나오지 않도록 희귀도별 남은 후보를 별도로 관리.
	TMap<ENSAugmentRarity, TArray<FNSAugmentCandidate>> RemainingByRarity = ByRarity;
	
	Result.Reserve(N);
	
	// 오퍼의 각 카드 슬롯마다 희귀도와 후보를 독립적으로 다시 추첨.
	for (int32 CardIndex = 0; CardIndex < N; ++CardIndex)
	{
		// 현재 후보가 남아 있는 희귀도만으로 가중치를 다시 계산.
		int32 TotalRarityWeight = 0;
		
		// 후보가 남은 희귀도만 합산해 이번 카드 슬롯의 희귀도 룰렛 범위를 구성.
		for (const TPair<ENSAugmentRarity, int32>& RarityWeight : RarityRule.RarityWeights)
		{
			const TArray<FNSAugmentCandidate>* Bucket = RemainingByRarity.Find(RarityWeight.Key);
			
			if (!Bucket || Bucket->IsEmpty())
			{
				continue;
			}
			
			TotalRarityWeight += FMath::Max(0, RarityWeight.Value);
		}
		
		if (TotalRarityWeight <= 0)
		{
			break;
		}
		
		const int32 RarityRollValue = FMath::RandRange(1, TotalRarityWeight);
		int32 AccumulatedRarityWeight = 0;
		ENSAugmentRarity ChosenRarity = ENSAugmentRarity::Common;
		bool bFoundRarity = false;
		
		// 희귀도 룰렛 값이 포함되는 누적 가중치 구간을 찾아 이번 카드의 희귀도 결정.
		for (const TPair<ENSAugmentRarity, int32>& RarityWeight : RarityRule.RarityWeights)
		{
			const TArray<FNSAugmentCandidate>* Bucket = RemainingByRarity.Find(RarityWeight.Key);
			
			if (!Bucket || Bucket->IsEmpty())
			{
				continue;
			}
			
			AccumulatedRarityWeight += FMath::Max(0, RarityWeight.Value);
			
			if (RarityRollValue <= AccumulatedRarityWeight)
			{
				ChosenRarity = RarityWeight.Key;
				bFoundRarity = true;
				break;
			}
		}
		
		if (!bFoundRarity)
		{
			break;
		}
		
		TArray<FNSAugmentCandidate>* SelectedBucket = RemainingByRarity.Find(ChosenRarity);
		
		if (!SelectedBucket || SelectedBucket->IsEmpty())
		{
			break;
		}
		
		int32 TotalSelectionWeight = 0;
		
		// 선택된 희귀도 버킷 안에서 후보 선택 룰렛의 총 가중치를 계산.
		for (const FNSAugmentCandidate& Candidate : *SelectedBucket)
		{
			TotalSelectionWeight += Candidate.SelectionWeight;
		}
		
		if (TotalSelectionWeight <= 0)
		{
			break;
		}
		
		const int32 SelectionRollValue = FMath::RandRange(1, TotalSelectionWeight);
		
		int32 AccumulatedSelectionWeight = 0;
		int32 PickIndex = INDEX_NONE;
		
		// 후보 룰렛 값이 포함되는 누적 SelectionWeight 구간을 찾아 카드 후보 하나 선택.
		for (int32 CandidateIndex = 0; CandidateIndex < SelectedBucket->Num(); ++CandidateIndex)
		{
			AccumulatedSelectionWeight += (*SelectedBucket)[CandidateIndex].SelectionWeight;
			
			if (SelectionRollValue <= AccumulatedSelectionWeight)
			{
				PickIndex = CandidateIndex;
				break;
			}
		}
		
		if (PickIndex == INDEX_NONE)
		{
			break;
		}
		
		const FNSAugmentCandidate Picked = (*SelectedBucket)[PickIndex];
		
		// 같은 후보가 이번 오퍼 안에서 중복 선택되지 않도록 제거.
		SelectedBucket->RemoveAtSwap(PickIndex);
		
		const UEnum* RarityEnum = StaticEnum<ENSAugmentRarity>();
		
		const FString RarityName = RarityEnum
			? RarityEnum->GetNameStringByValue(static_cast<int64>(Picked.Rarity))
			: TEXT("InValid");
		
		NS_OBJ_LOG(LogNS, Log,
			"증강 카드 후보를 가중치로 선택했습니다. Rarity={Rarity}, DefId={DefId}, Weight={Weight}",
			("Rarity", RarityName),
			("DefId", Picked.DefId.ToString()),
			("Weight", Picked.SelectionWeight)
		);
		
		FNSAugmentSelectionCard Card;
		Card.DefId = Picked.DefId;
		Card.Rarity = Picked.Rarity;
		
		Result.Add(Card);
	}
	
	return Result;
}
