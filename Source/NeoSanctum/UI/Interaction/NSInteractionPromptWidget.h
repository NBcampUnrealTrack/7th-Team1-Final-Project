// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSInteractionPromptWidget.generated.h"


class UTextBlock;
class UImage;
class UBorder;
struct FStreamableHandle;

// 등급 한 단계의 프롬프트 색상 (배경 + 강조)
USTRUCT(BlueprintType)
struct FNSRarityPromptStyle
{
	GENERATED_BODY()

	// 카드 전체 배경색
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FLinearColor BackgroundColor = FLinearColor(0.10f, 0.10f, 0.12f, 0.50f);

	// 상단/좌측 강조선 색
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FLinearColor AccentColor = FLinearColor(0.50f, 0.50f, 0.50f, 1.0f);
};

UCLASS()
class NEOSANCTUM_API UNSInteractionPromptWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UNSInteractionPromptWidget();

	//프롬프트 텍스트 설정
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetPromptText(const FText& KeyText, const FText& ActionText);

	//프롬프트 아이콘 설정 (비동기 로드)
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetPromptIcon(const TSoftObjectPtr<UTexture2D>& Icon);

	// 파츠 이름 설정
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetPartName(const FText& InName);

	// 파츠 교체 시 스탯 비교 표시: "{StatName} : {OldValue} -> {NewValue}" + 좋아짐/나빠짐 화살표
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetStatComparison(const FText& StatName, float OldValue, float NewValue, bool bHigherIsBetter);

	// 비교할 스탯 정보가 없을 때 (StatTag 매칭 실패 등) 비교 UI 숨김
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ClearStatComparison();

	// 등급 인덱스로 배경/강조 색 적용 (-1 = 기본)
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetRarityStyle(int32 RarityIndex);

protected:
	virtual void NativeConstruct() override;

	// 등급 인덱스 → 해당 스타일 반환 (0/그 외=Common, 1=Rare, 2=Epic, 3=Legendary)
	const FNSRarityPromptStyle& GetRarityStyle(int32 RarityIndex) const;

	// ===== 등급별 색상 (에디터에서 조절 가능) =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FNSRarityPromptStyle CommonStyle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FNSRarityPromptStyle RareStyle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FNSRarityPromptStyle EpicStyle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FNSRarityPromptStyle LegendaryStyle;
	
	//상호작용 키 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UTextBlock> KeyText;
	//상호작용 액션 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional),Category = "UI")
	TObjectPtr<UTextBlock> ActionText;
	//프롬프트 아이콘
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UImage> PartIconImage;
	
	// 파츠 이름 텍스트 (아이콘 아래)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UTextBlock> PartNameText;
	
	// 카드 전체 배경 Border —> 등급별 배경색 적용 대상
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UBorder> RarityBackgroundBorder;
	
	// 카드 상단,좌측 강조선 Image —> 등급별 강조색 적용 대상
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UImage> RarityAccentImage;

	// 스탯 비교 텍스트: "{StatName} : {OldValue} -> {NewValue}"
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UTextBlock> StatCompareText;

	// 좋아짐 화살표 이미지 — 위젯 블루프린트에서 원하는 브러시(색/모양)를 직접 지정
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UImage> StatArrowUpImage;

	// 나빠짐 화살표 이미지 — 위젯 블루프린트에서 원하는 브러시(색/모양)를 직접 지정
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UImage> StatArrowDownImage;

private:
	//진행 중인 아이콘 비동기 로드 핸들
	TSharedPtr<FStreamableHandle> IconLoadHandle;
};
