// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSDamageNumberWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;
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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DamageText;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> PopupAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Number")
	FLinearColor NormalDamageColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "Damag Number")
	FLinearColor CriticalDamageColor = FLinearColor(1.0f, 0.1f, 0.1, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Damage Number")
	FVector2D NormalRenderScale = FVector2D(1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Damage Number")
	FVector2D CriticalRenderScale = FVector2D(1.25f, 1.25f);
};
