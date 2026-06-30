// Copyright 2026 One Team. All rights reserved.


#include "NSPetUpgradeNodeWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

bool UNSPetUpgradeNodeWidget::ApplyNodeData(const FNSPetUpgradeNodeViewData& NodeData)
{
	// 다른 노드의 데이터가 잘못 적용되지 않도록 태그 일치 여부 확인
	if (!BoundNodeTag.IsValid() ||
	BoundNodeTag != NodeData.NodeTag)
	{
		return false;
	}
	// 클릭 요청과 화면 갱신에 사용할 최신 데이터 보관
	CurrentNodeData = NodeData;
	
	LevelText->SetText(
	FText::Format(
		NSLOCTEXT(
			"PetUpgrade",
			"NodeLevelFormat",
			"{0}/{1}"),
		FText::AsNumber(NodeData.CurrentLevel),
		FText::AsNumber(NodeData.MaxLevel)));

	UpgradeButton->SetIsEnabled(
		NodeData.CurrentLevel < NodeData.MaxLevel);

	return true;
}

void UNSPetUpgradeNodeWidget::RequestUpgrade()
{
	// Snapshot이 적용되지 않은 노드는 요청하지 않음
	if (!CurrentNodeData.CompanionTag.IsValid() ||
		!CurrentNodeData.NodeTag.IsValid())
	{
		return;
	}

	// 최대 레벨에서는 요청을 방송하지 않음
	if (CurrentNodeData.CurrentLevel >=
		CurrentNodeData.MaxLevel)
	{
		return;
	}

	OnUpgradeRequested.Broadcast(
		CurrentNodeData.CompanionTag,
		CurrentNodeData.NodeTag);
}
void UNSPetUpgradeNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	UpgradeButton->OnClicked.RemoveDynamic(
	this,
	&ThisClass::RequestUpgrade);

	UpgradeButton->OnClicked.AddUniqueDynamic(
		this,
		&ThisClass::RequestUpgrade);
}

void UNSPetUpgradeNodeWidget::NativeDestruct()
{
	UpgradeButton->OnClicked.RemoveDynamic(
	this,
	&ThisClass::RequestUpgrade);
	
	Super::NativeDestruct();
}
