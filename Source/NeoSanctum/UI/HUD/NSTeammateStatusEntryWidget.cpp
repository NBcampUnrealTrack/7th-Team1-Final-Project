// Copyright 2026 One Team. All rights reserved.


#include "NSTeammateStatusEntryWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UNSTeammateStatusEntryWidget::ApplyStatusData(const FNSPlayerStatusViewData& StatusData)
{
	PlayerId = StatusData.PlayerId;
	
	if (PlayerNameText)
	{
		PlayerNameText->SetText(FText::FromString(StatusData.PlayerName));
	}
	
	if (HealthBar)
	{
		HealthBar->SetPercent(
			GetSafePercent(
				StatusData.CurrentHealth,
				StatusData.MaxHealth));
	}
	
	if (ShieldBar)
	{
		ShieldBar->SetPercent(
			GetSafePercent(
				StatusData.CurrentShield,
				StatusData.MaxShield));
	}
	
	if (HealthValueText)
	{
		HealthValueText->SetText(
			FText::Format(
				NSLOCTEXT(
					"TeammateStatus",
					"HealthFormat",
					"{0}/{1}"),
					FText::AsNumber(
						FMath::RoundToInt(
							StatusData.CurrentHealth)),
							FText::AsNumber(
								FMath::RoundToInt(
									StatusData.MaxHealth))));
	}
	
	if (ShieldValueText)
	{
		ShieldValueText->SetText(
			FText::Format(
				NSLOCTEXT(
					"TeammateStatus",
					"ShieldFormat",
					"{0}/{1}"),
				FText::AsNumber(
					FMath::RoundToInt(
						StatusData.CurrentShield)),
				FText::AsNumber(
					FMath::RoundToInt(
						StatusData.MaxShield))));
	}
	if (ExperienceBar)
	{
		ExperienceBar->SetPercent(
			GetSafePercent(
				StatusData.CurrentExperience,
				StatusData.RequiredExperience));
	}

	if (ExperienceValueText)
	{
		ExperienceValueText->SetText(
			FText::Format(
				NSLOCTEXT(
					"TeammateStatus",
					"ExperienceFormat",
					"EXP {0}/{1}"),
				FText::AsNumber(
					FMath::RoundToInt(
						StatusData.CurrentExperience)),
				FText::AsNumber(
					FMath::RoundToInt(
						StatusData.RequiredExperience))));
	}
	
	if (DeadOverlay)
	{
		DeadOverlay->SetVisibility(
			StatusData.bIsDead
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}

float UNSTeammateStatusEntryWidget::GetSafePercent(float CurrentValue, float MaxValue) const
{
	if (MaxValue <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(
		CurrentValue/MaxValue,
		0.0f,
		1.0f);
}
