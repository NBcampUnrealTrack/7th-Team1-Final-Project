// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSGameplaySettingWidget.generated.h"

class UImage;
class UNSUISettingsSubsystem;
class USlider;
class UButton;
class UTextBlock;

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSGameplaySettingWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void ApplyCrosshairColor(FLinearColor NewColor);
	
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void ResetCrosshairColor();
	
private:
	UNSUISettingsSubsystem* GetUISettingSubsystem() const;
	
	void UpdateColorPreview(const FLinearColor& NewColor);
	
	UFUNCTION()
    void OnRedValueChanged(float Value);
    
    UFUNCTION()
    void OnGreenValueChanged(float Value);
    
    UFUNCTION()
    void OnBlueValueChanged(float Value);
    
    UFUNCTION()
    void OnApplyCustomColorClicked();
	
    void SynchronizeSliders(const FLinearColor& Color);
	
	void UpdateRGBValueTexts();
	
	void UpdateApplyButtonState();
	
private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CrosshairColorPreview;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> RedSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> GreenSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> BlueSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ApplyCustomColorButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RedValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GreenValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BlueValueText;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ApplyCustomColorText;

	FLinearColor PendingCrosshairColor =
		FLinearColor::White;

	bool bSynchronizingSliders = false;
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
};
