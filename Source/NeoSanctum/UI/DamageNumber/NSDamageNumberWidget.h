// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSDamageNumberWidget.generated.h"

struct FNSDamageNumberFeedbackContext;
/**
 *
 */
UCLASS()
class NEOSANCTUM_API UNSDamageNumberWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetDamageNumber(const FNSDamageNumberFeedbackContext& Context);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 DisplayDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bCritical = false;
};
