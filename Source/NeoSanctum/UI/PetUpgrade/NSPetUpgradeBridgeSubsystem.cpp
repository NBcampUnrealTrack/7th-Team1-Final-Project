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
#include "NeoSanctum/Data/AI/NSCompanionCatalog.h"
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
}

void UNSPetUpgradeBridgeSubsystem::Deinitialize()
{
	// GameInstance 종료 또는 PIE 종료 시 중복 호출을 방지하기 위해 구독 해제
	QueryListenerHandle.Unregister();
	UpgradeRequestListenerHandle.Unregister();
	
	Super::Deinitialize();
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
		// 선택된 펫 하나가 아니라 Catalog의 전체 펫 Definition을 순회
		for (const TObjectPtr<UNSCompanionDefinition>& Definition
			: CompanionCatalog->Companions)
		{
			if (!IsValid(Definition))
			{
				continue;
			}
			// 각 Definition이 보유한 모든 강화 노드를 UI ViewData로 변환
			for (const FNSCompanionUpgradeNode& UpgradeNode
				: Definition->UpgradeNodes)
			{
				FNSPetUpgradeNodeViewData& NodeViewData =
					Snapshot.Nodes.AddDefaulted_GetRef();
				// 강화 요청 시 올바른 펫 Definition을 찾기 위한 소유 태그
				NodeViewData.CompanionTag =
					Definition->CompanionTag;
				NodeViewData.NodeTag =
					UpgradeNode.NodeTag;
				// 저장된 현재 레벨과 Definition의 최대 레벨을 조합
				NodeViewData.CurrentLevel =
					ProgressionSubsystem->GetCompanionNodeLevel(
						UpgradeNode.NodeTag);
				NodeViewData.MaxLevel =
					UpgradeNode.MaxLevel;
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
		// UI 표시 상태를 신뢰하지 않고 기존 진행도 시스템에서 해금조건 재검사
		const bool bConditionMet =
			ProgressionSubsystem->CanSelectCompanion(
				Definition->RequiredCompanionTag,
				Definition->RequiredUpgradeCount);

		if (bConditionMet)
		{
			// 현재 기획은 무료 강화이므로 비용으로 0 전달
			// 최대 레벨 확인과 영구 저장은 ProgressionSubsystem이 처리
			Result.bSuccess =
				ProgressionSubsystem->UpgradeCompanionNode(
					Message.CompanionTag,
					Message.NodeTag,
					UpgradeNode->MaxLevel,
					0);
		}
	}

	// 강화 성공 여부를 요청한 UI에 전달
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_PetUpgrade_Upgrade_Result,
		Result);
}
