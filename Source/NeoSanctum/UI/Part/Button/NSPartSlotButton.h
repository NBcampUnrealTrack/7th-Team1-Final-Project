// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "Engine/StreamableManager.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NSPartSlotButton.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UNSPartDefinition;

/**
 * 창착된 파츠 하나를 표시하는 슬롯 버튼
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NEOSANCTUM_API UNSPartSlotButton : public UCommonButtonBase
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

	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UImage> PartIconImage;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PartNameText;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PartValueText;

	// 파츠 등급만 별도로 표시하는 텍스트 (아웃런 상점 레이아웃용)
	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PartRarityText;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RarityBorder;

	// 선택 표시용 오버레이 위젯 (WBP에서 배치, 기본 숨김)
	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SelectedHighlight;

private:
	void RefreshEmptyState();
	FText GetRarityText(ENSPartRarity Rarity) const;

	bool bHasPart;
	TSharedPtr<FStreamableHandle> IconLoadHandle;
};
