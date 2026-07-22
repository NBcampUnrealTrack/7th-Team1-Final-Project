// Copyright 2026 One Team. All rights reserved.


#include "NSAugmentSelectionComponent.h"

#include "NSAugmentInventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/Augment/NSAugmentDefinition.h"
#include "NeoSanctum/Data/Augment/NSAugmentRarityRuleSet.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"
#include "NeoSanctum/Data/CommonUpgrade/NSCommonUpgradeUtilityHelper.h"
#include "NeoSanctum/Data/Config/NSRunConfig.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Progression/Currency/NSCurrencyComponent.h"
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

void UNSAugmentSelectionComponent::Client_PresentOffer_Implementation(
	const TArray<FNSAugmentSelectionCard>& Cards,
	int64 RerollCost,
	bool bCanReroll,
	int32 PresentedOfferRevision,
	int32 MaxChoiceCount,
	int32 AvailableCardCount)
{
	OnOfferPresented.Broadcast(
		Cards,
		RerollCost,
		bCanReroll,
		PresentedOfferRevision,
		MaxChoiceCount,
		AvailableCardCount);
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

	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Data || !Data->IsRunReady())
	{
		NS_OBJ_LOG(LogNS, Warning, "증강 데이터가 준비되지 않아 오퍼를 대기열에 추가할 수 없습니다.");
		return;
	}

	FNSAugmentRarityRule RarityRule;
	if (!TryFindRarityRule(Data, RewardTriggerTag, RarityRule))
	{
		return;
	}

	RewardTriggerQueue.Add(RewardTriggerTag);
	SetPendingCount(RewardTriggerQueue.Num());


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

void UNSAugmentSelectionComponent::Server_RerollCard_Implementation(
	int32 ClientOfferRevision)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (!bFrontRolled || RewardTriggerQueue.IsEmpty())
	{
		Client_NotifyRerollResult(
			ENSAugmentRerollResult::NoActiveOffer,
			0,
			0,
			ClientOfferRevision,
			OfferRevision);
		return;
	}

	if (ClientOfferRevision != OfferRevision)
	{
		Client_NotifyRerollResult(
			ENSAugmentRerollResult::StaleRevision,
			0,
			0,
			ClientOfferRevision,
			OfferRevision);
		return;
	}

	UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem || !DataSubsystem->IsRunReady())
	{
		Client_NotifyRerollResult(
			ENSAugmentRerollResult::InvalidRequest,
			0,
			0,
			ClientOfferRevision,
			OfferRevision);
		return;
	}

	FNSAugmentRarityRule RarityRule;
	if (!TryFindRarityRule(
		DataSubsystem,
		RewardTriggerQueue[0],
		RarityRule))
	{
		Client_NotifyRerollResult(
			ENSAugmentRerollResult::InvalidRequest,
			0,
			0,
			ClientOfferRevision,
			OfferRevision);
		return;
	}

	FNSAugmentRerollRule RerollRule;
	if (!TryFindRerollRule(
		RewardTriggerQueue[0],
		RerollRule))
	{
		Client_NotifyRerollResult(
			ENSAugmentRerollResult::InvalidRequest,
			0,
			0,
			ClientOfferRevision,
			OfferRevision);
		return;
	}

	const int64 Cost =
		GetDiscountedRerollCost(
			RerollRule,
			CurrentOfferRerollCount);

	UNSCurrencyComponent* CurrencyComponent =
		GetOwnerCurrencyComponent();

	const int64 HaveCurrency =
		CurrencyComponent
			? CurrencyComponent->GetTemp()
			: 0;

	if (!CurrencyComponent || HaveCurrency < Cost)
	{
		Client_NotifyRerollResult(
			ENSAugmentRerollResult::NotEnoughCurrency,
			Cost,
			HaveCurrency,
			ClientOfferRevision,
			OfferRevision);
		return;
	}

	const int32 MaxChoiceCount =
		GetEffectiveCardsCount();

	const int32 CurrentChoiceCount =
		PendingOffer.Num();

	if (CurrentChoiceCount <= 0)
	{
		Client_NotifyRerollResult(
			ENSAugmentRerollResult::NoActiveOffer,
			0,
			0,
			ClientOfferRevision,
			OfferRevision);

		return;
	}

	TSet<FPrimaryAssetId> PreviousDefIds;

	for (const FNSAugmentSelectionCard& Card :
	     PendingOffer)
	{
		PreviousDefIds.Add(Card.DefId);
	}

	if (PreviousDefIds.Num() !=
		CurrentChoiceCount)
	{
		Client_NotifyRerollResult(
			ENSAugmentRerollResult::NoDifferentOffer,
			0,
			0,
			ClientOfferRevision,
			OfferRevision);

		return;
	}

	bool bDifferentPossible = false;
	int32 NewAvailableCardCount = 0;

	TArray<FNSAugmentSelectionCard> NewOffer =
		RollCardsExcludingComposition(
			RarityRule,
			CurrentChoiceCount,
			PreviousDefIds,
			bDifferentPossible,
			NewAvailableCardCount);

	TSet<FPrimaryAssetId> NewOfferDefIds;

	for (const FNSAugmentSelectionCard& Card :
	     NewOffer)
	{
		NewOfferDefIds.Add(Card.DefId);
	}

	if (!bDifferentPossible ||
		NewOffer.Num() != CurrentChoiceCount ||
		NewOfferDefIds.Num() != NewOffer.Num() ||
		AreDefIdSetsEqual(
			NewOfferDefIds,
			PreviousDefIds))
	{
		Client_NotifyRerollResult(
			ENSAugmentRerollResult::NoDifferentOffer,
			0,
			0,
			ClientOfferRevision,
			OfferRevision);

		return;
	}

	if (!CurrencyComponent->TrySpendTemp(Cost))
	{
		Client_NotifyRerollResult(
			ENSAugmentRerollResult::NotEnoughCurrency,
			Cost,
			CurrencyComponent->GetTemp(),
			ClientOfferRevision,
			OfferRevision);

		return;
	}

	PendingOffer = MoveTemp(NewOffer);
	CurrentAvailableCardCount =
		NewAvailableCardCount;

	++CurrentOfferRerollCount;
	++OfferRevision;

	const int64 NextCost =
		GetDiscountedRerollCost(
			RerollRule,
			CurrentOfferRerollCount);

	const bool bCanRerollAgain =
		CurrentAvailableCardCount >
		CurrentChoiceCount;

	Client_PresentOffer(
		PendingOffer,
		NextCost,
		bCanRerollAgain,
		OfferRevision,
		MaxChoiceCount,
		CurrentAvailableCardCount);
}

