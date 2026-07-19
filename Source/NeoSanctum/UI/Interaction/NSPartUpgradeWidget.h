// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/UI/Interaction/NSNPCInteractionWidgetBase.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NSPartUpgradeWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class UWidgetSwitcher;
class UNSPartCatalogEntryWidget;
class UNSPartDefinition;
class UNSPartDetailWidget;
class UNSPartSlotButton;
class UNSPartEquipComponent;
class UNSCurrencyComponent;
class ANSPartPreviewStage;
class UNSCharacterStatsWidget;

/**
 * 인런 파츠 NPC UI 루트. WidgetSwitcher 3페이지:
 *   0 허브(장착 요약 + 진입 버튼) / 1 구매(개인 재고 9칸) / 2 업그레이드·리롤
 */
UCLASS()
class NEOSANCTUM_API UNSPartUpgradeWidget : public UNSNPCInteractionWidgetBase
{
	GENERATED_BODY()

public:
	virtual void OpenForInteractor(APlayerController* Interactor) override;
	virtual void CloseWidget() override;

protected:
	// FInputModeUIOnly가 게임 입력을 완전히 차단해 ANSPlayerController의 네이티브 ESC 바인딩이
	// 도달하지 못하므로, 포커스를 가진 이 위젯이 직접 ESC를 가로채 닫기 처리.
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Part")
	void OpenHubPage();

	UFUNCTION(BlueprintCallable, Category = "Part")
	void OpenPurchasePage();

	UFUNCTION(BlueprintCallable, Category = "Part")
	void OpenUpgradePage();

protected:
	// 성공 이펙트 / 실패 셰이크 / 재화부족·품절 토스트 — WBP에 위임
	UFUNCTION(BlueprintImplementableEvent, Category = "Part")
	void OnUpgradeResultReceived(FGameplayTag PartSlot, ENSPartUpgradeResult Result);

	// ---- 공통 ----
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> PageSwitcher;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TempBalanceText;

	// 업그레이드/리롤 페이지의 임시 재화 표시 (TempBalanceText와 동일 값으로 함께 갱신)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UpgradeBalanceText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	// ---- Page 0: 허브 ----
	// 허브 페이지 우상단 종료(X) 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> HubCloseButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> HubPurchaseButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> HubUpgradeButton;

	// 장착중 슬롯 아이콘 3개 (표시 전용)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartSlotButton> HubBodySlotButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartSlotButton> HubArmSlotButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartSlotButton> HubLegSlotButton;

	// 장착 결과 최종 캐릭터 스텟 표시 (WBP_CharacterStatsWidget 재사용)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSCharacterStatsWidget> HubCharacterStatsWidget;

	// ---- Page 1: 구매 ----
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> PurchaseBackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> BodyStockContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ArmStockContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> LegStockContainer;

	// 구매 페이지의 "현재 장착중" 슬롯 표시 3개 (표시 전용 — 아웃런 레이아웃과 동일 이름)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartSlotButton> BodyEquippedButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartSlotButton> ArmEquippedButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartSlotButton> LegEquippedButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartDetailWidget> StockDetailWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BuyPriceText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BuyButton;

	// 재고 1칸 위젯 (에디터 Class Defaults에서 WBP_PartCatalogEntry 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Part")
	TSubclassOf<UNSPartCatalogEntryWidget> StockEntryTemplate;

	// 결과 토스트("업그레이드 성공", "재화 부족" 등) 위젯 (에디터 Class Defaults에서 WBP_NoticePopup 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Part")
	TSubclassOf<class UNSNoticePopupWidget> NoticePopupClass;

	// 에디터 Class Defaults에서 BP_PartPreviewStage 지정 (구매/업그레이드 3D 프리뷰용)
	UPROPERTY(EditDefaultsOnly, Category = "Part")
	TSubclassOf<ANSPartPreviewStage> PreviewStageClass;

	// ---- Page 2: 업그레이드/리롤 ----
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> UpgradeBackButton;

	// 업그레이드 페이지 전용 종료 버튼 (구매 페이지의 CloseButton과 별개)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> UpgradeCloseButton;

	// 상단 슬롯 선택 버튼 3개
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartSlotButton> UpgradeBodySlotButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartSlotButton> UpgradeArmSlotButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartSlotButton> UpgradeLegSlotButton;

