// Copyright 2026 One Team. All rights reserved.


#include "NSCompanionProgressionComponent.h"

#include "NeoSanctum/AI/Base/NSBaseDroneAI.h"
#include "NeoSanctum/Data/AI/NSCompanionCatalog.h"
#include "NeoSanctum/Data/AI/NSCompanionDefinition.h"


	

void UNSCompanionProgressionComponent::SetOwnedCompanion(ANSBaseDroneAI* Owner)
{
	if (!Owner) return;
		
	if (OwnedCompanion) return;
	OwnedCompanion = Owner;
}

void UNSCompanionProgressionComponent::ApplySelectedAndNodes(
	UNSCompanionDefinition* SelectedDefinition,
	const TMap<FGameplayTag, int32>& NodeLevels)
{
		if (!OwnedCompanion || !GetOwner() || !GetOwner()->HasAuthority())
		{
			return;
		}
		// 정의 같으면 드론 가드로 중복 적용 X
		if (SelectedDefinition)
		{
			OwnedCompanion->ApplyDroneDefinition(SelectedDefinition); 
		}
		
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