// 증강 골랐을때
void UNSAugmentSelectionComponent::Server_Choose_Implementation(int32 Index, int32 ClientOfferRevision)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// 오퍼가 이미 바뀐 뒤에 도착한 예전 선택 요청은 거부 (리롤 직후 구 카드 선택 방지)
	if (ClientOfferRevision != OfferRevision)
	{
		Client_NotifyRerollResult(
			ENSAugmentRerollResult::StaleRevision,
			0,
			0,
			ClientOfferRevision,
			OfferRevision
		);
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
	CurrentOfferRerollCount = Source->CurrentOfferRerollCount;
	CurrentAvailableCardCount = Source->CurrentAvailableCardCount;
	OfferRevision = Source->OfferRevision; // 같은 런이 이어지는 것이라 값을 그대로 이어받음(증가 아님).

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
	CurrentOfferRerollCount = 0;
	CurrentAvailableCardCount = 0;
	++OfferRevision; // 인런 종료 시점에 남아있던 리롤/선택 요청을 무효화.
	bHasValidatedAugmentDefinitionGroups = false;
	SetPendingCount(0);
}

// 대기열 front를 클라에 표시
void UNSAugmentSelectionComponent::PresentFront()
{
	if (!GetOwner() ||
		!GetOwner()->HasAuthority())
	{
		return;
	}

	if (RewardTriggerQueue.IsEmpty())
	{
		return;
	}

	UNSDataSubsystem* Data =
		UNSDataSubsystem::Get(this);

	if (!Data || !Data->IsRunReady())
	{
		NS_OBJ_LOG(
			LogNS,
			Warning,
			"Augment data is not ready. Cannot present augment offer.");

		return;
	}

	if (!IsValid(
		Data->GetCurrentAugmentDefinitionTable()))
	{
		NS_OBJ_LOG(
			LogNS,
			Warning,
			"Current augment definition table is invalid.");

		return;
	}

	if (!bHasValidatedAugmentDefinitionGroups)
	{
		ValidateAugmentDefinitionGroups(Data);
		bHasValidatedAugmentDefinitionGroups = true;
	}

	FGameplayTag OwnerCharacterTag;

	if (!TryGetOwnerCharacterTag(OwnerCharacterTag))
	{
		NS_OBJ_LOG(
			LogNS,
			Warning,
			"Cannot find owner character tag. Cannot present augment offer.");

		return;
	}

	while (!RewardTriggerQueue.IsEmpty())
	{
		FNSAugmentRarityRule RarityRule;

		if (!TryFindRarityRule(
			Data,
			RewardTriggerQueue[0],
			RarityRule))
		{
			return;
		}

		const int32 MaxChoiceCount =
			GetEffectiveCardsCount();

		const bool bNeedsFreshRoll =
			!bFrontRolled;

		if (bNeedsFreshRoll)
		{
			CurrentOfferRerollCount = 0;
			CurrentAvailableCardCount = 0;

			PendingOffer =
				RollCards(
					RarityRule,
					MaxChoiceCount,
					CurrentAvailableCardCount);

			bFrontRolled =
				!PendingOffer.IsEmpty();
		}

		if (PendingOffer.IsEmpty())
		{
			NS_OBJ_LOG(
				LogNS,
				Log,
				"No selectable augment candidates. Skipping current augment offer. RewardTriggerTag={RewardTriggerTag}",
				("RewardTriggerTag",
					RewardTriggerQueue[0].ToString()));

			ConsumeFrontOffer();
			continue;
		}

		if (bNeedsFreshRoll)
		{
			++OfferRevision;
		}

		TSet<FPrimaryAssetId> OfferDefIds;

		for (const FNSAugmentSelectionCard& Card :
		     PendingOffer)
		{
			OfferDefIds.Add(Card.DefId);
		}

		const bool bHasUniqueChoices =
			OfferDefIds.Num() ==
			PendingOffer.Num();

		FNSAugmentRerollRule RerollRule;

		const bool bHasRerollRule =
			TryFindRerollRule(
				RewardTriggerQueue[0],
				RerollRule);

		const bool bCanReroll =
			bHasRerollRule &&
			bHasUniqueChoices &&
			CurrentAvailableCardCount >
			PendingOffer.Num();

		const int64 RerollCost =
			bHasRerollRule
				? GetDiscountedRerollCost(
					RerollRule,
					CurrentOfferRerollCount)
				: 0;

		Client_PresentOffer(
			PendingOffer,
			RerollCost,
			bCanReroll,
			OfferRevision,
			MaxChoiceCount,
			CurrentAvailableCardCount);

		return;
	}

	Client_CloseOffer();
}