	// 중앙: 선택한 파츠 상세 (3D 프리뷰 포함)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartDetailWidget> SelectedPartDetailWidget;

	// 우측 상단: 리롤 박스
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RerollRangeText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RerollCostText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> RerollButton;

	// 우측 하단: 업그레이드 박스
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UpgradePreviewText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UpgradeChanceText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UpgradeCostText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> UpgradeButton;

	// 슬롯 태그 (에디터에서 Part.Slot.Body 등 지정 — NSPartEquipWidget과 동일 패턴)
	UPROPERTY(EditDefaultsOnly, Category = "Part", meta = (Categories = "Part.Slot"))
	FGameplayTag BodySlotTag;

	UPROPERTY(EditDefaultsOnly, Category = "Part", meta = (Categories = "Part.Slot"))
	FGameplayTag ArmSlotTag;

	UPROPERTY(EditDefaultsOnly, Category = "Part", meta = (Categories = "Part.Slot"))
	FGameplayTag LegSlotTag;

private:
	UNSPartEquipComponent* GetEquipComponent() const;
	UNSCurrencyComponent* GetCurrencyComponent() const;

	void BindComponentDelegates();
	void UnbindComponentDelegates();

	void RefreshBalance();
	void RefreshEquippedDisplays();
	void RefreshStockEntries();
	void RefreshBuyBox();
	void RefreshUpgradePanels();

	// 슬롯 버튼 하나의 표시 갱신 공용 처리
	void ApplySlotButtonDisplay(FGameplayTag SlotTag, UNSPartSlotButton* SlotButton) const;

	// 프리뷰 스테이지에 메시 로드/교체 후 대상 디테일 위젯에 연결 (Def가 없으면 프리뷰 숨김)
	void UpdatePreview(const UNSPartDefinition* Def, UNSPartDetailWidget* TargetDetail);

	// 현재 선택된 업그레이드 슬롯의 파츠로 중앙 프리뷰 갱신
	void RefreshSelectedSlotPreview();

	// 컴포넌트 델리게이트 핸들러
	void HandlePartChanged(FGameplayTag PartSlot, const FNSPartData& Part);
	void HandleTempChanged(int64 NewAmount);
	void HandleUpgradeResult(FGameplayTag PartSlot, ENSPartUpgradeResult Result, int64 NewTempBalance);

	// Wallet 프로퍼티 복제 도착을 기다리지 않고, 서버가 결과와 함께 보낸 잔액으로 즉시 표시
	void SetBalanceText(int64 Balance);
	void HandleShopStockChanged();

	// 재고 엔트리 클릭 (Index 페이로드)
	void OnStockEntryClicked(const FNSPartDefinitionRow& Row, UNSPartCatalogEntryWidget* Entry, int32 StockIndex);

	// 슬롯 선택 (Page 2)
	void OnUpgradeBodyClicked();
	void OnUpgradeArmClicked();
	void OnUpgradeLegClicked();
	void SelectUpgradeSlot(FGameplayTag SlotTag);

	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnHubPurchaseClicked();

	UFUNCTION()
	void OnHubUpgradeClicked();

	UFUNCTION()
	void OnBackClicked();

	UFUNCTION()
	void OnBuyClicked();

	UFUNCTION()
	void OnRerollClicked();

	UFUNCTION()
	void OnUpgradeClicked();

	// 결과 토스트 표시 (팝업 인스턴스는 최초 1회 생성 후 재사용)
	void ShowResultToast(const FText& Message);

	UPROPERTY()
	TWeakObjectPtr<APlayerController> OwningController;

	UPROPERTY()
	TObjectPtr<class UNSNoticePopupWidget> NoticePopup;

	UPROPERTY()
	TArray<TObjectPtr<UNSPartCatalogEntryWidget>> StockEntryWidgets;

	UPROPERTY()
	TObjectPtr<ANSPartPreviewStage> PreviewStage;

	TSharedPtr<FStreamableHandle> PreviewMeshLoadHandle;

	int32 SelectedStockIndex = INDEX_NONE;
	FGameplayTag SelectedUpgradeSlot;

	FDelegateHandle PartChangedHandle;
	FDelegateHandle TempChangedHandle;
	FDelegateHandle UpgradeResultHandle;
	FDelegateHandle StockChangedHandle;
};
