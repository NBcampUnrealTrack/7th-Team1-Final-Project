// Copyright 2026 One Team. All rights reserved.


#include "NSCompanionUpgradeDetailWidget.h"

#include "Components/TextBlock.h"
#include "NeoSanctum/Type/NSPetUpgradeMessageTypes.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "NeoSanctum/Type/NSPetUpgradeMessageTypes.h"

void UNSCompanionUpgradeDetailWidget::ApplyDetail(const FNSPetUpgradeNodeViewData& NodeData)
{
	const bool bDrone = NodeData.bIsDroneSelectNode;

	if (IsValid(TitleText))       { TitleText->SetText(NodeData.DisplayName); }
	if (IsValid(DescriptionText)) { DescriptionText->SetText(NodeData.Description); }

	// 증가량 섹션: 스탯 노드에서만 표시
	if (IsValid(IncreaseText))
	{
		IncreaseText->SetText(FText::Format(
			NSLOCTEXT("Pet", "Inc", "+ {0}"), FText::AsNumber(NodeData.IncreasePerLevel)));
	}
	if (IsValid(IncreaseSection))
	{
		IncreaseSection->SetVisibility(
			bDrone ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}

	// 비용: 둘 다 표시 (스탯=강화비용, 드론=해금비용, 만렙/해금완료=MAX)
	if (IsValid(CostText))
	{
		CostText->SetText(
			(NodeData.CurrentLevel >= NodeData.MaxLevel)
			? NSLOCTEXT("Pet", "Max", "MAX")
			: FText::Format(NSLOCTEXT("Pet", "Cost", "업그레이드 비용 : {0}"), FText::AsNumber(NodeData.UpgradeCost)));
	}

	// 드론 스탯 목록: 드론 노드에서만
	if (IsValid(DroneStatsBox))
	{
		DroneStatsBox->ClearChildren();

		if (bDrone && NodeData.DroneStats.Num() > 0)
		{
			DroneStatsBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			for (const FNSCompanionStatEntry& Stat : NodeData.DroneStats)
			{
				if (UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>())
				{
					Row->SetText(FText::Format(
						NSLOCTEXT("Pet", "StatRow", "{0} : {1}"),
						Stat.Name, FText::AsNumber(Stat.Value)));
					DroneStatsBox->AddChild(Row);
				}
			}
		}
		else
		{
			DroneStatsBox->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
