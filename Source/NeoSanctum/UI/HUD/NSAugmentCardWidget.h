// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NeoSanctum/Type/NSAugmentDisplayTypes.h"
#include "NSAugmentCardWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;
class USizeBox;
class UWidget;

/**
 * 증강 카드 한 장의 표시 전용 위젯입니다.
 * 입력 안내 아이콘(1/2/3/4)은 카드 바깥 UI이므로 UNSAugmentationWidget에서 관리합니다.
 */
UCLASS()
class NEOSANCTUM_API UNSAugmentCardWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// 증강 이름 설정
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetAugmentName(const FString& NewName);

	// 증강 설명 설정
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetAugmentDescription(const FString& NewDescription);

	// 카드 선택/강조 상태 설정
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetHighLighted(bool bHighLighted);

	// 증강 아이콘 설정
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetAugmentIcon(UTexture2D* NewIcon);

	// 입력 안내는 UNSAugmentationWidget에서 표시. 기존 BP/C++ 호환을 위해 함수만 유지
	UFUNCTION(BlueprintCallable, Category = "UI",
		meta = (DeprecatedFunction, DeprecationMessage = "Shortcut input icons are owned by NSAugmentationWidget."))
	void SetShortcutNumber(int32 NewShortcutNumber);

	// Bridge가 완성한 ViewData를 카드 UI에 적용
	UFUNCTION(BlueprintCallable, Category = "UI|Augment")
	void ApplyViewData(const FNSAugmentCardViewData& ViewData);

protected:
	virtual void NativeConstruct() override;

private:
	// 현재 희귀도/강조 상태에 맞춰 카드 배경 텍스처를 갱신
	void RefreshCardVisual();

	// 현재 희귀도/강조 상태에 사용할 카드 텍스처를 반환
	UTexture2D* GetCardTextureForCurrentState() const;
	
	void EnsureCardContentVisible();

	// 현재 카드에 적용된 희귀도
	ENSAugmentRarity CurrentRarity = ENSAugmentRarity::Common;

	// 현재 카드 강조 여부
	bool bIsHighlighted = false;

	// 카드 배경/프레임 이미지
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CardBackground;

	// 증강 이름 텍스트
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AugmentNameText;

	// 증강 설명 텍스트
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AugmentDescriptionText;

	// 증강 아이콘
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> AugmentIcon;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment|Card Texture")
	TObjectPtr<UTexture2D> CommonCardTexture;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment|Card Texture")
	TObjectPtr<UTexture2D> CommonHighlightedCardTexture;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment|Card Texture")
	TObjectPtr<UTexture2D> RareCardTexture;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment|Card Texture")
	TObjectPtr<UTexture2D> RareHighlightedCardTexture;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment|Card Texture")
	TObjectPtr<UTexture2D> EpicCardTexture;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment|Card Texture")
	TObjectPtr<UTexture2D> EpicHighlightedCardTexture;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment|Card Texture")
	TObjectPtr<UTexture2D> LegendaryCardTexture;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment|Card Texture")
	TObjectPtr<UTexture2D> LegendaryHighlightedCardTexture;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> CardSizeBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CardContentHorizontalBox;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment")
	FVector2D CardDisplaySize = FVector2D(330.f, 118.f);
};
