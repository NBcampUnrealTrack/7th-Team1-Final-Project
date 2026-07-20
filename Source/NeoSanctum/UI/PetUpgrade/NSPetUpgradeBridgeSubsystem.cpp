// Copyright 2026 One Team. All rights reserved.


#include "NSPetUpgradeBridgeSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"
#include "NeoSanctum/Type/NSPetUpgradeMessageTypes.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSProgressionSubsystem.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/AI/NSCompanionDefinition.h"
#include "NeoSanctum/Character/Component/NSCompanionProgressionComponent.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Data/AI/NSCompanionCatalog.h"
#include "NeoSanctum/Tag/NSGameplayTags_Companion.h"
#include "GameplayEffect.h"
#include "Subsystems/SubsystemCollection.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"


void UNSPetUpgradeBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// 브리지보다 GMS가 먼저 초기화되도록 의존성 순서를 보장
	Collection.InitializeDependency<UGameplayMessageSubsystem>();
	
	UGameplayMessageSubsystem& MessageSubsystem = 
		UGameplayMessageSubsystem::Get(this);
	
	// UI에서 방송하는 펫 강화 상태 조회 메시지를 구독
	QueryListenerHandle =
		MessageSubsystem.RegisterListener<FNSPetUpgradeQueryMessage>(
			NSGameplayTags::Message_UI_PetUpgrade_Query,
			this,
			&ThisClass::HandleQueryMessage);
	
	// UI에서 방송하는 특정 노드 강화 요청 메시지를 구독
	UpgradeRequestListenerHandle =
		MessageSubsystem.RegisterListener<FNSPetUpgradeRequestMessage>(
			NSGameplayTags::Message_UI_PetUpgrade_Upgrade_Request,
			this,
			&ThisClass::HandleUpgradeRequestMessage);
	
	// @민재 추가
	SelectRequestListenerHandle =
	MessageSubsystem.RegisterListener<FNSPetUpgradeSelectRequestMessage>(
		NSGameplayTags::Message_UI_PetUpgrade_Select_Request,
		this,
		&ThisClass::HandleSelectRequestMessage);
}

void UNSPetUpgradeBridgeSubsystem::Deinitialize()
{
	// GameInstance 종료 또는 PIE 종료 시 중복 호출을 방지하기 위해 구독 해제
	QueryListenerHandle.Unregister();
	UpgradeRequestListenerHandle.Unregister();
	
	// @민재 추가
	SelectRequestListenerHandle.Unregister();
	
	Super::Deinitialize();
}

static TArray<FNSCompanionStatEntry> ReadDroneBaseStats(const UNSCompanionDefinition* Def)
{
	TArray<FNSCompanionStatEntry> Out;
	if (!Def || !Def->TypeStatsEffect) { return Out; }

	const UGameplayEffect* GE = Def->TypeStatsEffect->GetDefaultObject<UGameplayEffect>();
	if (!GE) { return Out; }

	for (const FGameplayModifierInfo& Mod : GE->Modifiers)
	{
		float Value = 0.f;
		// 기본 스탯 GE는 보통 ScalableFloat라 정적 값 읽기 가능
		Mod.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.f, Value);

		FNSCompanionStatEntry Entry;
		Entry.Name  = FText::FromString(Mod.Attribute.GetName());
		Entry.Value = Value;
		Out.Add(Entry);
	}
	return Out;
}

