// Copyright 2026 One Team. All rights reserved.


#include "NSCompanionProgressionComponent.h"

#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "NeoSanctum/Data/AI/NSCompanionCatalog.h"
#include "NeoSanctum/Data/AI/NSCompanionDefinition.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"


	

void UNSCompanionProgressionComponent::SetOwnedCompanion(ANSBaseCompanionAI* Owner)
{
	if (!Owner) return;
		
	if (OwnedCompanion) return;
	OwnedCompanion = Owner;
}

void UNSCompanionProgressionComponent::ApplySelectedAndNodes(
	UNSCompanionDefinition* SelectedDefinition,
	const TMap<FGameplayTag, int32>& NodeLevels)
{
	if (!OwnedCompanion ||
		!GetOwner() ||
		!GetOwner()->HasAuthority())
	{
		return;
	}

	if (SelectedDefinition)
	{
		OwnedCompanion->ApplyDroneDefinition(
			SelectedDefinition);
	}
	// 현재 세이브에 존재하는 강화만 다시 적용
	ApplyNodeLevels(NodeLevels);
}

void UNSCompanionProgressionComponent::ApplyNodeLevels(const TMap<FGameplayTag, int32>& NodeLevels)
{
		if (!OwnedCompanion || !GetOwner() || !GetOwner()->HasAuthority())
		{
			return;
		}
		
		for (const TPair<FGameplayTag, int32>& NodeLevelPair : NodeLevels)
		{
			OwnedCompanion->ApplyStatUpgrade(NodeLevelPair.Key, NodeLevelPair.Value);
		}
}

void UNSCompanionProgressionComponent::Server_TryUpgrade_Implementation(
	FGameplayTag NodeTag)
{
	if (!NodeTag.IsValid() || !Catalog)
	{
		return;
	}

	ANSPlayerState* NSPlayerState =
		Cast<ANSPlayerState>(GetOwner());

	if (!NSPlayerState)
	{
		return;
	}

	UNSPlayerProgressComponent* ProgressComponent =
		NSPlayerState->GetProgressComponent();

	if (!ProgressComponent)
	{
		return;
	}

	UNSCompanionDefinition* NodeDefinition = nullptr;
	const FNSCompanionUpgradeNode* UpgradeNode = nullptr;

	//Catalog 전체에서 요청한 노드와 소유 Definition을 찾는다.
	for (UNSCompanionDefinition* Definition : Catalog->Companions)
	{
		if (!Definition)
		{
			continue;
		}

		UpgradeNode = Definition->UpgradeNodes.FindByPredicate(
			[NodeTag](const FNSCompanionUpgradeNode& Node)
			{
				return Node.NodeTag == NodeTag;
			});

		if (UpgradeNode)
		{
			NodeDefinition = Definition;
			break;
		}
	}

	if (!NodeDefinition || !UpgradeNode)
	{
		return;
	}

	//선행 펫 노드의 누적 강화 횟수를 검사한다.
	if (NodeDefinition->RequiredCompanionTag.IsValid() &&
		NodeDefinition->RequiredUpgradeCount > 0)
	{
		UNSCompanionDefinition* RequiredDefinition =
			Catalog->FindByTag(
				NodeDefinition->RequiredCompanionTag);

		if (!RequiredDefinition)
		{
			return;
		}

		int32 RequiredUpgradeTotal = 0;

		for (const FNSCompanionUpgradeNode& RequiredNode :
			RequiredDefinition->UpgradeNodes)
		{
			RequiredUpgradeTotal +=
				ProgressComponent->GetCompanionNodeLevel(
					RequiredNode.NodeTag);
		}

		if (RequiredUpgradeTotal <
			NodeDefinition->RequiredUpgradeCount)
		{
			return;
		}
	}
	
	const int32 CurrentLevel =
	ProgressComponent->GetCompanionNodeLevel(NodeTag);

	if (CurrentLevel >= UpgradeNode->MaxLevel)
	{
		return;
	}

	const int64 UpgradeCost =
		UpgradeNode->BaseUpgradeCost +
		(CurrentLevel * UpgradeNode->CostIncreasePerLevel);

	if (!ProgressComponent->TryPurchaseCompanionUpgrade(
		NodeTag,
		UpgradeNode->MaxLevel,
		UpgradeCost))
	{
		return;
	}

	//강화된 서버 데이터를 소유 클라이언트의 캐시 및 영구 세이브에 저장한다.
	if (ANSPlayerController* PlayerController =
		Cast<ANSPlayerController>(
			NSPlayerState->GetPlayerController()))
	{
		PlayerController->SaveProgressToOwningClient();
	}
}

UNSCompanionProgressionComponent::UNSCompanionProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}


