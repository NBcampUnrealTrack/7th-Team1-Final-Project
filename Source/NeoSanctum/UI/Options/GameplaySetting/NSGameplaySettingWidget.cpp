// Copyright 2026 One Team. All rights reserved.


#include "NSGameplaySettingWidget.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "NeoSanctum/UI/Options/NSUISettingsSubsystem.h"

void UNSGameplaySettingWidget::ApplyCrosshairColor(FLinearColor NewColor)
{
	UNSUISettingsSubsystem* Settings =
		GetUISettingSubsystem();
	
	if (!Settings)
	{
		return;
	}
	
	Settings->SetCrosshairColor(NewColor);
	UpdateColorPreview(Settings->GetCrosshairColor());
	
	PendingCrosshairColor = Settings->GetCrosshairColor();
	SynchronizeSliders(PendingCrosshairColor);
	UpdateApplyButtonState();
}

void UNSGameplaySettingWidget::ResetCrosshairColor()
{
	UNSUISettingsSubsystem* Settings =
		GetUISettingSubsystem();
	
	if (!Settings)
	{
		return;
	}
	
	Settings->ResetCrosshairColor();
	UpdateColorPreview(Settings->GetCrosshairColor());
	
	PendingCrosshairColor = Settings->GetCrosshairColor();
	SynchronizeSliders(PendingCrosshairColor);
	UpdateApplyButtonState();
}

UNSUISettingsSubsystem* UNSGameplaySettingWidget::GetUISettingSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	
	return GameInstance
	? GameInstance->GetSubsystem<UNSUISettingsSubsystem>()
	: nullptr;
}

void UNSGameplaySettingWidget::UpdateColorPreview(const FLinearColor& NewColor)
{
	if (CrosshairColorPreview)
	{
		CrosshairColorPreview->SetColorAndOpacity(NewColor);
	}
}

void UNSGameplaySettingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (const UNSUISettingsSubsystem* Settings =
		GetUISettingSubsystem())
	{
		PendingCrosshairColor =
			Settings->GetCrosshairColor();

		UpdateColorPreview(PendingCrosshairColor);
		SynchronizeSliders(PendingCrosshairColor);
	}

	if (RedSlider)
	{
		RedSlider->OnValueChanged.AddDynamic(
			this,
			&ThisClass::OnRedValueChanged);
	}

	if (GreenSlider)
	{
		GreenSlider->OnValueChanged.AddDynamic(
			this,
			&ThisClass::OnGreenValueChanged);
	}

	if (BlueSlider)
	{
		BlueSlider->OnValueChanged.AddDynamic(
			this,
			&ThisClass::OnBlueValueChanged);
	}

	if (ApplyCustomColorButton)
	{
		ApplyCustomColorButton->OnClicked.AddDynamic(
			this,
			&ThisClass::OnApplyCustomColorClicked);
	}
	UpdateApplyButtonState();
}

void UNSGameplaySettingWidget::NativeDestruct()
{
	if (RedSlider)
	{
		RedSlider->OnValueChanged.RemoveAll(this);
	}

	if (GreenSlider)
	{
		GreenSlider->OnValueChanged.RemoveAll(this);
	}

	if (BlueSlider)
	{
		BlueSlider->OnValueChanged.RemoveAll(this);
	}

	if (ApplyCustomColorButton)
	{
		ApplyCustomColorButton->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UNSGameplaySettingWidget::OnRedValueChanged(float Value)
{
	if (bSynchronizingSliders)
	{
		return;
	}

	PendingCrosshairColor.R = Value;
	UpdateColorPreview(PendingCrosshairColor);
	UpdateRGBValueTexts();
	UpdateApplyButtonState();
}

void UNSGameplaySettingWidget::OnGreenValueChanged(float Value)
{
	if (bSynchronizingSliders)
	{
		return;
	}

	PendingCrosshairColor.G = Value;
	UpdateColorPreview(PendingCrosshairColor);
	UpdateRGBValueTexts();
	UpdateApplyButtonState();
}

void UNSGameplaySettingWidget::OnBlueValueChanged(float Value)
{
	if (bSynchronizingSliders)
	{
		return;
	}

	PendingCrosshairColor.B = Value;
	UpdateColorPreview(PendingCrosshairColor);
	UpdateRGBValueTexts();
	UpdateApplyButtonState();
}

void UNSGameplaySettingWidget::OnApplyCustomColorClicked()
{
	ApplyCrosshairColor(PendingCrosshairColor);
}

void UNSGameplaySettingWidget::SynchronizeSliders(
	const FLinearColor& Color)
{
	bSynchronizingSliders = true;

	if (RedSlider)
	{
		RedSlider->SetValue(Color.R);
	}

	if (GreenSlider)
	{
		GreenSlider->SetValue(Color.G);
	}

	if (BlueSlider)
	{
		BlueSlider->SetValue(Color.B);
	}

	bSynchronizingSliders = false;
	UpdateRGBValueTexts();
}

void UNSGameplaySettingWidget::UpdateRGBValueTexts()
{
	if (RedValueText)
	{
		RedValueText->SetText(
			FText::AsNumber(
				FMath::RoundToInt(
					PendingCrosshairColor.R * 255.0f)));
	}

	if (GreenValueText)
	{
		GreenValueText->SetText(
			FText::AsNumber(
				FMath::RoundToInt(
					PendingCrosshairColor.G * 255.0f)));
	}

	if (BlueValueText)
	{
		BlueValueText->SetText(
			FText::AsNumber(
				FMath::RoundToInt(
					PendingCrosshairColor.B * 255.0f)));
	}
}

void UNSGameplaySettingWidget::UpdateApplyButtonState()
{
	const UNSUISettingsSubsystem* Settings =
		GetUISettingSubsystem();

	if (!Settings)
	{
		return;
	}

	const bool bHasPendingChanges =
		!PendingCrosshairColor.Equals(
			Settings->GetCrosshairColor());

	if (ApplyCustomColorButton)
	{
		ApplyCustomColorButton->SetIsEnabled(
			bHasPendingChanges);
	}

	if (ApplyCustomColorText)
	{
		ApplyCustomColorText->SetText(
			bHasPendingChanges
				? NSLOCTEXT(
					"GameplaySettings",
					"ApplyCrosshairColor",
					"적용")
				: NSLOCTEXT(
					"GameplaySettings",
					"AppliedCrosshairColor",
					"적용됨"));
	}
}
