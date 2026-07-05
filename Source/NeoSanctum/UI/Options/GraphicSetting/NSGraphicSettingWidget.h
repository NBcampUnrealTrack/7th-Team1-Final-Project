// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSGraphicSettingWidget.generated.h"

class UButton;
class UCheckBox;
class UComboBoxString;
class UGameUserSettings;

/**
 * 옵션에서 그래픽의 대한 부분을 바꾸는 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSGraphicSettingWidget : public UCommonUserWidget
{
	GENERATED_BODY()

private:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	void InitializeResolutionOptions();
	void InitializeWindowModeOptions();
	void InitializeFrameRateOptions();
	void InitializeQualityOptions();
	void SynchronizeSettings();
	
	UFUNCTION()
	void OnApplyClicked();
	
	UFUNCTION()
	void OnResetClicked();
	
	UGameUserSettings* GetGameUserSettings() const;
	
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> ResolutionComboBox;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> WindowModeComboBox;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> FrameRateComboBox;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> OverallQualityComboBox;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> VSyncCheckBox;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ApplyButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResetButton;
	
	TArray<FIntPoint> SupportedResolutions;
	TArray<float> FrameRateLimits;
	
};
