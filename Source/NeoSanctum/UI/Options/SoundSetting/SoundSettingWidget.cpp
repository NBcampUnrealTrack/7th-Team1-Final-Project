// Copyright 2026 One Team. All rights reserved.


#include "SoundSettingWidget.h"

#include "Components/Button.h"
#include "Components/Slider.h"

void USoundSettingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.AddDynamic(this, &USoundSettingWidget::OnMasterVolumeChanged);
	}
	if (BGMVolumeSlider)
	{
		BGMVolumeSlider->OnValueChanged.AddDynamic(this, &USoundSettingWidget::OnBGMVolumeChanged);
	}
	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->OnValueChanged.AddDynamic(this, &USoundSettingWidget::OnSFXVolumeChanged);
	}
	if (UIVolumeSlider)
	{
		UIVolumeSlider->OnValueChanged.AddDynamic(this, &USoundSettingWidget::OnUIVolumeChanged);
	}
	
	if (ApplyButton)
	{
		ApplyButton->OnClicked.AddDynamic(this, &USoundSettingWidget::OnApplyClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &USoundSettingWidget::OnBackClicked);
	}
}

void USoundSettingWidget::OnMasterVolumeChanged(float Value)
{
}

void USoundSettingWidget::OnBGMVolumeChanged(float Value)
{
}

void USoundSettingWidget::OnSFXVolumeChanged(float Value)
{
}

void USoundSettingWidget::OnUIVolumeChanged(float Value)
{
}

void USoundSettingWidget::OnApplyClicked()
{
}

void USoundSettingWidget::OnBackClicked()
{
}
