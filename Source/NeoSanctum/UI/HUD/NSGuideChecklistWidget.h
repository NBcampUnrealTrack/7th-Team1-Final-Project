// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NeoSanctum/Data/UI/NSGuideTextData.h"
#include "NSGuideChecklistWidget.generated.h"

class UVerticalBox;
class UNSGuideChecklistItemWidget;

/**
 * 가이드 체크리스트 컨테이너 (기존 UNSGuideTextWidget 자리 대체)
 * 단계 진입 시 ShowChecklist로 항목을 전체 스폰하고,
 * 개별 완료 시 CompleteChecklistItem으로 해당 줄만 애니메이션 재생/제거한다.
 * 마지막 줄이 제거되어 비워지면 서브시스템에 알려 다음 단계로 넘어간다.
 */
UCLASS()
class NEOSANCTUM_API UNSGuideChecklistWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// 항목 전체를 새로 스폰 (기존 항목은 정리). 항목이 있으면 위젯 표시
	void ShowChecklist(const TArray<FNSGuideChecklistEntry>& Entries);

	// 지정 ItemId 줄의 완료 애니메이션 재생 (재스폰하지 않음)
	void CompleteChecklistItem(FName ItemId);

	// 지정 ItemId 줄의 완료 애니메이션을 재생하고, 그 애니메이션이 끝나는 시점에
	// 다른 미완료 항목(예: 선택적 리롤)이 남아있어도 상관없이 컨테이너 전체를 숨긴다
	void CompleteChecklistItemAndHide(FName ItemId);

	// 전체 정리 후 위젯 숨김 (모든 안내 완료 시)
	void HideChecklist();

protected:
	/**
	 * HUD는 트래블/리스폰마다 재생성(ClientRestart의 ClearHUD→CreateHUD)되므로
	 * 표시 중이던 안내를 유실하지 않도록 서브시스템에서 현재 상태를 당겨온다
	 * (기존 UNSGuideTextWidget::NativeConstruct의 RefreshGuideForHUD 훅과 동일한 역할)
	 */
	virtual void NativeConstruct() override;


	// 아이템 줄을 동적으로 담는 세로 박스 (필수 바인딩)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ItemContainer;

	// 스폰할 한 줄 위젯 클래스 (WBP_GuideChecklistItem 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Guide")
	TSubclassOf<UNSGuideChecklistItemWidget> ItemWidgetClass;

private:
	// 아이템 완료 애니메이션 종료 콜백 — 해당 줄 제거 후 비었으면 서브시스템에 통보
	UFUNCTION()
	void HandleItemCompleteFinished(UNSGuideChecklistItemWidget* Item);

	// 현재 표시 중인 아이템 줄들 (완료되면 제거됨)
	UPROPERTY(Transient)
	TArray<TObjectPtr<UNSGuideChecklistItemWidget>> ActiveItems;

	// CompleteChecklistItemAndHide로 예약된 항목 — 이 항목의 애니메이션이 끝나면 즉시 전체 숨김
	FName PendingHideAfterItemId = NAME_None;
};
