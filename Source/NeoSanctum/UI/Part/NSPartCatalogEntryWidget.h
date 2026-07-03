// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/StreamableManager.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NSPartCatalogEntryWidget.generated.h"

class UButton;
class UImage;
class UNSPartEquipWidget;

UCLASS(Abstract, BlueprintType, Blueprintable)
class NEOSANCTUM_API UNSPartCatalogEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetupEntry(const FNSPartDefinitionRow& Row, UNSPartEquipWidget* OwnerWidget);

	// 이 항목이 현재 선택된 상태인지 표시 (SelectedIndicator 보임/숨김)
	UFUNCTION(BlueprintCallable, Category = "UI|Part")
	void SetIsSelected(bool bSelected);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	// 항목 전체를 클릭 가능하게 만드는 버튼 (WBP에서 배경 버튼으로 사용)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> SelectButton;

	// 선택됨을 표시하는 테두리/배경 위젯 (WBP에서 배치, 기본 숨김)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SelectedIndicator;

private:
	void OnDefinitionLoaded();

	UFUNCTION()
	void OnSelectButtonClicked();

	FNSPartDefinitionRow StoredRow;
	TWeakObjectPtr<UNSPartEquipWidget> OwnerRef;
	TSharedPtr<FStreamableHandle> LoadHandle;
};
