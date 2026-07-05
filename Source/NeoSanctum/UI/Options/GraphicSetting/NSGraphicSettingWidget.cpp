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
	SynchronizeSettings();
	
	ApplyButton->OnClicked.AddDynamic(
		this,
		&ThisClass::OnApplyClicked);
	
void UNSGraphicSettingWidget::NativeDestruct()
{
	ApplyButton->OnClicked.RemoveAll(this);
	
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

	Settings->ApplySettings(false);
	SynchronizeSettings();
}

	Settings->ApplySettings(false);
	
	SynchronizeSettings();
}

UGameUserSettings* UNSGraphicSettingWidget::GetGameUserSettings() const
{
	return UGameUserSettings::GetGameUserSettings();
}