void UNSAugmentSelectionComponent::ValidateAugmentDefinitionGroups(UNSDataSubsystem* Data) const
{
	UDataTable* AugmentDefinitionTable = Data ? Data->GetCurrentAugmentDefinitionTable() : nullptr;
	if (!IsValid(AugmentDefinitionTable))
	{
		NS_OBJ_LOG(LogNS, Warning, "현재 RunConfig의 증강 효과 정의 DataTable이 설정되지 않았습니다.");
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
		FPrimaryAssetId DefId;
		FGameplayTag OwnerCharacterTag;
		ENSAugmentRarity Rarity = ENSAugmentRarity::Common;
		int32 MaxStack = 1;
		int32 SelectionWeight = 1;
		bool bCountAsLegendarySlot = false;
	};

	struct FNSDefinitionMeta
	{
		FName FirstRowName;
		FGameplayTag AugmentTag;
	};

	TMap<FGameplayTag, FNSAugmentGroupMeta> GroupMetas;
	TMap<FPrimaryAssetId, FNSDefinitionMeta> DefinitionMeta;
	TSet<FPrimaryAssetId> ReportedDefinitionConflicts;

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

		if (Row->Definition.IsNull())
		{
			NS_OBJ_LOG(LogNS, Warning,
			           "증강 정의 Row의 Definition이 설정되지 않았습니다. RowName={RowName}, AugmentTag={AugmentTag}",
			           ("RowName", RowName.ToString()),
			           ("AugmentTag", Row->AugmentTag.ToString())
			);
			continue;
		}

		UNSAugmentDefinition* Definition = ResolveDefinition(Data, Row->Definition);
		if (!IsValid(Definition))
		{
			NS_OBJ_LOG(LogNS, Warning,
			           "증강 정의 Row의 Definition을 해석하지 못했습니다. RowName={RowName}, AugmentTag={AugmentTag}, Definition={Definition}",
			           ("RowName", RowName.ToString()),
			           ("AugmentTag", Row->AugmentTag.ToString()),
			           ("Definition", Row->Definition.ToSoftObjectPath().ToString())
			);
			continue;
		}

		const FPrimaryAssetId DefId = Definition->GetPrimaryAssetId();
		if (!DefId.IsValid())
		{
			NS_OBJ_LOG(LogNS, Warning,
			           "증강 Definition에서 유효한 DefId를 만들지 못했습니다. RowName={RowName}, AugmentTag={AugmentTag}",
			           ("RowName", RowName.ToString()),
			           ("AugmentTag", Row->AugmentTag.ToString())
			);
			continue;
		}

		const FNSDefinitionMeta* ExistingDefinition = DefinitionMeta.Find(DefId);

		if (!ExistingDefinition)
		{
			FNSDefinitionMeta NewDefinitionMeta;
			NewDefinitionMeta.FirstRowName = RowName;
			NewDefinitionMeta.AugmentTag = Row->AugmentTag;

			DefinitionMeta.Add(DefId, NewDefinitionMeta);
		}
		else if (ExistingDefinition->AugmentTag != Row->AugmentTag
			&& !ReportedDefinitionConflicts.Contains(DefId))
		{
			NS_OBJ_LOG(LogNS, Warning,
			           "같은 Definition이 서로 다른 AugmentTag에 연결되어 있습니다. DefId={DefId}, 기준 Row={BaseRowName}, 기준 AugmentTag={BaseAugmentTag}, 불일치 Row={RowName}, 불일치 AugmentTag={AugmentTag}",
			           ("DefId", DefId.ToString()),
			           ("BaseRowName", ExistingDefinition->FirstRowName.ToString()),
			           ("BaseAugmentTag", ExistingDefinition->AugmentTag.ToString()),
			           ("RowName", RowName.ToString()),
			           ("AugmentTag", Row->AugmentTag.ToString())
			);

			ReportedDefinitionConflicts.Add(DefId);
		}

		const FNSAugmentGroupMeta* ExistingGroup = GroupMetas.Find(Row->AugmentTag);
		if (!ExistingGroup)
		{
			FNSAugmentGroupMeta NewGroup;
			NewGroup.FirstRowName = RowName;
			NewGroup.DefId = DefId;
			NewGroup.OwnerCharacterTag = Row->OwnerCharacterTag;
			NewGroup.Rarity = Row->Rarity;
			NewGroup.MaxStack = Row->MaxStack;
			NewGroup.SelectionWeight = Row->SelectionWeight;
			NewGroup.bCountAsLegendarySlot = Row->bCountAsLegendarySlot;

			GroupMetas.Add(Row->AugmentTag, NewGroup);
			continue;
		}

		if (ExistingGroup->DefId != DefId)
		{
			NS_OBJ_LOG(LogNS, Warning,
			           "같은 AugmentTag 그룹의 Definition이 일치하지 않습니다. AugmentTag={AugmentTag}, 기준 Row={BaseRowName}, 기준 DefId={BaseDefId}, 불일치 Row={RowName}, 불일치 DefId={DefId}",
			           ("AugmentTag", Row->AugmentTag.ToString()),
			           ("BaseRowName", ExistingGroup->FirstRowName.ToString()),
			           ("BaseDefId", ExistingGroup->DefId.ToString()),
			           ("RowName", RowName.ToString()),
			           ("DefId", DefId.ToString())
			);
		}

		if (ExistingGroup->OwnerCharacterTag != Row->OwnerCharacterTag)
		{
			NS_OBJ_LOG(LogNS, Warning,
			           "같은 AugmentTag 그룹의 OwnerCharacterTag가 일치하지 않습니다. AugmentTag={AugmentTag}, 기준 Row={BaseRowName}, 기준 OwnerCharacterTag={BaseOwnerCharacterTag}, 불일치 Row={RowName}, 불일치 OwnerCharacterTag={OwnerCharacterTag}",
			           ("AugmentTag", Row->AugmentTag.ToString()),
			           ("BaseRowName", ExistingGroup->FirstRowName.ToString()),
			           ("BaseOwnerCharacterTag", ExistingGroup->OwnerCharacterTag.ToString()),
			           ("RowName", RowName.ToString()),
			           ("OwnerCharacterTag", Row->OwnerCharacterTag.ToString())
			);
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

		if (ExistingGroup->bCountAsLegendarySlot != Row->bCountAsLegendarySlot)
		{
			NS_OBJ_LOG(LogNS, Warning,
			           "같은 AugmentTag 그룹의 bCountAsLegendarySlot이 일치하지 않습니다. AugmentTag={AugmentTag}, 기준 Row={BaseRowName}, 기준 값={BaseValue}, 불일치 Row={RowName}, 불일치 값={Value}",
			           ("AugmentTag", Row->AugmentTag.ToString()),
			           ("BaseRowName", ExistingGroup->FirstRowName.ToString()),
			           ("BaseValue", ExistingGroup->bCountAsLegendarySlot),
			           ("RowName", RowName.ToString()),
			           ("Value", Row->bCountAsLegendarySlot)
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
	CurrentOfferRerollCount = 0;
	CurrentAvailableCardCount = 0;

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

void UNSAugmentSelectionComponent::Client_NotifyRerollResult_Implementation(
	ENSAugmentRerollResult Result,
	int64 RequiredCost,
	int64 HaveCurrency,
	int32 RequestRevision,
	int32 CurrentOfferRevision)
{
	OnRerollResult.Broadcast(Result, RequiredCost, HaveCurrency, RequestRevision, CurrentOfferRevision);
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
		UIManager->OpenAugmentSelectionPanel();
	}
}

UNSCurrencyComponent* UNSAugmentSelectionComponent::GetOwnerCurrencyComponent() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	const ANSPlayerState* PlayerState = Cast<ANSPlayerState>(PlayerController->PlayerState);
	if (!IsValid(PlayerState))
	{
		return nullptr;
	}

	return PlayerState->GetCurrencyComponent();
}

UNSPlayerProgressComponent* UNSAugmentSelectionComponent::GetOwnerProgressComponent() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	const ANSPlayerState* PlayerState = Cast<ANSPlayerState>(PlayerController->PlayerState);
	if (!IsValid(PlayerState))
	{
		return nullptr;
	}

	return PlayerState->GetProgressComponent();
}

int32 UNSAugmentSelectionComponent::GetEffectiveCardsCount() const
{
	const UNSPlayerProgressComponent* ProgressComp = GetOwnerProgressComponent();
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);

	const int32 AddBonus = NSCommonUpgradeUtility::GetAddBonus(
		DataSubsystem, ProgressComp, NSCommonUpgradeUtility::NodeId_AugmentChoiceCount);

	return FMath::Clamp(BaseCardsCount + AddBonus, BaseCardsCount, MaxCardsCount);
}

