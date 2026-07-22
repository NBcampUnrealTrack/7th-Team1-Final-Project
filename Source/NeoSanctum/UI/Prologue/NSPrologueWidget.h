// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSPrologueWidget.generated.h"

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSPrologueWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	// 홀드 진행률(0~1)을 게이지에 반영
	UFUNCTION(BlueprintImplementableEvent, Category = "Intro")
	void SetSkipProgress(float Ratio);
};
