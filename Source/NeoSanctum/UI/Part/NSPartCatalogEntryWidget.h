// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NeoSanctum/UI/Common/NSButtonBase.h"
#include "NSPartCatalogEntryWidget.generated.h"

class UImage;
class UNSPartEquipWidget;

DECLARE_DELEGATE_TwoParams(FNSOnCatalogEntryClicked, const FNSPartDefinitionRow&, UNSPartCatalogEntryWidget*);

UCLASS(Abstract, BlueprintType, Blueprintable)
class NEOSANCTUM_API UNSPartCatalogEntryWidget : public UNSButtonBase
{
	GENERATED_BODY()

public:
	UNSPartCatalogEntryWidget();

	void SetupEntry(const FNSPartDefinitionRow& Row, UNSPartEquipWidget* OwnerWidget);

	// 소유 위젯 타입에 묶이지 않는 범용 진입점 (인런 상점 등에서 사용)
	void SetupEntry(const FNSPartDefinitionRow& Row, FNSOnCatalogEntryClicked InClickHandler);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	// 선택됨을 표시하는 테두리/배경 위젯 (WBP에서 배치, 기본 숨김)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SelectedIndicator;

	// 마우스 호버 시 아이콘 위에 겹쳐 보여줄 글로우/하이라이트 이미지 (WBP에서 배치, 기본 숨김)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> HoverHighlight;

	// 마우스로 누르고 있는 동안, 또는 선택된 상태에서 계속 보여줄 눌림 이미지 (WBP에서 배치, 기본 숨김)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> PressedHighlight;

private:
	void OnDefinitionLoaded();

	void HandleClicked();
	void HandleHovered();
	void HandleUnhovered();
	void HandlePressed();
	void HandleReleased();
	void HandleSelectionChanged(bool bInSelected);

	FNSPartDefinitionRow StoredRow;
	TWeakObjectPtr<UNSPartEquipWidget> OwnerRef;
	TSharedPtr<FStreamableHandle> LoadHandle;
	FNSOnCatalogEntryClicked ClickHandler;
};
