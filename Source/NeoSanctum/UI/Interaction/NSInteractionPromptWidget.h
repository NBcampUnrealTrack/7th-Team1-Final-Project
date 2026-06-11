// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSInteractionPromptWidget.generated.h"


class UTextBlock;

/**
 * 
 */

UCLASS()
class NEOSANCTUM_API UNSInteractionPromptWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//프롬프트 텍스트 설정
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetPromptText(const FText& KeyText, const FText& ActionText);
	
protected:
	virtual void NativeConstruct() override;
	
	//상호작용 키 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UTextBlock> KeyText;
	//상호작용 액션 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional),Category = "UI")
	TObjectPtr<UTextBlock> ActionText;
};