bool UNSAugmentSelectionComponent::TryFindRarityRule(
	UNSDataSubsystem* Data,
	const FGameplayTag& RewardTriggerTag,
	FNSAugmentRarityRule& OutRule) const
{
	OutRule = FNSAugmentRarityRule();

	const UNSAugmentRarityRuleSet* RarityRuleSet =
		Data ? Data->GetCurrentAugmentRarityRuleSet() : nullptr;

	if (!IsValid(RarityRuleSet))
	{
		NS_OBJ_LOG(LogNS, Warning, "현재 RunConfig의 증강 희귀도 규칙 세트가 설정되지 않았습니다.");
		return false;
	}

	if (!RewardTriggerTag.IsValid())
	{
		return false;
	}

	for (const FNSAugmentRarityRule& Rule : RarityRuleSet->RarityRules)
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

// IsDataValid는 에디터 시점 검증일 뿐이며, 런타임에는 그 이후 잘못 수정된 값이 그대로 로드될 수 있음.
// InitialCost=0 같은 설정 오류가 NotEnoughCurrency로 오인되지 않도록 필드까지 다시 검증.
bool UNSAugmentSelectionComponent::TryFindRerollRule(
	const FGameplayTag& RewardTriggerTag, FNSAugmentRerollRule& OutRule) const
{
	OutRule = FNSAugmentRerollRule();

	UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	const UNSRunConfig* RunConfig = DataSubsystem ? DataSubsystem->GetCurrentRunConfig() : nullptr;
	if (!RunConfig || !RewardTriggerTag.IsValid())
	{
		return false;
	}

	static const FGameplayTag RewardTriggerRoot =
		FGameplayTag::RequestGameplayTag(FName(TEXT("Reward.Trigger")), false);

	for (const FNSAugmentRerollRule& Rule : RunConfig->AugmentRerollRules)
	{
		if (Rule.RewardTriggerTag != RewardTriggerTag)
		{
			continue;
		}

		const bool bHierarchyValid =
			!RewardTriggerRoot.IsValid() || Rule.RewardTriggerTag.MatchesTag(RewardTriggerRoot);

		if (!bHierarchyValid || Rule.InitialCost < 1
			|| !FMath::IsFinite(Rule.CostMultiplier) || Rule.CostMultiplier < 1.0f)
		{
			NS_OBJ_LOG(LogNS, Warning,
			           "증강 리롤 규칙 필드가 유효하지 않습니다. RewardTriggerTag={RewardTriggerTag}",
			           ("RewardTriggerTag", RewardTriggerTag.ToString())
			);
			return false;
		}

		OutRule = Rule;
		return true;
	}

	NS_OBJ_LOG(LogNS, Warning,
	           "증강 리롤 규칙을 찾지 못했습니다(리롤 불가 트리거). RewardTriggerTag={RewardTriggerTag}",
	           ("RewardTriggerTag", RewardTriggerTag.ToString())
	);
	return false;
}

int64 UNSAugmentSelectionComponent::ComputeRerollCost(const FNSAugmentRerollRule& Rule, int32 Count)
{
	// 2^53 미만이면 double의 정수 표현이 정확해 ceil 결과가 항상 안전.
	constexpr double SafeCeiling = 1.0e15;

	if (Count <= 0)
	{
		return FMath::Max<int64>(0, Rule.InitialCost);
	}

	const double Base = static_cast<double>(FMath::Max<int64>(0, Rule.InitialCost));
	const double Mult = FMath::Max(1.0, static_cast<double>(Rule.CostMultiplier));
	const double Raw = Base * FMath::Pow(Mult, static_cast<double>(Count));

	if (!FMath::IsFinite(Raw) || Raw >= SafeCeiling)
	{
		return static_cast<int64>(SafeCeiling);
	}
	return static_cast<int64>(FMath::CeilToDouble(Raw));
}

int64 UNSAugmentSelectionComponent::GetDiscountedRerollCost(const FNSAugmentRerollRule& Rule, int32 Count) const
{
	// 리롤 비용 계산: ceil(InitialCost * CostMultiplier^Count). overflow/정밀도 안전 상한 포함.
	// 할인 전 원가라 직접 부르면 안 되고, 실제로 쓸 땐 GetDiscountedRerollCost()를 거쳐야 함.
	const int64 BaseCost = ComputeRerollCost(Rule, Count);

	const UNSPlayerProgressComponent* ProgressComp = GetOwnerProgressComponent();
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);

	const double Percent = NSCommonUpgradeUtility::GetPercent(
		DataSubsystem, ProgressComp, NSCommonUpgradeUtility::NodeId_AugmentRerollDiscount);
	return NSCommonUpgradeUtility::ApplyPercentAsCost(BaseCost, Percent);
}

