// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "NeoSanctum/UI/Interaction/NSNPCInteractionWidgetBase.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NeoSanctum/Core/PlayerState/NSProgressTypes.h"
#include "NSPartEquipWidget.generated.h"

class UNSPartDefinition;
class UNSPartCatalogEntryWidget;
class UNSPartDetailWidget;
class UPanelWidget;
class UButton;
class UTextBlock;
class ANSPartPreviewStage;
class UNSPartSlotButton;

UENUM()
enum class ENSPartSelectionMode : uint8
{
	None,
	Part,        // 카탈로그에서 파츠 선택 (SelectedRow 사용)
	SlotUnlock,  // 잠긴 슬롯버튼 클릭 (SelectedSlotForUnlock 사용)
	UnequipPart, // 해금된 슬롯버튼 클릭 + 그 슬롯에 파츠가 장착됨
};

UCLASS()
class NEOSANCTUM_API UNSPartEquipWidget : public UNSNPCInteractionWidgetBase
{
	GENERATED_BODY()
public:
	virtual void OpenForInteractor(APlayerController* Interactor) override;

	// 닫기 호출 (변경 있으면 다이얼로그)
	UFUNCTION(BlueprintCallable, Category = "Part")
	void RequestClose();

	// 슬롯 언락 요청 (활성 캐릭터 기준)
	UFUNCTION(BlueprintCallable, Category = "Part")
	bool RequestUnlockSlot(FGameplayTag PartSlot);

	// 슬롯 언락 여부 (활성 캐릭터 기준)
	UFUNCTION(BlueprintPure, Category = "Part")
	bool IsSlotUnlocked(FGameplayTag PartSlot) const;

	// 현재 장착된 파츠가 이 슬롯 소속인지 (슬롯버튼 활성 표시용)
	UFUNCTION(BlueprintPure, Category = "Part")
	bool IsSlotEquipped(FGameplayTag PartSlot) const;

	// 슬롯 언락 비용
	UFUNCTION(BlueprintPure, Category = "Part")
	int64 GetSlotUnlockCost(FGameplayTag PartSlot) const;

	// 전체 슬롯 row 목록
	UFUNCTION(BlueprintPure, Category = "Part")
	TArray<FNSPartSlotRow> GetAllSlotRows() const;

	// 영구재화로 파츠 언락 (Common 등급)
	UFUNCTION(BlueprintCallable, Category = "Part")
	bool RequestUnlockPart(TSoftObjectPtr<UNSPartDefinition> Definition);

	// 소유한 파츠를 장착 저장 (Common 등급)
	UFUNCTION(BlueprintCallable, Category = "Part")
	void RequestEquipPart(TSoftObjectPtr<UNSPartDefinition> Definition);

	// 해당 파츠(Common 등급) 소유 여부
	UFUNCTION(BlueprintPure, Category = "Part")
	bool IsPartOwned(TSoftObjectPtr<UNSPartDefinition> Definition) const;

	// 현재 캐릭터의 장착 파츠 (미장착 시 Definition == null)
	UFUNCTION(BlueprintPure, Category = "Part")
	FNSPartSaveData GetEquippedPart() const;

	// DT 전체 row 목록 (bEnabled 포함 전부 — 필터는 BP에서)
	UFUNCTION(BlueprintPure, Category = "Part")
	TArray<FNSPartDefinitionRow> GetAllPartRows() const;

	// 현재 영구 재화량
	UFUNCTION(BlueprintPure, Category = "Part")
	int64 GetCommonCurrency() const;

	// 카탈로그 항목 클릭 시 후보로 선택 (중앙 설명 패널 갱신). SourceEntry는 선택 표시(테두리 on/off)용
	UFUNCTION(BlueprintCallable, Category = "Part")
	void SelectCatalogPart(const FNSPartDefinitionRow& Row, UNSPartCatalogEntryWidget* SourceEntry = nullptr);

protected:
	// 실제 닫기 + 입력모드 복구
	virtual void OnCloseWidget() override;

	/**
	 * FInputModeUIOnly가 게임 입력을 완전히 차단해 ANSPlayerController의 네이티브 ESC 바인딩이
	 * 도달하지 못하므로, 포커스를 가진 이 위젯이 직접 ESC를 가로채 닫기 처리.
	 */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 닫기 시 변경사항이 있으면 "저장하는 중" 팝업을 띄우고, 저장 완료 콜백에서 실제로 닫는다
	void HandleSaveComplete(bool bSuccess);

	// 에디터 Class Defaults에서 WBP_NoticePopup 지정 (저장 진행 표시용)
	UPROPERTY(EditDefaultsOnly, Category = "Part")
	TSubclassOf<class UNSNoticePopupWidget> NoticePopupClass;

	// 재사용 캐시 (닫기 시점에 생성)
	UPROPERTY()
	TObjectPtr<class UNSNoticePopupWidget> NoticePopup;

	// 저장 완료 대기 중 중복 닫기 요청 방지
	bool bSavePending = false;

