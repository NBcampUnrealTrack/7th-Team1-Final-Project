// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/UI/Common/NSButtonBase.h"
#include "Engine/StreamableManager.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NSPartSlotButton.generated.h"

class UImage;
class UTextBlock;
class UNSPartDefinition;

/**
 * 창착된 파츠 하나를 표시하는 슬롯 버튼
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NEOSANCTUM_API UNSPartSlotButton : public UNSButtonBase
{
	GENERATED_BODY()
	
public:
	UNSPartSlotButton();
	
	/**
	 * 파츠데이터를 슬롯에 표시
	 */
	
	UFUNCTION(BlueprintCallable, Category = "UI|Part")
	void SetPart(const FNSPartData& InPartData, const UNSPartDefinition* InPartDefinition);

	//슬롯을 빈 상태로 표시한다
	UFUNCTION(BlueprintCallable, Category = "UI|Part")
	void ClearPart();

	//슬롯이 비어 있는지 반환한다
	UFUNCTION(BlueprintPure, Category = "UI|Part")
	bool IsEmpty() const;

	// 이 슬롯버튼이 현재 선택된 상태인지 표시 (SelectedHighlight 보임/숨김). CommonButtonStyle에 의존하지 않는 수동 하이라이트
	UFUNCTION(BlueprintCallable, Category = "UI|Part")
	void SetHighlighted(bool bHighlighted);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UImage> PartIconImage;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PartNameText;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PartValueText;

	// 파츠 등급만 별도로 표시하는 텍스트 (아웃런 상점 레이아웃용)
	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PartRarityText;

	// 선택 표시용 오버레이 위젯 (WBP에서 배치, 기본 숨김)
	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SelectedHighlight;

	// 마우스 호버 시 아이콘 위에 겹쳐 보여줄 글로우/하이라이트 이미지 (WBP에서 배치, 기본 숨김)
	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> HoverHighlight;

	// 마우스로 누르고 있는 동안 보여줄 눌림 이미지 (WBP에서 배치, 기본 숨김)
	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> PressedHighlight;

private:
	void RefreshEmptyState();
	FText GetRarityText(ENSPartRarity Rarity) const;

	void HandleHovered();
	void HandleUnhovered();
	void HandlePressed();
	void HandleReleased();

	bool bHasPart;
	bool bIsHighlighted = false;
	TSharedPtr<FStreamableHandle> IconLoadHandle;
};