TArray<FNSAugmentSelectionCard>
UNSAugmentSelectionComponent::RollCardsExcludingComposition(
	const FNSAugmentRarityRule& RarityRule,
	int32 N,
	const TSet<FPrimaryAssetId>& PreviousDefIds,
	bool& bOutDifferentPossible,
	int32& OutAvailableCardCount) const
{
	bOutDifferentPossible = false;
	OutAvailableCardCount = 0;

	// 직전 오퍼가 완전한 N장이 아니면(부분 오퍼) 리롤 대상이 아니다.
	if (N <= 0 || PreviousDefIds.Num() != N)
	{
		return {};
	}

	UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem || !DataSubsystem->IsRunReady())
	{
		return {};
	}

	bool bLegendaryFull = false;
	TSet<FPrimaryAssetId> OwnedLegendarySlotIds;
	TSet<FPrimaryAssetId> StackFullIds;
	CollectInventoryFilter(bLegendaryFull, OwnedLegendarySlotIds, StackFullIds);

	// 기존 카드 일부 재등장은 허용하므로 PreviousDefIds를 후보 풀에서 제외하지 않음.
	const TSet<FPrimaryAssetId> EmptyExcluded;

	TMap<ENSAugmentRarity, TArray<FNSAugmentCandidate>> ByRarity;
	BuildRarityBuckets(DataSubsystem, bLegendaryFull, OwnedLegendarySlotIds, StackFullIds, EmptyExcluded, ByRarity);

	// 후보 풀을 DefId 집합 + 메타 맵으로 평탄화. 이 트리거에서 가중치가 0(또는 미설정)인 희귀도는 제외.
	TSet<FPrimaryAssetId> CandidateDefIds;
	TMap<FPrimaryAssetId, FNSAugmentCandidate> CandidatesByDefId;

	for (const TPair<
		     ENSAugmentRarity,
		     TArray<FNSAugmentCandidate>>& RarityBucket :
	     ByRarity)
	{
		const int32* RarityWeight =
			RarityRule.RarityWeights.Find(
				RarityBucket.Key);

		if (!RarityWeight ||
			*RarityWeight <= 0)
		{
			continue;
		}

		for (const FNSAugmentCandidate& Candidate :
		     RarityBucket.Value)
		{
			if (Candidate.SelectionWeight <= 0 ||
				!Candidate.DefId.IsValid())
			{
				continue;
			}

			CandidateDefIds.Add(Candidate.DefId);
			CandidatesByDefId.Add(
				Candidate.DefId,
				Candidate);
		}
	}

	OutAvailableCardCount =
		CandidateDefIds.Num();

	// 후보 수가 N 이하이면 직전과 다른 N장 구성이 존재할 수 없다.
	if (CandidateDefIds.Num() <= N)
	{
		return {};
	}

	// 자연스러운 기존 확률 분포를 최대한 유지하기 위해 정상 추첨을 K회 시도.
	constexpr int32 K = 8;
	TArray<FNSAugmentSelectionCard> LastFullOffer;

	for (int32 Attempt = 0; Attempt < K; ++Attempt)
	{
		TArray<FNSAugmentSelectionCard> Candidate = DrawCards(RarityRule, ByRarity, N);
		if (Candidate.Num() != N)
		{
			continue;
		}

		LastFullOffer = Candidate;

		TSet<FPrimaryAssetId> CandidateSet;
		for (const FNSAugmentSelectionCard& Card : Candidate)
		{
			CandidateSet.Add(Card.DefId);
		}

		if (!AreDefIdSetsEqual(CandidateSet, PreviousDefIds))
		{
			bOutDifferentPossible = true;
			return Candidate;
		}
	}

	// 여기까지 왔다는 건 K번을 뽑아도 계속 직전과 같은 조합만 나왔다는 뜻.
	// 마지막으로 뽑은 오퍼에서 카드 한 장만 강제로 바꿔서 다른 조합을 만듬.
	if (LastFullOffer.Num() != N)
	{
		return {};
	}

	TArray<FPrimaryAssetId> OutsideCandidates;
	for (const FPrimaryAssetId& Id : CandidateDefIds)
	{
		if (!PreviousDefIds.Contains(Id))
		{
			OutsideCandidates.Add(Id);
		}
	}

	if (OutsideCandidates.IsEmpty())
	{
		return {};
	}

	// OutsideCandidates 중에서 SelectionWeight 비율대로 하나를 무작위로 뽑아 치환 후보로 사용.
	int32 TotalOutsideWeight = 0;
	for (const FPrimaryAssetId& Id : OutsideCandidates)
	{
		if (const FNSAugmentCandidate* Candidate = CandidatesByDefId.Find(Id))
		{
			TotalOutsideWeight += FMath::Max(0, Candidate->SelectionWeight);
		}
	}

	if (TotalOutsideWeight <= 0)
	{
		return {};
	}

	const int32 RollValue = FMath::RandRange(1, TotalOutsideWeight);
	int32 Accumulated = 0;
	FPrimaryAssetId ChosenDefId;
	for (const FPrimaryAssetId& Id : OutsideCandidates)
	{
		const FNSAugmentCandidate* Candidate = CandidatesByDefId.Find(Id);
		if (!Candidate)
		{
			continue;
		}
		Accumulated += FMath::Max(0, Candidate->SelectionWeight);
		if (RollValue <= Accumulated)
		{
			ChosenDefId = Id;
			break;
		}
	}

	const FNSAugmentCandidate* ChosenCandidate = ChosenDefId.IsValid() ? CandidatesByDefId.Find(ChosenDefId) : nullptr;
	if (!ChosenCandidate)
	{
		return {};
	}

	// 동일 희귀도 슬롯이 있으면 그 슬롯을 우선 교체해 자연 추첨 결과의 희귀도 구성을 최대한 보존.
	// 없으면 0번 슬롯을 교체.
	int32 ReplaceIndex = 0;
	for (int32 Index = 0; Index < LastFullOffer.Num(); ++Index)
	{
		if (LastFullOffer[Index].Rarity == ChosenCandidate->Rarity)
		{
			ReplaceIndex = Index;
			break;
		}
	}

	TArray<FNSAugmentSelectionCard> NewOffer = LastFullOffer;
	NewOffer[ReplaceIndex].DefId = ChosenCandidate->DefId;
	NewOffer[ReplaceIndex].Rarity = ChosenCandidate->Rarity;

	// 방금 한 장을 강제로 바꿔치기했으니, 결과가 여전히 N장인지 / 직전과 다른 구성인지 / 카드가 중복되지 않는지 마지막으로 다시 확인.
	TSet<FPrimaryAssetId> NewOfferDefIds;
	for (const FNSAugmentSelectionCard& Card : NewOffer)
	{
		NewOfferDefIds.Add(Card.DefId);
	}

	if (NewOffer.Num() != N
		|| AreDefIdSetsEqual(NewOfferDefIds, PreviousDefIds)
		|| NewOfferDefIds.Num() != NewOffer.Num())
	{
		return {};
	}

	bOutDifferentPossible = true;
	return NewOffer;
}