	// 바디 카탈로그 항목을 채울 컨테이너 (WBP에서 이름 일치 필요)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> BodyListContainer;

	// 암 카탈로그 항목을 채울 컨테이너 (WBP에서 이름 일치 필요)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ArmListContainer;

	// 레그 카탈로그 항목을 채울 컨테이너 (WBP에서 이름 일치 필요)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> LegListContainer;

	// 왼쪽 "장착중" 3박스 클릭 버튼 (WBP에서 이름 일치 필요, 장착된 파츠 아이콘/이름/수치 표시 겸용)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartSlotButton> BodyEquippedButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartSlotButton> ArmEquippedButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartSlotButton> LegEquippedButton;

	// 왼쪽 "장착중인 파츠 설명" (WBP에서 이름 일치 필요)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartDetailWidget> EquippedDetailWidget;

	// 중앙 "선택한 파츠 설명" (WBP에서 이름 일치 필요)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNSPartDetailWidget> SelectedDetailWidget;

	// 중앙 "장착 버튼" (WBP에서 이름 일치 필요)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> EquipButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EquipButtonText;

	// EquipButton 호버 시 겹쳐 보여줄 하이라이트 이미지 (WBP에서 배치, 기본 숨김)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> EquipButtonHoverHighlight;

	// EquipButton을 누르고 있는 동안 보여줄 눌림 이미지 (WBP에서 배치, 기본 숨김)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> EquipButtonPressedHighlight;

	// 공통 재화(영구 재화) 표시 (WBP에서 이름 일치 필요)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CommonCurrencyText;

	// 종료(X) 버튼 — ESC와 동일하게 RequestClose 경유 (저장 진행 표시 포함)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	// 에디터 Class Defaults에서 WBP_PartCatalogEntry 지정
	UPROPERTY(EditDefaultsOnly, Category = "Part")
	TSubclassOf<UNSPartCatalogEntryWidget> PartEntryTemplate;

	// 에디터 Class Defaults에서 BP_PartPreviewStage 지정 (중앙 Detail 3D 프리뷰용)
	UPROPERTY(EditDefaultsOnly, Category = "Part")
	TSubclassOf<ANSPartPreviewStage> PreviewStageClass;

	// 카탈로그를 3분할할 때 사용하는 슬롯 태그 (에디터에서 Part.Slot.Body 등 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Part", meta = (Categories = "Part.Slot"))
	FGameplayTag BodySlotTag;

	UPROPERTY(EditDefaultsOnly, Category = "Part", meta = (Categories = "Part.Slot"))
	FGameplayTag ArmSlotTag;

	UPROPERTY(EditDefaultsOnly, Category = "Part", meta = (Categories = "Part.Slot"))
	FGameplayTag LegSlotTag;

private:
	void BuildPartEntries();
	UPanelWidget* GetCatalogContainerForSlot(FGameplayTag SlotTag) const;

	UFUNCTION()
	void OnEquipButtonClicked();

	UFUNCTION()
	void OnBodyEquippedClicked();

	UFUNCTION()
	void OnArmEquippedClicked();

	UFUNCTION()
	void OnLegEquippedClicked();

	UFUNCTION()
	void OnEquipButtonHovered();

	UFUNCTION()
	void OnEquipButtonUnhovered();

	UFUNCTION()
	void OnEquipButtonPressed();

	UFUNCTION()
	void OnEquipButtonReleased();

	void OnSlotButtonClicked(FGameplayTag SlotTag);
	void RefreshEquippedDisplay();
	void RefreshEquippedSlotButton(UNSPartSlotButton* Button, FGameplayTag SlotTag, FGameplayTag EquippedSlot,
		const FNSPartData& EquippedPartData, UNSPartDefinition* EquippedDef);
	void RefreshEquipButton();
	void RefreshCommonCurrencyDisplay();
	void RequestUnequipPart();

	// 현재 SelectionMode에 맞춰 카탈로그 항목/슬롯버튼 3개의 선택 표시를 갱신
	void RefreshSelectionHighlights();

	bool bDirty = false;

	// 현재 가운데 패널/버튼이 어떤 대상을 가리키는지
	ENSPartSelectionMode SelectionMode = ENSPartSelectionMode::None;

	// SelectionMode == Part일 때 사용
	FNSPartDefinitionRow SelectedRow;

	// SelectionMode == SlotUnlock일 때 사용
	FNSPartSlotRow SelectedSlotForUnlock;

	// SelectionMode == SlotUnlock 또는 UnequipPart일 때, 선택된 슬롯버튼을 가리키는 태그 (하이라이트용)
	FGameplayTag SelectedSlotTag;

	// SelectionMode == Part일 때, 선택 표시(테두리)를 켜둔 카탈로그 항목 위젯
	UPROPERTY()
	TWeakObjectPtr<UNSPartCatalogEntryWidget> SelectedCatalogEntryWidget;

	UPROPERTY()
	TObjectPtr<ANSPartPreviewStage> PreviewStage;

	TSharedPtr<FStreamableHandle> PreviewMeshLoadHandle;
};
