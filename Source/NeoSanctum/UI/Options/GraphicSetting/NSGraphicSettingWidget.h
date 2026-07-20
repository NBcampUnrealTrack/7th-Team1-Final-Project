// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Components/ComboBoxString.h"
#include "NSGraphicSettingWidget.generated.h"

class UCommonButtonBase;
class UCheckBox;
class UComboBoxString;
class UGameUserSettings;
class UTextBlock;
class UWidget;

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
	void InitializeAntiAliasingOptions();
	void SynchronizeSettings();
	
	void HandleTextRevisionChanged();
	
	UFUNCTION()
	void OnApplyClicked();
	
	UFUNCTION()
	void OnResetClicked();
	
	// 그래픽 ComboBox의 선택 항목 TextBlock을 생성한다.
	UFUNCTION()
	UWidget* GenerateGraphicOptionWidget(FString Item);

	void BindOptionWidgetGenerators();
	void UnbindOptionWidgetGenerators();
	
	UGameUserSettings* GetGameUserSettings() const;
	
	// 안티앨리어싱을 제외한 기본 그래픽 품질을 설정한다.
    void SetBaseQualityLevel(
    	UGameUserSettings* Settings,
    	int32 QualityLevel) const;
    
    // 현재 기본 그래픽 품질을 반환한다.
    int32 GetBaseQualityLevel(
    	const UGameUserSettings* Settings) const;
	
	void MakeComboBoxPopupTransparent(UComboBoxString* ComboBox);
	
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> ResolutionComboBox;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> WindowModeComboBox;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> FrameRateComboBox;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> OverallQualityComboBox;
	// 수직 동기화 활성화 여부를 선택하는 체크박스
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> VSyncCheckBox;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> ApplyButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> ResetButton;
	// 안티앨리어싱 품질 선택 ComboBox
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> AntiAliasingQualityComboBox;
	
	// 생성한 항목 위젯을 그래픽 설정 위젯의 수명 동안 보관한다.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> GeneratedOptionWidgets;
	
	TArray<FIntPoint> SupportedResolutions;
	TArray<float> FrameRateLimits;
	
	UFUNCTION()
	void HandleComboBoxSelectionChanged(
		FString SelectedItem,
		ESelectInfo::Type SelectInfo);
	
	void CenterSelectedOptionText(UComboBoxString* ComboBox);
	
	void CenterAllSelectedOptionTexts();
	
};