TArray<FNSAugmentSelectionCard>
UNSAugmentSelectionComponent::RollCards(
	const FNSAugmentRarityRule& RarityRule,
	int32 N,
	int32& OutAvailableCardCount) const
{
	OutAvailableCardCount = 0;

	const int32 RequestedCount =
		FMath::Clamp(N, 0, MaxCardsCount);

	if (RequestedCount <= 0)
	{
		return {};
	}

	UNSDataSubsystem* Data =
		UNSDataSubsystem::Get(this);

	if (!Data || !Data->IsRunReady())
	{
		return {};
	}

	bool bLegendaryFull = false;

	TSet<FPrimaryAssetId> OwnedLegendarySlotIds;
	TSet<FPrimaryAssetId> StackFullIds;

	CollectInventoryFilter(
		bLegendaryFull,
		OwnedLegendarySlotIds,
		StackFullIds);

	const TSet<FPrimaryAssetId> EmptyExcluded;

	TMap<
		ENSAugmentRarity,
		TArray<FNSAugmentCandidate>> ByRarity;

	BuildRarityBuckets(
		Data,
		bLegendaryFull,
		OwnedLegendarySlotIds,
		StackFullIds,
		EmptyExcluded,
		ByRarity);

	OutAvailableCardCount =
		CountSelectableCandidates(
			RarityRule,
			ByRarity);

	const int32 ActualDrawCount =
		FMath::Min(
			RequestedCount,
			OutAvailableCardCount);

	if (ActualDrawCount <= 0)
	{
		return {};
	}

	return DrawCards(
		RarityRule,
		ByRarity,
		ActualDrawCount);
}

