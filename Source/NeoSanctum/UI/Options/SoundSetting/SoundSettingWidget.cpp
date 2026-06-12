// Copyright 2026 One Team. All rights reserved.


#include "SoundSettingWidget.h"

#include "CommonTextBlock.h"
#include "Components/Slider.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSoundSubsystem.h"

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
}

void USoundSettingWidget::OnMasterVolumeChanged(float Value)
{
	UpdateVolumeText(MasterVolumeText, Value);
	if (UNSSoundSubsystem* SoundManager = UNSSoundSubsystem::Get(this))
	{
		SoundManager->SetMasterVolume(Value);
	}
}

void USoundSettingWidget::OnBGMVolumeChanged(float Value)
{
	UpdateVolumeText(BGMVolumeText, Value);
	if (UNSSoundSubsystem* SoundManager = UNSSoundSubsystem::Get(this))
	{
		SoundManager->SetCategoryVolume(ENSSoundCategory::BGM, Value);
	}
}

void USoundSettingWidget::OnSFXVolumeChanged(float Value)
{
	UpdateVolumeText(SFXVolumeText, Value);
	if (UNSSoundSubsystem* SoundManager = UNSSoundSubsystem::Get(this))
	{
		SoundManager->SetCategoryVolume(ENSSoundCategory::SFX, Value);
	}
}

void USoundSettingWidget::OnUIVolumeChanged(float Value)
{
	UpdateVolumeText(UIVolumeText, Value);
	if (UNSSoundSubsystem* SoundManager = UNSSoundSubsystem::Get(this))
	{
		SoundManager->SetCategoryVolume(ENSSoundCategory::UI, Value);
	}
}

void USoundSettingWidget::UpdateVolumeText(UTextBlock* Text, float Value)
{
	if (Text)
	{
		Text->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f))));
	}
}
