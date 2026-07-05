// Copyright 2026 One Team. All rights reserved.


#include "NSCommonUpgradeNodeWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/AssetManager.h"

void UNSCommonUpgradeNodeWidget::SetupEntry(FName InNodeId, const FNSCommonUpgradeNodeRow& Row, int32 CurrentLevel)
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
			NSLOCTEXT("CommonUpgrade", "NodeLevelFormat", "{0}/{1}"),
			FText::AsNumber(CurrentLevel),
			FText::AsNumber(Row.MaxLevel))
		);
	}
}

void UNSCommonUpgradeNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	OnHovered().AddUObject(this, &ThisClass::HandleHovered);
	OnUnhovered().AddUObject(this, &ThisClass::HandleUnhovered);
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
	if (!BoundNodeId.IsNone())
	{
		OnNodeHovered.Broadcast(BoundNodeId);
	}
}

void UNSCommonUpgradeNodeWidget::HandleUnhovered()
{
	if (!BoundNodeId.IsNone())
	{
		OnNodeUnhovered.Broadcast(BoundNodeId);
	}
}

void UNSCommonUpgradeNodeWidget::HandleClicked()
{
	if (!BoundNodeId.IsNone())
	{
		OnUpgradeRequested.Broadcast(BoundNodeId);
	}
}
