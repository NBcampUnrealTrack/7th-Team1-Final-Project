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
 * 표시 (아이콘/레벨)와 호버,클릭 브리지만 담당하며, NewLevel/Cost/재화는 계산, 전달, 보관하지 않음.
 */
UCLASS()
class NEOSANCTUM_API UNSCommonUpgradeNodeWidget : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	void SetupEntry(FName InNodeId, const FNSCommonUpgradeNodeRow& Row, int32 CurrentLevel);

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
	TObjectPtr<UTextBlock> LevelText;

private:
	void HandleHovered();
	void HandleUnhovered();
	void HandleClicked();

	FName BoundNodeId;
	TSharedPtr<FStreamableHandle> IconLoadHandle;
};
