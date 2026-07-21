// Copyright 2026 One Team. All rights reserved.

#include "NSGuideChecklistWidget.h"
#include "Components/VerticalBox.h"
#include "NeoSanctum/UI/HUD/NSGuideChecklistItemWidget.h"
#include "NeoSanctum/Core/Waypoint/NSGuideSubsystemBase.h"
#include "NeoSanctum/Core/Waypoint/NSOutRunGuideSubsystem.h"
#include "NeoSanctum/Core/Waypoint/NSInRunGuideSubsystem.h"

namespace
{
	/** 이 월드에서 실제로 시작된(bGuideStarted) 가이드 서브시스템을 반환 (아웃런/인런 중 하나) */
	UNSGuideSubsystemBase* GetActiveGuide(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		if (UNSOutRunGuideSubsystem* OutRun = World->GetSubsystem<UNSOutRunGuideSubsystem>())
		{
			if (OutRun->IsGuideStarted())
			{
				return OutRun;
			}
		}
		if (UNSInRunGuideSubsystem* InRun = World->GetSubsystem<UNSInRunGuideSubsystem>())
		{
			if (InRun->IsGuideStarted())
			{
				return InRun;
			}
		}
		return nullptr;
	}
}

void UNSGuideChecklistWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// HUD 재생성 시 현재 월드의 활성 가이드(아웃런/인런) 상태를 새 위젯에 재적용
	if (UNSGuideSubsystemBase* GuideSubsystem = GetActiveGuide(GetWorld()))
	{
		GuideSubsystem->RefreshGuideForHUD();
	}
}

void UNSGuideChecklistWidget::ShowChecklist(const TArray<FNSGuideChecklistEntry>& Entries)
{
	// 컨테이너/아이템 클래스가 세팅 안 됐으면 표시 불가
	if (!ItemContainer || !ItemWidgetClass)
	{
		return;
	}

	/**
	 * 기존 항목을 직접 정리한다 (RemoveFromParent 경로가 아니므로
	 * HandleItemCompleteFinished를 거치지 않아 비었음 통보가 잘못 발생하지 않음)
	 */
	ItemContainer->ClearChildren();
	ActiveItems.Reset();

	for (const FNSGuideChecklistEntry& Entry : Entries)
	{
		UNSGuideChecklistItemWidget* Item =
			CreateWidget<UNSGuideChecklistItemWidget>(this, ItemWidgetClass);
		if (!Item)
		{
			continue;
		}

		Item->Setup(Entry.ItemId, Entry.Text);
		Item->OnItemCompleteFinished.AddDynamic(this, &UNSGuideChecklistWidget::HandleItemCompleteFinished);
		ItemContainer->AddChildToVerticalBox(Item);
		ActiveItems.Add(Item);
	}

	// 항목이 하나라도 있으면 표시, 없으면 접기
	SetVisibility(ActiveItems.Num() > 0
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed);
}

void UNSGuideChecklistWidget::CompleteChecklistItem(FName ItemId)
{
	// 해당 ItemId 줄을 찾아 완료 애니메이션만 재생 (제거는 애니메이션 종료 콜백에서)
	for (UNSGuideChecklistItemWidget* Item : ActiveItems)
	{
		if (Item && Item->GetItemId() == ItemId)
		{
			Item->PlayCompleteAnimation();
			return;
		}
	}
}

void UNSGuideChecklistWidget::CompleteChecklistItemAndHide(FName ItemId)
{
	for (UNSGuideChecklistItemWidget* Item : ActiveItems)
	{
		if (Item && Item->GetItemId() == ItemId)
		{
			// 이 항목의 완료 애니메이션이 끝나는 시점(HandleItemCompleteFinished)에 전체 숨김을 실행한다
			PendingHideAfterItemId = ItemId;
			Item->PlayCompleteAnimation();
			return;
		}
	}

	// 해당 항목을 못 찾으면(WBP 미배치 등) 기다릴 애니메이션이 없으므로 즉시 숨김
	HideChecklist();
}

void UNSGuideChecklistWidget::HandleItemCompleteFinished(UNSGuideChecklistItemWidget* Item)
{
	if (!Item)
	{
		return;
	}

	ActiveItems.Remove(Item);

	/**
	 * RemoveFromParent 대신 Hidden으로만 처리한다
	 * → VerticalBox에서 자식을 완전히 빼면 남은 줄들이 빈 자리를 메우려 위로 당겨지는데,
	 *   Hidden은 렌더링만 끄고 레이아웃 공간은 유지하므로 나머지 줄이 제자리에 고정된다.
	 *   (다음 단계로 넘어갈 때 ShowChecklist의 ClearChildren이 숨겨둔 줄까지 통째로 정리)
	 */
	Item->SetVisibility(ESlateVisibility::Hidden);

	/**
	 * 명시적 종료 예약 대상 항목이면, 다른 미완료 항목이 남아있어도 상관없이
	 * 이 애니메이션이 끝난 지금 컨테이너 전체를 숨긴다 (CompleteChecklistItemAndHide 참고)
	 */
	if (!PendingHideAfterItemId.IsNone() && Item->GetItemId() == PendingHideAfterItemId)
	{
		PendingHideAfterItemId = NAME_None;
		HideChecklist();
		return;
	}

	/**
	 * 마지막 줄까지 완료되어 ActiveItems가 비면 서브시스템에 통보한다
	 * → 서브시스템이 다음 단계를 ShowChecklist 하거나 전부 완료면 HideChecklist 한다.
	 * 이 empty 기반 전환 덕분에 단계 경계에서 마지막 줄의 사라짐 애니메이션이 잘리지 않는다.
	 */
	if (ActiveItems.Num() == 0)
	{
		// 현재 월드의 활성 가이드에 통보 → 다음 단계 표시 또는 전체 숨김
		if (UNSGuideSubsystemBase* GuideSubsystem = GetActiveGuide(GetWorld()))
		{
			GuideSubsystem->NotifyChecklistEmptied();
		}
	}
}

void UNSGuideChecklistWidget::HideChecklist()
{
	ActiveItems.Reset();
	if (ItemContainer)
	{
		ItemContainer->ClearChildren();
	}
	SetVisibility(ESlateVisibility::Collapsed);
}
