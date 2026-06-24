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
	
protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UImage> PartIconImage;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PartNameText;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PartValueText;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RarityBorder;

private:
	void RefreshEmptyState();
	FText GetRarityText(ENSPartRarity Rarity) const;

	bool bHasPart;
	TSharedPtr<FStreamableHandle> IconLoadHandle;
};
