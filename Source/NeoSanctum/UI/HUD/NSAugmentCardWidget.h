// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NeoSanctum/Type/NSAugmentDisplayTypes.h"
#include "NSAugmentCardWidget.generated.h"

class UBorder;
class UTextBlock;
class UImage;
class UTexture2D;
class UCommonTextBlock;

/**
 *  증강 카드를 표시하는 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSAugmentCardWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//증강 이름 설정
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetAugmentName(const FString& NewName);
	//증강 설명 설정
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetAugmentDescription(const FString& NewDescription);
	//카드 하이라이트 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetHighLighted(bool bHighLighted);
	//증강 아이콘 설정
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetAugmentIcon(UTexture2D* NewIcon);
	//카드 선택 단축기 번호 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetShortcutNumber(int32 NewShortcutNumber);
	//Bridge가 완성한 ViewData를 카드 UI에 적용
	UFUNCTION(BlueprintCallable, Category = "UI|Augment")
	void ApplyViewData(const FNSAugmentCardViewData& ViewData);
	
protected:
	virtual void NativeConstruct() override;
	
private:

	//현재 카드에 적용된 희귀도
	ENSAugmentRarity CurrentRarity = ENSAugmentRarity::Common;
	//현재 희귀도에 대응하는 하이라이트 색상을 반환
	FLinearColor GetRarityHighlightColor() const;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment")
	FLinearColor CommonHighlightColor =
		FLinearColor(0.25f, 0.25f, 0.25f, 1.0f);
	
	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment")
	FLinearColor RareHighlightColor =
		FLinearColor(0.10f, 0.45f, 1.00f, 1.0f);
	
	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment")
	FLinearColor EpicHighlightColor =
		FLinearColor(0.60f, 0.15f, 0.90f, 1.0f);
	
	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment")
	FLinearColor LegendaryHighlightColor =
		FLinearColor(1.00f, 0.70f, 0.10f, 1.0f);
	
	//증강 이름 텍스트
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> AugmentNameText;
	//증강 설명 텍스트
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> AugmentDescriptionText;
	//카드 선택 단축키 번호
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> ShortcutNumberText;
	//카드 테두리
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UBorder> CardBorder;
	//증강 아이콘
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> AugmentIcon;
	
};
