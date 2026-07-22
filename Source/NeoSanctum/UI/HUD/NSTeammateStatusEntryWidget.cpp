// Copyright 2026 One Team. All rights reserved.


#include "NSTeammateStatusEntryWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UNSTeammateStatusEntryWidget::ApplyStatusData(const FNSPlayerStatusViewData& StatusData)
{
	PlayerId = StatusData.PlayerId;
	
	if (PlayerNameText)
	{
		PlayerNameText->SetText(FText::FromString(StatusData.PlayerName));
	}
	
	if (PortraitImage)
	{
		UTexture2D* PortraitTexture =
			StatusData.PortraitTexture.Get();

		if (!PortraitTexture &&
			!StatusData.PortraitTexture.IsNull())
		{
			PortraitTexture =
				StatusData.PortraitTexture.LoadSynchronous();
		}

		if (PortraitTexture)
		{
			PortraitImage->SetBrushFromTexture(
				PortraitTexture,
				false);

			PortraitImage->SetVisibility(
				ESlateVisibility::HitTestInvisible);
		}
		else
		{
			PortraitImage->SetVisibility(
				ESlateVisibility::Hidden);
		}
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
