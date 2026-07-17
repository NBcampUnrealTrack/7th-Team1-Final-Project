// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "NeoSanctum/Data/CommonUpgrade/NSCommonUpgradeTypes.h"
#include "NSCommonUpgradeNodeWidget.generated.h"

struct FStreamableHandle;
class UImage;
class UTextBlock;

// 노드가 상위 UNSCommonUpgradeWidget에 상태를 알릴 때 쓰는 이벤트. NodeId만 전달.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNSCommonUpgradeNodeSignature, FName, NodeId);

/**
 * 공용 업그레이드 노드 하나를 표시하는 그리드 카드.
 * 비용 계산은 상위 위젯이 담당하고, 이 위젯은 전달받은 다음 비용을 표시만 함.
 */
UCLASS()
class NEOSANCTUM_API UNSCommonUpgradeNodeWidget : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	void SetupEntry(
		FName InNodeId,
		const FNSCommonUpgradeNodeRow& Row,
		int32 CurrentLevel,
		int64 NextCost
	);

	FNSCommonUpgradeNodeSignature OnNodeHovered;
	FNSCommonUpgradeNodeSignature OnNodeUnhovered;

	// 클릭 브리지.
	FNSCommonUpgradeNodeSignature OnUpgradeRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> NodeHoveredFrameImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> NodePressedFrameImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> CostCurrencyIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText;

private:
	void HandleHovered();
	void HandleUnhovered();
	void HandlePressed();
	void HandleReleased();
	void HandleClicked();

	FName BoundNodeId;
	TSharedPtr<FStreamableHandle> IconLoadHandle;
};
