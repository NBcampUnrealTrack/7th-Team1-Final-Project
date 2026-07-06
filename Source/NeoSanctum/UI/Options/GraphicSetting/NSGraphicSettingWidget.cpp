// Copyright 2026 One Team. All rights reserved.


#include "NSGraphicSettingWidget.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Internationalization/TextLocalizationManager.h"

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
	
	FTextLocalizationManager::Get()
	.OnTextRevisionChangedEvent.RemoveAll(this);

	FTextLocalizationManager::Get()
		.OnTextRevisionChangedEvent.AddUObject(
			this,
			&ThisClass::HandleTextRevisionChanged);
}

void UNSGraphicSettingWidget::NativeDestruct()
{
	
	FTextLocalizationManager::Get()
	.OnTextRevisionChangedEvent.RemoveAll(this);
	
	ApplyButton->OnClicked.RemoveAll(this);
	ResetButton->OnClicked.RemoveAll(this);
	
	Super::NativeDestruct();
}

void UNSGraphicSettingWidget::InitializeResolutionOptions()
{
	ResolutionComboBox->ClearOptions();
	SupportedResolutions.Reset();
	
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(
		SupportedResolutions);
	
	UGameUserSettings* Settings =
		GetGameUserSettings();
	
	const FIntPoint DesktopResolution =
		Settings
			? Settings->GetDesktopResolution()
			: FIntPoint::ZeroValue;
	
	SupportedResolutions.RemoveAll(
		[DesktopResolution](const FIntPoint& Resolution)
		{
			const float AspectRatio =
				static_cast<float>(Resolution.X) /
					static_cast<float>(Resolution.Y);
			
			const bool bIs16By9 =
				FMath::IsNearlyEqual(AspectRatio,
					16.0f / 9.0f,
					0.01f);
			
			const bool bIsDesktopResolution =
				Resolution == DesktopResolution;
			
			return !bIs16By9 &&
				!bIsDesktopResolution;
		});
	
	if (DesktopResolution.X > 0 &&
		DesktopResolution.Y > 0)
	{
		SupportedResolutions.AddUnique(
			DesktopResolution);
	}
	
	SupportedResolutions.Sort(
		[](const FIntPoint& Left, const FIntPoint& Right)
		{
			if (Left.X == Right.X)
				{
				return Left.Y < Right.Y;
				}
			
			return Left.X < Right.X;
			});
	
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

	WindowModeComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"Fullscreen",
			"전체 화면").ToString());

	WindowModeComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"BorderlessFullscreen",
			"테두리 없는 전체 화면").ToString());

	WindowModeComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"Windowed",
			"창 모드").ToString());
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
	FrameRateComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"UnlimitedFrameRate",
			"제한 없음").ToString());
}

void UNSGraphicSettingWidget::InitializeQualityOptions()
{
	OverallQualityComboBox->ClearOptions();

	OverallQualityComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"QualityLow",
			"낮음").ToString());

	OverallQualityComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"QualityMedium",
			"중간").ToString());

	OverallQualityComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"QualityHigh",
			"높음").ToString());

	OverallQualityComboBox->AddOption(
		NSLOCTEXT(
			"GraphicSettings",
			"QualityEpic",
			"최상").ToString());
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
	
	FIntPoint SelectedResolution =
		Settings->GetScreenResolution();
	
	int32 ResolutionIndex =
		SupportedResolutions.IndexOfByKey(
			SelectedResolution);
	
	if (ResolutionIndex == INDEX_NONE)
	{
		SelectedResolution =
			Settings->GetLastConfirmedScreenResolution();

		ResolutionIndex =
			SupportedResolutions.IndexOfByKey(
				SelectedResolution);
	}

	if (ResolutionIndex == INDEX_NONE)
	{
		ResolutionIndex =
			SupportedResolutions.IndexOfByKey(
				Settings->GetDesktopResolution());
	}

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

void UNSGraphicSettingWidget::HandleTextRevisionChanged()
{
	InitializeWindowModeOptions();
	InitializeFrameRateOptions();
	InitializeQualityOptions();
	SynchronizeSettings();
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
	Settings->ConfirmVideoMode();
	Settings->SaveSettings();
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
	Settings->ConfirmVideoMode();
	Settings->SaveSettings();
	
	SynchronizeSettings();
}

UGameUserSettings* UNSGraphicSettingWidget::GetGameUserSettings() const
{
	return UGameUserSettings::GetGameUserSettings();
}
