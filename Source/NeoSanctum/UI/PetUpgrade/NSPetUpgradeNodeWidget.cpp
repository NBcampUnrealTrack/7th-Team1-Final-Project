#include "NSPetUpgradeNodeWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/AssetManager.h"
#include "NeoSanctum/Tag/NSGameplayTags_Companion.h"

bool UNSPetUpgradeNodeWidget::ApplyNodeData(const FNSPetUpgradeNodeViewData& NodeData)
{
	if (!BoundNodeTag.IsValid() || BoundNodeTag != NodeData.NodeTag)
	{
		return false;
	}
	CurrentNodeData = NodeData;

	ApplyIcon(NodeData.Icon);

	if (IsValid(LevelText))
	{
		// 드론(0/1)·스탯(0/5) 모두 동일 포맷
		LevelText->SetText(FText::Format(
			NSLOCTEXT("PetUpgrade", "NodeLevelFormat", "{0} / {1}"),
			FText::AsNumber(NodeData.CurrentLevel),
			FText::AsNumber(NodeData.MaxLevel)));
	}

	if (IsValid(CostText))
	{
		if (NodeData.CurrentLevel >= NodeData.MaxLevel)
		{
			CostText->SetText(NSLOCTEXT("CompanionUpgrade", "Max", "MAX"));
		}
		else
		{
			CostText->SetText(FText::AsNumber(NodeData.UpgradeCost));
		}
	}
	
	// 잠금 노드는 호버/클릭 등 상호작용 자체를 막는다
	SetIsInteractionEnabled(NodeData.StateTag != NSGameplayTags::UI_PetUpgrade_State_Locked);
	
	// 호버로 상세를 봐야 하므로 버튼 자체는 비활성화하지 않고, 클릭만 상태로 게이트한다.
	OnNodeStateUpdated(NodeData);
	return true;
}

void UNSPetUpgradeNodeWidget::ApplyIcon(const TSoftObjectPtr<UTexture2D>& Icon)
{
	if (!IsValid(NodeIcon))
	{
		return;
	}

	if (UTexture2D* Texture = Icon.Get())
	{
		NodeIcon->SetBrushFromTexture(Texture);
		NodeIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else if (!Icon.IsNull())
	{
		TWeakObjectPtr<UNSPetUpgradeNodeWidget> WeakThis(this);
		TSoftObjectPtr<UTexture2D> SoftIcon = Icon;
		IconLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
			SoftIcon.ToSoftObjectPath(),
			[WeakThis, SoftIcon]()
			{
				if (!WeakThis.IsValid() || !IsValid(WeakThis->NodeIcon)) { return; }
				WeakThis->NodeIcon->SetBrushFromTexture(SoftIcon.Get());
				WeakThis->NodeIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
				WeakThis->IconLoadHandle.Reset();
			});
	}
	else
	{
		NodeIcon->SetBrushFromTexture(nullptr);
		NodeIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSPetUpgradeNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 처음엔 hover/pressed 프레임 숨김
	if (IsValid(NodeHoveredFrameImage)) { NodeHoveredFrameImage->SetVisibility(ESlateVisibility::Collapsed); }
	if (IsValid(NodePressedFrameImage)) { NodePressedFrameImage->SetVisibility(ESlateVisibility::Collapsed); }

	OnClicked().AddUObject(this, &ThisClass::HandleClickedInternal);
	OnHovered().AddUObject(this, &ThisClass::HandleHoveredInternal);
	OnUnhovered().AddUObject(this, &ThisClass::HandleUnhoveredInternal);
	OnPressed().AddUObject(this, &ThisClass::HandlePressedInternal);
	OnReleased().AddUObject(this, &ThisClass::HandleReleasedInternal);
}

void UNSPetUpgradeNodeWidget::NativeDestruct()
{
	if (IconLoadHandle.IsValid())
	{
		IconLoadHandle->CancelHandle();
		IconLoadHandle.Reset();
	}
	Super::NativeDestruct();
}

void UNSPetUpgradeNodeWidget::HandleClickedInternal()
{
	if (!CurrentNodeData.CompanionTag.IsValid())
	{
		return;
	}

	if (CurrentNodeData.bIsDroneSelectNode)
	{
		if (CurrentNodeData.StateTag != NSGameplayTags::UI_PetUpgrade_State_Selectable) { return; }
		OnSelectRequested.Broadcast(CurrentNodeData.CompanionTag);
		return;
	}

	if (!CurrentNodeData.NodeTag.IsValid()) { return; }
	if (CurrentNodeData.StateTag != NSGameplayTags::UI_PetUpgrade_State_Upgradable) { return; }
	OnUpgradeRequested.Broadcast(CurrentNodeData.CompanionTag, CurrentNodeData.NodeTag);
}

void UNSPetUpgradeNodeWidget::HandleHoveredInternal()
{
	if (IsValid(NodeHoveredFrameImage))
	{
		NodeHoveredFrameImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	OnNodeHovered.Broadcast(CurrentNodeData, this);
}

void UNSPetUpgradeNodeWidget::HandleUnhoveredInternal()
{
	if (IsValid(NodeHoveredFrameImage))
	{
		NodeHoveredFrameImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	OnNodeUnhovered.Broadcast(CurrentNodeData.NodeTag);
}

void UNSPetUpgradeNodeWidget::HandlePressedInternal()
{
	if (IsValid(NodePressedFrameImage))
	{
		NodePressedFrameImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UNSPetUpgradeNodeWidget::HandleReleasedInternal()
{
	if (IsValid(NodePressedFrameImage))
	{
		NodePressedFrameImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}
