// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSCompanionUpgradeDetailWidget.generated.h"

class UTextBlock;
class UPanelWidget;
struct FNSPetUpgradeNodeViewData;

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSCompanionUpgradeDetailWidget : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	void ApplyDetail(const FNSPetUpgradeNodeViewData& NodeData);

protected:
	UPROPERTY(meta=(BindWidgetOptional)) 
	TObjectPtr<UTextBlock> TitleText;        // "드론 공격력 증가"
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;  // "공격 드론의 공격력이 증가합니다"
	
	UPROPERTY(meta=(BindWidgetOptional)) 
	TObjectPtr<UTextBlock> IncreaseText;     // "+ X"
	
	UPROPERTY(meta=(BindWidgetOptional)) 
	TObjectPtr<UTextBlock> CostText;         // "업그레이드 비용 : N"
	
	UPROPERTY(meta=(BindWidgetOptional)) 
	TObjectPtr<UPanelWidget> DroneStatsBox;  // 드론 노드용 스탯 목록(선택)
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UWidget> IncreaseSection;   // "업그레이드 시 증가량" 라벨+값 묶는 패널
};
