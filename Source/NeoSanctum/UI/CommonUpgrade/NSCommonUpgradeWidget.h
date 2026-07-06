// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Data/CommonUpgrade/NSCommonUpgradeTypes.h"
#include "NeoSanctum/UI/Interaction/NSNPCInteractionWidgetBase.h"
#include "NSCommonUpgradeWidget.generated.h"

class UNSCommonUpgradeNodeDetailWidget;
class UPanelWidget;
class UTextBlock;
class UWidget;
class UNSCommonUpgradeNodeWidget;
class UCommonButtonBase;

/**
 * 공용 업그레이드 콘솔 UI.
 * 현재는 오픈/클로즈, 입력모드 전환, 카테고리별 노드 카탈로그 표시, 호버 디테일 패널을 담당.
 * 구매 처리는 추후 추가.
 */
UCLASS()
class NEOSANCTUM_API UNSCommonUpgradeWidget : public UNSNPCInteractionWidgetBase
{
	GENERATED_BODY()

public:
	virtual void OpenForInteractor(APlayerController* Interactor) override;
	virtual void CloseWidget() override;

protected:
	// FInputModeUIOnly가 게임 입력을 완전히 차단해 ANSPlayerController의 네이티브 ESC 바인딩이
	// 도달하지 못하므로, 포커스를 가진 이 위젯이 직접 ESC를 가로채 닫기 처리.
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 구매 실패 사유를 Blueprint 쪽 UI(토스트/라벨 등)에 표시하도록 위임
	UFUNCTION(BlueprintImplementableEvent, Category = "CommonUpgrade")
	void OnPurchaseFailed(const FText& Reason);

	// 닫기 버튼 (WBP에서 이름 일치 필요)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> CloseButton;

	// 전투/생존/유틸 카테고리별 노드 컨테이너 (WBP에서 이름 일치 필요)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> CombatListContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> SurvivalListContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> UtilityListContainer;

	// 표시 가능한 노드 Row가 없을 때 보여줄 빈 상태 위젯 (WBP에서 이름 일치 필요)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> EmptyStateWidget;

	// 호버된 노드의 상세 정보를 보여주는 공용 패널. Canvas Panel의 자식이어야 위치 이동이 동작.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSCommonUpgradeNodeDetailWidget> DetailWidget;

	// 공통 재화 표시 (WBP에서 이름 일치 필요)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CommonCurrencyText;

	// 에디터 Class Defaults에서 WBP_CommonUpgradeNode 지정
	UPROPERTY(EditDefaultsOnly, Category = "CommonUpgrade")
	TSubclassOf<UNSCommonUpgradeNodeWidget> NodeEntryTemplate;

	// 카테고리별 디테일 패널 표시 좌표 (Canvas Panel 기준, 에디터에서 눈으로 보며 조정)
	UPROPERTY(EditDefaultsOnly, Category = "CommonUpgrade|Layout")
	FVector2D DetailPositionForCombat = FVector2D(760.0f, 260.0f);

	UPROPERTY(EditDefaultsOnly, Category = "CommonUpgrade|Layout")
	FVector2D DetailPositionForSurvival = FVector2D(1550.0f, 260.0f);

	UPROPERTY(EditDefaultsOnly, Category = "CommonUpgrade|Layout")
	FVector2D DetailPositionForUtility = FVector2D(340.0f, 260.0f);

private:
	void BuildNodeCatalog();
	void RefreshCommonCurrencyDisplay();
	UPanelWidget* GetContainerForCategory(ENSCommonUpgradeCategory Category) const;
	void MoveDetailWidgetToCategoryPosition(ENSCommonUpgradeCategory Category);
	void TryPurchase(FName NodeId);

	UFUNCTION()
	void HandleCloseButtonClicked();

	UFUNCTION()
	void HandleNodeHovered(FName NodeId);

	UFUNCTION()
	void HandleNodeUnhovered(FName NodeId);

	UFUNCTION()
	void HandleNodeUpgradeRequested(FName NodeId);

	UPROPERTY()
	TWeakObjectPtr<APlayerController> OwningController;

	// 현재 디테일 패널이 표시 중인 노드. 호버 종료 이벤트가 늦게 도착해 다른 노드의 패널을 잘못 닫는 것을 방지.
	FName CurrentlyHoveredNodeId;
};