bool UNSAugmentSelectionComponent::AreDefIdSetsEqual(const TSet<FPrimaryAssetId>& A, const TSet<FPrimaryAssetId>& B)
{
	if (A.Num() != B.Num())
	{
		return false;
	}

	for (const FPrimaryAssetId& Id : A)
	{
		if (!B.Contains(Id))
		{
			return false;
		}
	}

	return true;
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

	UDataTable* AugmentDefinitionTable = Data ? Data->GetCurrentAugmentDefinitionTable() : nullptr;
	if (!IsValid(AugmentDefinitionTable))
	{
		NS_OBJ_LOG(LogNS, Warning, "현재 RunConfig의 증강 효과 정의 DataTable이 설정되지 않았습니다.");
		return false;
	}

	if (!DefId.IsValid())
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

	UDataTable* AugmentDefinitionTable = Data ? Data->GetCurrentAugmentDefinitionTable() : nullptr;
	if (!IsValid(AugmentDefinitionTable))
	{
		NS_OBJ_LOG(LogNS, Warning, "현재 RunConfig의 증강 효과 정의 DataTable이 설정되지 않았습니다.");
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

int32 UNSAugmentSelectionComponent::CountSelectableCandidates(
	const FNSAugmentRarityRule& RarityRule,
	const TMap<
		ENSAugmentRarity,
		TArray<FNSAugmentCandidate>>& ByRarity) const
{
	TSet<FPrimaryAssetId> SelectableDefIds;

	for (const TPair<ENSAugmentRarity, int32>& RarityWeight :
	     RarityRule.RarityWeights)
	{
		if (RarityWeight.Value <= 0)
		{
			continue;
		}

		const TArray<FNSAugmentCandidate>* Bucket =
			ByRarity.Find(RarityWeight.Key);

		if (!Bucket)
		{
			continue;
		}

		for (const FNSAugmentCandidate& Candidate : *Bucket)
		{
			if (Candidate.SelectionWeight <= 0 ||
				!Candidate.DefId.IsValid())
			{
				continue;
			}

			SelectableDefIds.Add(Candidate.DefId);
		}
	}

	return SelectableDefIds.Num();
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

	TMap<ENSAugmentRarity, TArray<FNSAugmentCandidate>> RemainingByRarity = ByRarity;
	Result.Reserve(N);

	auto HasSelectableCandidate =
		[](const TArray<FNSAugmentCandidate>& Bucket)
	{
		for (const FNSAugmentCandidate& Candidate : Bucket)
		{
			if (Candidate.SelectionWeight > 0)
			{
				return true;
			}
		}

		return false;
	};

	auto MakeCard =
		[this](const FNSAugmentCandidate& Picked)
	{
		const UEnum* RarityEnum = StaticEnum<ENSAugmentRarity>();

		const FString RarityName = RarityEnum
			                           ? RarityEnum->GetNameStringByValue(static_cast<int64>(Picked.Rarity))
			                           : TEXT("Invalid");

		NS_OBJ_LOG(LogNS, Log,
		           "증강 카드 후보를 선택했습니다. Rarity={Rarity}, DefId={DefId}, Weight={Weight}",
		           ("Rarity", RarityName),
		           ("DefId", Picked.DefId.ToString()),
		           ("Weight", Picked.SelectionWeight));

		FNSAugmentSelectionCard Card;
		Card.DefId = Picked.DefId;
		Card.Rarity = Picked.Rarity;
		return Card;
	};

	auto DrawFromBucket =
		[&MakeCard](TArray<FNSAugmentCandidate>& Bucket, FNSAugmentSelectionCard& OutCard)
	{
		int32 TotalSelectionWeight = 0;

		for (const FNSAugmentCandidate& Candidate : Bucket)
		{
			TotalSelectionWeight += FMath::Max(0, Candidate.SelectionWeight);
		}

		if (TotalSelectionWeight <= 0)
		{
			return false;
		}

		const int32 RollValue = FMath::RandRange(1, TotalSelectionWeight);

		int32 AccumulatedWeight = 0;
		int32 PickIndex = INDEX_NONE;

		for (int32 Index = 0; Index < Bucket.Num(); ++Index)
		{
			AccumulatedWeight += FMath::Max(0, Bucket[Index].SelectionWeight);

			if (RollValue <= AccumulatedWeight)
			{
				PickIndex = Index;
				break;
			}
		}

		if (PickIndex == INDEX_NONE)
		{
			return false;
		}

		const FNSAugmentCandidate Picked = Bucket[PickIndex];
		Bucket.RemoveAtSwap(PickIndex);

		OutCard = MakeCard(Picked);
		return true;
	};

	auto TryDrawByRarityRule =
		[&]()
	{
		FNSAugmentSelectionCard DrawnCard;

		int32 TotalRarityWeight = 0;

		for (const TPair<ENSAugmentRarity, int32>& RarityWeight : RarityRule.RarityWeights)
		{
			const int32 Weight = FMath::Max(0, RarityWeight.Value);
			if (Weight <= 0)
			{
				continue;
			}

			const TArray<FNSAugmentCandidate>* Bucket = RemainingByRarity.Find(RarityWeight.Key);
			if (!Bucket || !HasSelectableCandidate(*Bucket))
			{
				continue;
			}

			TotalRarityWeight += Weight;
		}

		if (TotalRarityWeight <= 0)
		{
			return TOptional<FNSAugmentSelectionCard>();
		}

		const int32 RollValue = FMath::RandRange(1, TotalRarityWeight);

		int32 AccumulatedWeight = 0;
		ENSAugmentRarity ChosenRarity = ENSAugmentRarity::Common;
		bool bFoundRarity = false;

		for (const TPair<ENSAugmentRarity, int32>& RarityWeight : RarityRule.RarityWeights)
		{
			const int32 Weight = FMath::Max(0, RarityWeight.Value);
			if (Weight <= 0)
			{
				continue;
			}

			const TArray<FNSAugmentCandidate>* Bucket = RemainingByRarity.Find(RarityWeight.Key);
			if (!Bucket || !HasSelectableCandidate(*Bucket))
			{
				continue;
			}

			AccumulatedWeight += Weight;

			if (RollValue <= AccumulatedWeight)
			{
				ChosenRarity = RarityWeight.Key;
				bFoundRarity = true;
				break;
			}
		}

		if (!bFoundRarity)
		{
			return TOptional<FNSAugmentSelectionCard>();
		}

		TArray<FNSAugmentCandidate>* SelectedBucket = RemainingByRarity.Find(ChosenRarity);
		if (!SelectedBucket || !DrawFromBucket(*SelectedBucket, DrawnCard))
		{
			return TOptional<FNSAugmentSelectionCard>();
		}

		return TOptional<FNSAugmentSelectionCard>(DrawnCard);
	};

	for (int32 CardIndex = 0;
	     CardIndex < N;
	     ++CardIndex)
	{
		const TOptional<FNSAugmentSelectionCard> DrawnCard =
			TryDrawByRarityRule();

		if (!DrawnCard.IsSet())
		{
			break;
		}

		Result.Add(DrawnCard.GetValue());
	}

	return Result;
}