void UNSPetUpgradeBridgeSubsystem::HandleQueryMessage(FGameplayTag Channel, const FNSPetUpgradeQueryMessage& Message)
{
	// 영구 진행도 데이터를 관리하는 GameInstance가 없으면 조회 중단
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}
	// UI가 직접 접근하지 않도록 브리지에서 진행도 시스템 조회
	UNSProgressionSubsystem* ProgressionSubsystem =
		GameInstance->GetSubsystem<UNSProgressionSubsystem>();
	
	if (!ProgressionSubsystem)
	{
		return;
	}
	// 응답을 요청 UI와 연결할 수 있도록 동일한 RequestId 사용
	FNSPetUpgradeSnapshotMessage Snapshot;
	Snapshot.RequestId = Message.RequestId;
	// 요청에 펫 태그가 없다면 저장된 현재 선택 펫을 사용
	Snapshot.CompanionTag = Message.CompanionTag.IsValid()
		? Message.CompanionTag
		: ProgressionSubsystem->GetSelectedCompanion();
	// 펫 강화 화면에 표시할 현재 공용 재화 조회
	Snapshot.CurrentCurrency =
		ProgressionSubsystem->GetCommonCurrency();

	// 현재 로컬 플레이어가 사용하는 PlayerState 조회
	const UWorld* World = GetWorld();
	APlayerController* PlayerController =
		World ? World->GetFirstPlayerController() : nullptr;

	ANSPlayerState* PlayerState =
		PlayerController
			? PlayerController->GetPlayerState<ANSPlayerState>()
			: nullptr;
	
	// 저장 데이터에 선택된 펫이 없을 때 기본 펫 태그를 구하기 위해 조회
	UNSCompanionDefinition* CompanionDefinition =
		PlayerState
			? PlayerState->GetCurrentCompanionDefinition()
			: nullptr;
	
	if (CompanionDefinition && !Snapshot.CompanionTag.IsValid())
	{
		Snapshot.CompanionTag = CompanionDefinition->CompanionTag;
	}
	// 전체 펫 강화 트리가 등록된 Catalog 접근
	UNSCompanionProgressionComponent* CompanionComponent =
	PlayerState
		? PlayerState->GetCompanionProgressionComponent()
		: nullptr;

	const UNSCompanionCatalog* CompanionCatalog =
		CompanionComponent
			? CompanionComponent->Catalog
			: nullptr;

	if (CompanionCatalog)
	{
		// 실제 선택된 드론(저장 기준). 조회 대상(Snapshot.CompanionTag)과 분리한다.
		// 저장값이 없으면 기본 드론(GetCurrentCompanionDefinition의 폴백)으로 간주.
		FGameplayTag SelectedTag = ProgressionSubsystem->GetSelectedCompanion();
		if (!SelectedTag.IsValid() && CompanionDefinition)
		{
			SelectedTag = CompanionDefinition->CompanionTag;
		}

		if (const UNSCompanionDefinition* SelectedDef = CompanionCatalog->FindByTag(SelectedTag))
		{
			Snapshot.SelectedDisplayName = SelectedDef->DisplayName;
			Snapshot.SelectedIcon        = SelectedDef->Icon;
		}
		
		for (const TObjectPtr<UNSCompanionDefinition>& Definition : CompanionCatalog->Companions)
		{
			if (!IsValid(Definition))
			{
				continue;
			}

			// 이 드론의 해금 여부 = 선행 드론의 모든 노드가 Max인지
			const UNSCompanionDefinition* RequiredDrone =
				CompanionCatalog->FindByTag(Definition->RequiredCompanionTag);
			const bool bUnlocked = ProgressionSubsystem->CanSelectCompanion(RequiredDrone);
			const bool bSelected = (Definition->CompanionTag == SelectedTag);
			const bool bDroneOwned =
		!Definition->RequiredCompanionTag.IsValid()
		|| ProgressionSubsystem->IsCompanionUnlocked(Definition->CompanionTag);

			// (1) 드론 선택 노드 (Definition 파생)
			{
				FNSPetUpgradeNodeViewData& DroneView = Snapshot.Nodes.AddDefaulted_GetRef();
				DroneView.CompanionTag       = Definition->CompanionTag;
				DroneView.NodeTag            = Definition->CompanionTag; // 선택 노드 식별자 = 드론 태그
				DroneView.bIsDroneSelectNode = true;
				DroneView.DisplayName        = Definition->DisplayName;
				DroneView.Icon               = Definition->Icon;
				DroneView.DroneStats = ReadDroneBaseStats(Definition);
				DroneView.Description = Definition->Description;
				
				// 해금을 1레벨 업그레이드로 취급 → 0/1 표시
				const bool bDroneUnlocked = ProgressionSubsystem->IsCompanionUnlocked(Definition->CompanionTag);
				DroneView.MaxLevel     = 1;
				DroneView.CurrentLevel = bDroneUnlocked ? 1 : 0;
				DroneView.UpgradeCost  = bDroneUnlocked ? 0 : Definition->UnlockCost;
				
				DroneView.StateTag =
					bSelected  ? NSGameplayTags::UI_PetUpgrade_State_Selected
					: bUnlocked ? NSGameplayTags::UI_PetUpgrade_State_Selectable
					:             NSGameplayTags::UI_PetUpgrade_State_Locked;
			}

			// (2) 스탯 노드들
			for (const FNSCompanionUpgradeNode& UpgradeNode : Definition->UpgradeNodes)
			{
				FNSPetUpgradeNodeViewData& NodeView = Snapshot.Nodes.AddDefaulted_GetRef();
				NodeView.CompanionTag       = Definition->CompanionTag;
				NodeView.NodeTag            = UpgradeNode.NodeTag;
				NodeView.bIsDroneSelectNode = false;
				NodeView.CurrentLevel       = ProgressionSubsystem->GetCompanionNodeLevel(
					Definition->CompanionTag, UpgradeNode.NodeTag, UpgradeNode.bSharedAcrossDrones);
				NodeView.MaxLevel           = UpgradeNode.MaxLevel;
				NodeView.IncreasePerLevel = UpgradeNode.MagnitudePerLevel;
				NodeView.DisplayName = UpgradeNode.DisplayName;
				NodeView.Description = UpgradeNode.Description;
				NodeView.UpgradeCost = UpgradeNode.BaseCost + UpgradeNode.CostPerLevel * NodeView.CurrentLevel;
				NodeView.Icon     = UpgradeNode.Icon; 
				NodeView.StateTag =
					!bDroneOwned                                  ? NSGameplayTags::UI_PetUpgrade_State_Locked
					: (NodeView.CurrentLevel >= NodeView.MaxLevel) ? NSGameplayTags::UI_PetUpgrade_State_Maxed
					:                                             NSGameplayTags::UI_PetUpgrade_State_Upgradable;
			}
		}
	}
	// 완성된 전체 트리 상태를 요청한 UI에 전달
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_PetUpgrade_Snapshot,
		Snapshot);

}

