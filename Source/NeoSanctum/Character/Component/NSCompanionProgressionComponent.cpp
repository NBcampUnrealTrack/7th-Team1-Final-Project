// Copyright 2026 One Team. All rights reserved.


#include "NSCompanionProgressionComponent.h"

#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "NeoSanctum/Data/AI/NSCompanionCatalog.h"
#include "NeoSanctum/Data/AI/NSCompanionDefinition.h"


void UNSCompanionProgressionComponent::ServerTryUpgrade_Implementation(FGameplayTag NodeTag)
{
	if (!OwnedCompanion || !Catalog || !SelectedCompanionTag.IsValid()) return;
	
	// 클릭시 받은 태그를 기반으로 드론의 정의를 캐싱
	UNSCompanionDefinition* CurrentDefinition = Catalog->FindByTag(SelectedCompanionTag);
	if (!CurrentDefinition) return;
	
	// 캐싱된 정의가 보유한 업그레이드 노드들 순회
	for (const FNSCompanionUpgradeNode& UpgradeNode : CurrentDefinition->UpgradeNodes)
	{
		// 현재 요청된 태그와 다를경우 건너뛰기
		if (UpgradeNode.NodeTag != NodeTag) continue;
		
		// 같다면 해당 노드의 최대 레벨과 Player가 가지고있는 해당 노드의 현재 레벨 리딩
		const int32 MaxNodeLevel = UpgradeNode.MaxLevel;
		const int32 CurrentNodeLevel = NodeLevels.FindRef(NodeTag);
		
		// 새로 적용할 레벨 계산
		const int32 NewLevel = CurrentNodeLevel + 1;
		
		// 업그레이드 적용 후 레벨이 Max레벨보다 높다면 스킵
		if (NewLevel > MaxNodeLevel) break;
		
		// 노드 레벨 저장
		NodeLevels.Add(NodeTag, NewLevel);
		// 선택된 태그의 누적 업그레이드를 찾아서 새로운 값으로 적용
		CompanionUpgradeCounts.FindOrAdd(SelectedCompanionTag)++;
		// 소유중인 Companion에 능력치 실제 적용
		OwnedCompanion->ApplyStatUpgrade(UpgradeNode.NodeTag, NewLevel);
		break;
	}
}


