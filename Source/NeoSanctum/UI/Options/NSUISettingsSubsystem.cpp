// Copyright 2026 One Team. All rights reserved.


#include "NSUISettingsSubsystem.h"
#include "Misc/ConfigCacheIni.h"

namespace NSUISettings
{
	const TCHAR* ConfigSection =
		TEXT("/Script/NeoSanctum.UISettings");
}


void UNSUISettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	LoadSettings();
}

FLinearColor UNSUISettingsSubsystem::GetCrosshairColor() const
{
	return CrosshairColor;
}

void UNSUISettingsSubsystem::SetCrosshairColor(FLinearColor NewColor)
{
	NewColor.R = FMath::Clamp(NewColor.R, 0.0f, 1.0f);
	NewColor.G = FMath::Clamp(NewColor.G, 0.0f, 1.0f);
	NewColor.B = FMath::Clamp(NewColor.B, 0.0f, 1.0f);
	NewColor.A = 1.0f;
	
	if (CrosshairColor.Equals(NewColor))
	{
		return;
	}
	
	CrosshairColor = NewColor;
	
	SaveSettings();
	OnCrosshairColorChanged.Broadcast(CrosshairColor);
}

void UNSUISettingsSubsystem::ResetCrosshairColor()
{
	SetCrosshairColor(FLinearColor::White);
}

void UNSUISettingsSubsystem::LoadSettings()
{
	if (!GConfig)
	{
		return;
	}
	
	GConfig->GetFloat(
		NSUISettings::ConfigSection,
		TEXT("CrosshairColorR"),
		CrosshairColor.R,
		GGameUserSettingsIni);
	
	GConfig->GetFloat(
		NSUISettings::ConfigSection,
		TEXT("CrosshairColorG"),
		CrosshairColor.G,
		GGameUserSettingsIni);
	
	GConfig->GetFloat(
		NSUISettings::ConfigSection,
		TEXT("CrosshairColorB"),
		CrosshairColor.B,
		GGameUserSettingsIni);
	
	CrosshairColor.A = 1.0f;
}

void UNSUISettingsSubsystem::SaveSettings() const
{
	if (!GConfig)
	{
		return;
	}
	
	GConfig->SetFloat(
		NSUISettings::ConfigSection,
		TEXT("CrosshairColorR"),
		CrosshairColor.R,
		GGameUserSettingsIni);
	
	GConfig->SetFloat(
		NSUISettings::ConfigSection,
		TEXT("CrosshairColorG"),
		CrosshairColor.G,
		GGameUserSettingsIni);
	
	GConfig->SetFloat(
		NSUISettings::ConfigSection,
		TEXT("CrosshairColorB"),
		CrosshairColor.B,
		GGameUserSettingsIni);
	
	GConfig->Flush(false, GGameUserSettingsIni);
}