void UNSPetUpgradeBridgeSubsystem::HandleUpgradeRequestMessage(FGameplayTag Channel,
	const FNSPetUpgradeRequestMessage& Message)
{
	//강화 비용과 조건이 확정된 다음에 구현
	
	// 성공과 실패 모두 요청자에게 반환할 결과 메시지 생성
	FNSPetUpgradeResultMessage Result;
	Result.RequestId = Message.RequestId;
	Result.CompanionTag = Message.CompanionTag;
	Result.NodeTag = Message.NodeTag;

	UGameInstance* GameInstance = GetGameInstance();
	const UWorld* World = GetWorld();

	// 요청한 로컬 플레이어의 Companion Catalog 접근 경로 조회
	APlayerController* PlayerController =
		World ? World->GetFirstPlayerController() : nullptr;

	ANSPlayerState* PlayerState =
		PlayerController
			? PlayerController->GetPlayerState<ANSPlayerState>()
			: nullptr;

	UNSCompanionProgressionComponent* CompanionComponent =
		PlayerState
			? PlayerState->GetCompanionProgressionComponent()
			: nullptr;

	const UNSCompanionCatalog* CompanionCatalog =
		CompanionComponent
			? CompanionComponent->Catalog
			: nullptr;

	UNSProgressionSubsystem* ProgressionSubsystem =
		GameInstance
			? GameInstance->GetSubsystem<UNSProgressionSubsystem>()
			: nullptr;

	// 요청한 CompanionTag가 실제 Catalog에 등록되어 있는지 검증
	UNSCompanionDefinition* Definition =
		CompanionCatalog
			? CompanionCatalog->FindByTag(Message.CompanionTag)
			: nullptr;

	// 요청한 NodeTag가 해당 CompanionDefinition 소속인지 검증
	const FNSCompanionUpgradeNode* UpgradeNode =
		Definition
			? Definition->UpgradeNodes.FindByPredicate(
				[&Message](const FNSCompanionUpgradeNode& Node)
				{
					return Node.NodeTag == Message.NodeTag;
				})
			: nullptr;

	if (ProgressionSubsystem && Definition && UpgradeNode)
	{
		// UI를 신뢰하지 않고 서버측 재검사: 이 드론을 실제로 소유(구매)했는지
		const bool bDroneOwned =
			!Definition->RequiredCompanionTag.IsValid()
			|| ProgressionSubsystem->IsCompanionUnlocked(Definition->CompanionTag);

		if (bDroneOwned)
		{
			const int32 CurLevel = ProgressionSubsystem->GetCompanionNodeLevel(
				Message.CompanionTag, Message.NodeTag, UpgradeNode->bSharedAcrossDrones);
			const int64 Cost = UpgradeNode->BaseCost + UpgradeNode->CostPerLevel * CurLevel;

			Result.bSuccess = ProgressionSubsystem->UpgradeCompanionNode(
				Message.CompanionTag, Message.NodeTag,
				UpgradeNode->bSharedAcrossDrones, UpgradeNode->MaxLevel, Cost);
		}
	}

	if (Result.bSuccess)
	{
		if (ANSPlayerController* NSPC = Cast<ANSPlayerController>(PlayerController))
		{
			NSPC->UploadLocalProgress(NSPC->GetActiveCharacterIdForUpload());
		}
	}
	
	// 강화 성공 여부를 요청한 UI에 전달
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_PetUpgrade_Upgrade_Result,
		Result);
}

