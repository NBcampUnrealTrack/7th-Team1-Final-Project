// Copyright 2026 One Team. All rights reserved.


#include "NSGraphicSettingWidget.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"

void UNSGraphicSettingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	InitializeResolutionOptions();
	InitializeWindowModeOptions();
	InitializeFrameRateOptions();
	InitializeQualityOptions();
	SynchronizeSettings();
	
	ApplyButton->OnClicked.AddDynamic(
		this,
		&ThisClass::OnApplyClicked);
	
	ResetButton->OnClicked.AddDynamic(
		this,
		&ThisClass::OnResetClicked);
}

void UNSGraphicSettingWidget::NativeDestruct()
{
	ApplyButton->OnClicked.RemoveAll(this);
	ResetButton->OnClicked.RemoveAll(this);
	
	Super::NativeDestruct();
}

void UNSGraphicSettingWidget::InitializeResolutionOptions()
{
	ResolutionComboBox->ClearOptions();
	SupportedResolutions.Reset();
	
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(SupportedResolutions);
	
	for (const FIntPoint& Resolution : SupportedResolutions)
	{
		ResolutionComboBox->AddOption(
			FString::Printf(
				TEXT("%d x %d"),
				Resolution.X,
				Resolution.Y));
	}
}

void UNSGraphicSettingWidget::InitializeWindowModeOptions()
{
	WindowModeComboBox->ClearOptions();
	WindowModeComboBox->AddOption(TEXT("전체화면"));
	WindowModeComboBox->AddOption(TEXT("테두리 없음"));
	WindowModeComboBox->AddOption(TEXT("창화면"));
}

void UNSGraphicSettingWidget::InitializeFrameRateOptions()
{
	FrameRateComboBox->ClearOptions();
	
	FrameRateLimits =
	{
		30.0f,
		60.0f,
		120.0f,
		144.0f,
		0.0f
	};
	
	FrameRateComboBox->AddOption(TEXT("30"));
	FrameRateComboBox->AddOption(TEXT("60"));
	FrameRateComboBox->AddOption(TEXT("120"));
	FrameRateComboBox->AddOption(TEXT("144"));
	FrameRateComboBox->AddOption(TEXT("제한 없음"));
}

void UNSGraphicSettingWidget::InitializeQualityOptions()
{
	OverallQualityComboBox->ClearOptions();
	OverallQualityComboBox->AddOption(TEXT("낮음"));
	OverallQualityComboBox->AddOption(TEXT("중간"));
	OverallQualityComboBox->AddOption(TEXT("높음"));
	OverallQualityComboBox->AddOption(TEXT("최상"));
}

void UNSGraphicSettingWidget::SynchronizeSettings()
{
	UGameUserSettings* Settings =
		GetGameUserSettings();
	
	if (!Settings)
	{
		return;
	}
	
	Settings->LoadSettings(false);
	
	const FIntPoint CurrentResolution =
		Settings->GetScreenResolution();
	
	const int32 ResolutionIndex =
		SupportedResolutions.IndexOfByKey(
			CurrentResolution);
	
	if (ResolutionIndex != INDEX_NONE)
	{
		ResolutionComboBox->SetSelectedIndex(
			ResolutionIndex);
	}
	
	switch (Settings->GetFullscreenMode())
	{
	case EWindowMode::Fullscreen:
		WindowModeComboBox->SetSelectedIndex(0);
		break;

	case EWindowMode::WindowedFullscreen:
		WindowModeComboBox->SetSelectedIndex(1);
		break;

	case EWindowMode::Windowed:
	default:
		WindowModeComboBox->SetSelectedIndex(2);
		break;
	}
	
	const float CurrentFrameRate =
		Settings->GetFrameRateLimit();
	
	int32 FrameRateIndex =
		FrameRateLimits.IndexOfByPredicate(
			[CurrentFrameRate](float FrameRate)
			{
				return FMath::IsNearlyEqual(
					FrameRate,
					CurrentFrameRate);
			});
	
	if (FrameRateIndex == INDEX_NONE)
	{
		FrameRateIndex =
			FrameRateLimits.Add(CurrentFrameRate);
		
		FrameRateComboBox->AddOption(
			FString::FromInt(
				FMath::RoundToInt(
					CurrentFrameRate)));
	}
	
	FrameRateComboBox->SetSelectedIndex(FrameRateIndex);
	
	const int32 QualityLevel =
		Settings->GetOverallScalabilityLevel();
	
	OverallQualityComboBox->SetSelectedIndex(
		QualityLevel >= 0 && QualityLevel <= 3
			? QualityLevel
			: 3);
	
	VSyncCheckBox->SetIsChecked(
		Settings->IsVSyncEnabled());
}

void UNSGraphicSettingWidget::OnApplyClicked()
{
	UGameUserSettings* Settings =
		GetGameUserSettings();
	
	if (!Settings)
	{
		return;
	}
	
	const int32 ResolutionIndex =
		ResolutionComboBox->GetSelectedIndex();
	
	if (SupportedResolutions.IsValidIndex(
		ResolutionIndex))
	{
		Settings->SetScreenResolution(
			SupportedResolutions[ResolutionIndex]);
	}
	
	switch (WindowModeComboBox->GetSelectedIndex())
	{
	case 0:
		Settings->SetFullscreenMode(
			EWindowMode::Fullscreen);
		break;

	case 1:
		Settings->SetFullscreenMode(
			EWindowMode::WindowedFullscreen);
		break;

	case 2:
	default:
		Settings->SetFullscreenMode(
			EWindowMode::Windowed);
		break;
	}

	const int32 FrameRateIndex =
		FrameRateComboBox->GetSelectedIndex();

	if (FrameRateLimits.IsValidIndex(
		FrameRateIndex))
	{
		Settings->SetFrameRateLimit(
			FrameRateLimits[FrameRateIndex]);
	}

	const int32 QualityLevel =
		OverallQualityComboBox->GetSelectedIndex();

	if (QualityLevel >= 0 &&
		QualityLevel <= 3)
	{
		Settings->SetOverallScalabilityLevel(
			QualityLevel);
	}

	Settings->SetVSyncEnabled(
		VSyncCheckBox->IsChecked());

	Settings->ApplySettings(false);
	SynchronizeSettings();
}

void UNSGraphicSettingWidget::OnResetClicked()
{
	UGameUserSettings* Settings =
		GetGameUserSettings();
	
	if (!Settings)
	{
		return;
	}
	
	Settings->SetScreenResolution(
		Settings->GetDesktopResolution());
	Settings->SetFullscreenMode(
		EWindowMode::WindowedFullscreen);
	Settings->SetFrameRateLimit(0.0f);
	Settings->SetOverallScalabilityLevel(3);
	Settings->SetVSyncEnabled(false);

	Settings->ApplySettings(false);
	
	SynchronizeSettings();
}

UGameUserSettings* UNSGraphicSettingWidget::GetGameUserSettings() const
{
	return UGameUserSettings::GetGameUserSettings();
}
