// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSReadyPlayerEntry.generated.h"


class UImage;
class UCommonTextBlock;
class UTexture2D;

UCLASS()
class NEOSANCTUM_API UNSReadyPlayerEntry : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	void SetPlayer(
		UTexture2D* ClassIconTexture,  
		const FString& PlayerName,
		const FString& ClassName, 
		bool bIsReady);

	void ClearSlot();

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> FrameImage;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ClassIconImage;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> PlayerNameText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> ClassNameText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> ReadyStatusText;

	// 프레임 기본/활성 스프라이트 (스프라이트라 UObject + AllowedClasses)
	UPROPERTY(EditDefaultsOnly, Category = "Ready|Frame")
	TObjectPtr<UObject> FrameDefaultSprite;

	UPROPERTY(EditDefaultsOnly, Category = "Ready|Frame")
	TObjectPtr<UObject> FrameActiveSprite;
};