void UNSPetUpgradeBridgeSubsystem::HandleSelectRequestMessage(FGameplayTag Channel,
	const FNSPetUpgradeSelectRequestMessage& Message)
{
	FNSPetUpgradeResultMessage Result;
	Result.RequestId    = Message.RequestId;
	Result.CompanionTag = Message.CompanionTag;

	UGameInstance* GameInstance = GetGameInstance();
	const UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	ANSPlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<ANSPlayerState>() : nullptr;
	UNSCompanionProgressionComponent* CompanionComponent =
		PlayerState ? PlayerState->GetCompanionProgressionComponent() : nullptr;
	const UNSCompanionCatalog* CompanionCatalog =
		CompanionComponent ? CompanionComponent->Catalog : nullptr;
	UNSProgressionSubsystem* ProgressionSubsystem =
		GameInstance ? GameInstance->GetSubsystem<UNSProgressionSubsystem>() : nullptr;

	UNSCompanionDefinition* Definition =
		CompanionCatalog ? CompanionCatalog->FindByTag(Message.CompanionTag) : nullptr;

	if (ProgressionSubsystem && Definition)
	{
		// 선행 드론(전 노드 Max 조건)을 넘겨 게이트 재검사 후 선택
		const UNSCompanionDefinition* RequiredDrone =
			CompanionCatalog->FindByTag(Definition->RequiredCompanionTag);
		Result.bSuccess = ProgressionSubsystem->SelectCompanion(Message.CompanionTag, RequiredDrone, Definition->UnlockCost);
	}

	// 성공 시 아웃게임(허브) 드론에 즉시 반영
	if (Result.bSuccess)
	{
		if (ANSPlayerController* NSPC = Cast<ANSPlayerController>(PlayerController))
		{
			NSPC->UploadLocalProgress(NSPC->GetActiveCharacterIdForUpload());
		}
	}
	
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_PetUpgrade_Select_Result, Result);
}


