// Copyright 2026 One Team. All rights reserved.


#include "NSCommonUpgradeWidget.h"

#include "CommonButtonBase.h"
#include "NSCommonUpgradeNodeDetailWidget.h"
#include "NSCommonUpgradeNodeWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSProgressionSubsystem.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Debug/Logging/NSLogCategories.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"

void UNSCommonUpgradeWidget::OpenForInteractor(APlayerController* Interactor)
{
	if (!Interactor)
	{
		return;
	}

	OwningController = Interactor;
	AddToViewport();

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked().AddUObject(this, &ThisClass::HandleCloseButtonClicked);
	}

	BuildNodeCatalog();

	Interactor->SetShowMouseCursor(true);

	// 다른 상호작용 위젯(Part/Pet)과 달리 화면 전체를 채우는 메뉴이므로 게임 입력을 완전히 차단(UIOnly)한다.
	// TODO(원종): 현재는 테스트의 편의성 때문에 마우스를 가두지 않지만 추후에는 가둠.
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockOnCapture);
	Interactor->SetInputMode(InputMode);
}

void UNSCommonUpgradeWidget::CloseWidget()
{
	if (APlayerController* PC = OwningController.Get())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}

	RemoveFromParent();
}

FReply UNSCommonUpgradeWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		HandleCloseButtonClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

static UNSProgressionSubsystem* GetProgressionSubsystem(const UObject* WorldContext)
{
	const UGameInstance* GameInstance = WorldContext ? WorldContext->GetWorld()->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UNSProgressionSubsystem>() : nullptr;
}

void UNSCommonUpgradeWidget::HandleCloseButtonClicked()
{
	// CloseWidget()을 직접 부르지 않고 Controller를 거쳐, ActiveInteractionWidget 포인터 정리까지 Controller가 맡게 한다.
	ANSPlayerController* NSPC = Cast<ANSPlayerController>(OwningController.Get());
	if (!NSPC)
	{
		NS_LOG(LogNS, Warning, "[CommonUpgrade] Close 실패: OwningController가 유효하지 않습니다.");
		return;
	}

	NSPC->CloseInteractionWidget();
}

void UNSCommonUpgradeWidget::BuildNodeCatalog()
{
	if (IsValid(CombatListContainer))
	{
		CombatListContainer->ClearChildren();
	}
	if (IsValid(SurvivalListContainer))
	{
		SurvivalListContainer->ClearChildren();
	}
	if (IsValid(UtilityListContainer))
	{
		UtilityListContainer->ClearChildren();
	}

	RefreshCommonCurrencyDisplay();

	CurrentlyHoveredNodeId = NAME_None;
	if (IsValid(DetailWidget))
	{
		DetailWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	const UNSProgressionSubsystem* ProgressionSubsystem = GetProgressionSubsystem(this);

	if (!DataSubsystem || !ProgressionSubsystem)
	{
		if (IsValid(EmptyStateWidget))
		{
			EmptyStateWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		return;
	}

	const TMap<FName, FNSCommonUpgradeNodeRow>& AllRows = DataSubsystem->GetAllCommonUpgradeNodeRows();

	if (IsValid(EmptyStateWidget))
	{
		EmptyStateWidget->SetVisibility(
			AllRows.Num() == 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (!NodeEntryTemplate)
	{
		return;
	}

	for (const TPair<FName, FNSCommonUpgradeNodeRow>& Pair : AllRows)
	{
		UPanelWidget* Container = GetContainerForCategory(Pair.Value.Category);
		if (!IsValid(Container))
		{
			continue;
		}

		UNSCommonUpgradeNodeWidget* Entry = CreateWidget<UNSCommonUpgradeNodeWidget>(this, NodeEntryTemplate);
		if (!Entry)
		{
			continue;
		}

		const int32 CurrentLevel = ProgressionSubsystem->GetCommonSkillLevel(Pair.Key);

		Entry->SetupEntry(Pair.Key, Pair.Value, CurrentLevel);
		Entry->OnNodeHovered.AddUniqueDynamic(this, &ThisClass::HandleNodeHovered);
		Entry->OnNodeUnhovered.AddUniqueDynamic(this, &ThisClass::HandleNodeUnhovered);

		Container->AddChild(Entry);
	}
}

void UNSCommonUpgradeWidget::RefreshCommonCurrencyDisplay()
{
	if (!IsValid(CommonCurrencyText))
	{
		return;
	}

	const UNSProgressionSubsystem* ProgressionSubsystem = GetProgressionSubsystem(this);
	CommonCurrencyText->SetText(FText::AsNumber(ProgressionSubsystem ? ProgressionSubsystem->GetCommonCurrency() : 0));
}

UPanelWidget* UNSCommonUpgradeWidget::GetContainerForCategory(ENSCommonUpgradeCategory Category) const
{
	switch (Category)
	{
	case ENSCommonUpgradeCategory::Combat:
		return CombatListContainer;

	case ENSCommonUpgradeCategory::Survival:
		return SurvivalListContainer;

	case ENSCommonUpgradeCategory::Utility:
		return UtilityListContainer;

	default:
		return nullptr;
	}
}

void UNSCommonUpgradeWidget::MoveDetailWidgetToCategoryPosition(ENSCommonUpgradeCategory Category)
{
	UCanvasPanelSlot* CanvasSlot = IsValid(DetailWidget) ? Cast<UCanvasPanelSlot>(DetailWidget->Slot) : nullptr;
	if (!CanvasSlot)
	{
		return;
	}

	switch (Category)
	{
	case ENSCommonUpgradeCategory::Combat:
		CanvasSlot->SetPosition(DetailPositionForCombat);
		break;

	case ENSCommonUpgradeCategory::Survival:
		CanvasSlot->SetPosition(DetailPositionForSurvival);
		break;

	case ENSCommonUpgradeCategory::Utility:
		CanvasSlot->SetPosition(DetailPositionForUtility);
		break;

	default:
		break;
	}
}

void UNSCommonUpgradeWidget::HandleNodeHovered(FName NodeId)
{
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	const UNSProgressionSubsystem* ProgressionSubsystem = GetProgressionSubsystem(this);
	if (!DataSubsystem || !ProgressionSubsystem)
	{
		return;
	}

	const FNSCommonUpgradeNodeRow* Row = DataSubsystem->GetCommonUpgradeNodeRow(NodeId);
	if (!Row)
	{
		return;
	}

	CurrentlyHoveredNodeId = NodeId;

	const int32 CurrentLevel = ProgressionSubsystem->GetCommonSkillLevel(NodeId);
	const int32 NewLevel = CurrentLevel + 1;
	const int64 NextCost = ProgressionSubsystem->GetCommonUpgradeCost(NodeId, NewLevel);

	if (IsValid(DetailWidget))
	{
		DetailWidget->SetupDetail(*Row, CurrentLevel, NewLevel, NextCost);
		DetailWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	MoveDetailWidgetToCategoryPosition(Row->Category);
}

void UNSCommonUpgradeWidget::HandleNodeUnhovered(FName NodeId)
{
	// 다른 노드로 빠르게 이동했을 때, 방금 호버된 노드의 언호버 이벤트가 늦게 도착해
	// 새로 채운 패널을 잘못 닫아버리는 것을 방지.
	if (CurrentlyHoveredNodeId != NodeId)
	{
		return;
	}

	CurrentlyHoveredNodeId = NAME_None;

	if (IsValid(DetailWidget))
	{
		DetailWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}
