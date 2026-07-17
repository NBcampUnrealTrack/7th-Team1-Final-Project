// Copyright 2026 One Team. All rights reserved.


#include "NSCommonUpgradeNodeWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/AssetManager.h"

void UNSCommonUpgradeNodeWidget::SetupEntry(
	FName InNodeId,
	const FNSCommonUpgradeNodeRow& Row,
	int32 CurrentLevel,
	int64 NextCost)
{
	BoundNodeId = InNodeId;

	if (IsValid(IconImage))
	{
		if (UTexture2D* Texture = Row.Icon.Get())
		{
			IconImage->SetBrushFromTexture(Texture);
		}
		else if (!Row.Icon.IsNull())
		{
			// DT_CommonUpgradeNode의 아이콘은 UNSDataSubsystem의 선로드 대상에 포함되지 않아
			// 위젯이 직접 비동기 로드해 동기 로드 히치를 피함.
			TWeakObjectPtr<UNSCommonUpgradeNodeWidget> WeakThis(this);
			TSoftObjectPtr<UTexture2D> SoftIcon = Row.Icon;
			IconLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
				SoftIcon.ToSoftObjectPath(),
				[WeakThis, SoftIcon]()
				{
					if (!WeakThis.IsValid() || !IsValid(WeakThis->IconImage))
					{
						return;
					}
					WeakThis->IconImage->SetBrushFromTexture(SoftIcon.Get());
					WeakThis->IconLoadHandle.Reset();
				});
		}
		else
		{
			IconImage->SetBrushFromTexture(nullptr);
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (IsValid(LevelText))
	{
		LevelText->SetText(FText::Format(
			NSLOCTEXT("CommonUpgrade", "NodeLevelFormat", "{0} / {1}"),
			FText::AsNumber(CurrentLevel),
			FText::AsNumber(Row.MaxLevel))
		);
	}

	const bool bIsMaxLevel = CurrentLevel >= Row.MaxLevel;

	if (IsValid(CostText))
	{
		CostText->SetText(bIsMaxLevel ? NSLOCTEXT("CommonUpgrade", "NodeMaxLevel", "MAX") : FText::AsNumber(NextCost));
	}

	if (IsValid(CostCurrencyIcon))
	{
		// 최대 레벨일 때는 MAX 글자만 가운데 남겨둠.
		CostCurrencyIcon->SetVisibility(
			bIsMaxLevel ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
}

void UNSCommonUpgradeNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(NodeHoveredFrameImage))
	{
		// 처음 열렸을 때는 일반 프레임만 보이게 해둠.
		NodeHoveredFrameImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (IsValid(NodePressedFrameImage))
	{
		// Pressed 프레임은 버튼을 누를 때만 보여줌.
		NodePressedFrameImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	OnHovered().AddUObject(this, &ThisClass::HandleHovered);
	OnUnhovered().AddUObject(this, &ThisClass::HandleUnhovered);
	OnPressed().AddUObject(this, &ThisClass::HandlePressed);
	OnReleased().AddUObject(this, &ThisClass::HandleReleased);
	OnClicked().AddUObject(this, &ThisClass::HandleClicked);
}

void UNSCommonUpgradeNodeWidget::NativeDestruct()
{
	if (IconLoadHandle.IsValid())
	{
		IconLoadHandle->CancelHandle();
		IconLoadHandle.Reset();
	}

	Super::NativeDestruct();
}

void UNSCommonUpgradeNodeWidget::HandleHovered()
{
	if (IsValid(NodeHoveredFrameImage))
	{
		NodeHoveredFrameImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	if (!BoundNodeId.IsNone())
	{
		OnNodeHovered.Broadcast(BoundNodeId, this);
	}
}

void UNSCommonUpgradeNodeWidget::HandleUnhovered()
{
	if (IsValid(NodeHoveredFrameImage))
	{
		NodeHoveredFrameImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!BoundNodeId.IsNone())
	{
		OnNodeUnhovered.Broadcast(BoundNodeId);
	}
}

void UNSCommonUpgradeNodeWidget::HandlePressed()
{
	if (IsValid(NodePressedFrameImage))
	{
		NodePressedFrameImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UNSCommonUpgradeNodeWidget::HandleReleased()
{
	if (IsValid(NodePressedFrameImage))
	{
		NodePressedFrameImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSCommonUpgradeNodeWidget::HandleClicked()
{
	if (!BoundNodeId.IsNone())
	{
		OnUpgradeRequested.Broadcast(BoundNodeId);
	}
}
