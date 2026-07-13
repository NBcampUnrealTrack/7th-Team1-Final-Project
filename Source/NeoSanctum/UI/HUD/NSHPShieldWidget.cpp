// Copyright 2026 One Team. All rights reserved.


#include "NSHPShieldWidget.h"

#include "IMediaCache.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Widget.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"


void UNSHPShieldWidget::SetHealth(float CurrentHealth, float MaxHealth)
{
	RefreshPortrait();

	//체력 비율 계산
	
	float HealthPercent = GetSafePercent(CurrentHealth, MaxHealth);
	
	if (HealthBar)
	{
		HealthBar->SetPercent(HealthPercent);
	}
	//현재 체력 / 최대 체력
	if (HealthText)
	{
		HealthText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f/%.0f"),CurrentHealth,MaxHealth)
			));
	}
}

void UNSHPShieldWidget::SetShield(float CurrentShield, float MaxShield)
{
	//쉴드 비율 계산
	float ShieldPercent = GetSafePercent(CurrentShield, MaxShield);
	if (ShieldBar)
	{
		ShieldBar->SetPercent(ShieldPercent);
	}
	//현재 쉴드 / 최대 쉴드
	if (ShieldText)
	{
		ShieldText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f/%.0f"),CurrentShield,MaxShield)
				));
	}
}

void UNSHPShieldWidget::SetHealthTextVisible(bool bVisible)
{
	if (HealthText)
	{
		HealthText->SetVisibility(
			bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UNSHPShieldWidget::SetShieldTextVisible(bool bVisible)
{
	if (ShieldText)
	{
		ShieldText->SetVisibility(
			bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UNSHPShieldWidget::ResetHealthAndShield()
{
	//TODO(영웅):플레이어 사망 또는 런 재시작시 체력 실드 초기화
	SetHealth(0.0f, 0.0f);
	SetShield(0.0f, 0.0f);
	SetExperience(0.0f, 0.0f);
	SetReloading(false);
	SetAmmo(0, 0);
}

void UNSHPShieldWidget::SetExperience(float CurrentExperience, float RequiredExperience)
{
	if (PlayerExperienceBar)
	{
		PlayerExperienceBar->SetPercent(
			GetSafePercent(
				CurrentExperience,
				RequiredExperience));
	}
	
	if (PlayerExperienceValueText)
	{
		PlayerExperienceValueText->SetText(
			FText::Format(
				NSLOCTEXT(
					"PlayerStatus",
					"ExperienceFormat",
					"EXP {0}/{1}"),
				FText::AsNumber(
					FMath::RoundToInt(
						CurrentExperience)),
				FText::AsNumber(
					FMath::RoundToInt(
						RequiredExperience))));
	}
}

void UNSHPShieldWidget::SetAmmo(int32 CurrentAmmo, int32 MaxAmmo)
{
	CachedCurrentAmmo = FMath::Max(CurrentAmmo, 0);
	CachedMaxAmmo = FMath::Max(MaxAmmo, 0);
	
	RefreshAmmoText();
}

void UNSHPShieldWidget::SetReloading(bool bReloading)
{
	bIsReloading = bReloading;
	
	RefreshAmmoText();
}

float UNSHPShieldWidget::GetSafePercent(float CurrentValue, float MaxValue)const
{
	//최대값이 0 이하일때 비율 계산 X
	if (MaxValue <= 0.0f)
	{
		return 0.0f;
	}
	
	return FMath::Clamp(CurrentValue / MaxValue, 0.0f, 1.0f);
}

void UNSHPShieldWidget::RefreshPortrait()
{
	if (!PortraitImage)
	{
		return;
	}

	const APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer)
	{
		PortraitImage->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const ANSPlayerState* NSPlayerState =
		OwningPlayer->GetPlayerState<ANSPlayerState>();

	if (!NSPlayerState)
	{
		PortraitImage->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const UNSCharacterData* CharacterData =
		NSPlayerState->GetCurrentCharacterData();

	if (!CharacterData || CharacterData->HUDPortraitTexture.IsNull())
	{
		PortraitImage->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	UTexture2D* PortraitTexture = CharacterData->HUDPortraitTexture.Get();

	if (!PortraitTexture)
	{
		PortraitTexture = CharacterData->HUDPortraitTexture.LoadSynchronous();
	}

	if (!PortraitTexture)
	{
		PortraitImage->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	
	if (CachedPortraitCharacterData == CharacterData)
	{
		return;
	}
	
	CachedPortraitCharacterData = CharacterData;

	PortraitImage->SetBrushFromTexture(PortraitTexture);
	PortraitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UNSHPShieldWidget::RefreshAmmoText()
{
	const bool bUsesAmmo =
		CachedMaxAmmo > 0;

	if (AmmoRow)
	{
		AmmoRow->SetVisibility(
			bUsesAmmo
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (!AmmoText)
	{
		return;
	}

	if (!bUsesAmmo)
	{
		AmmoText->SetText(FText::GetEmpty());
		return;
	}

	if (bIsReloading)
	{
		AmmoText->SetText(
			NSLOCTEXT(
				"PlayerStatus",
				"ReloadingText",
				"Reload"));

		return;
	}

	AmmoText->SetText(
		FText::Format(
			NSLOCTEXT(
				"PlayerStatus",
				"AmmoFormat",
				"{0} / {1}"),
			FText::AsNumber(CachedCurrentAmmo),
			FText::AsNumber(CachedMaxAmmo)));
}

void UNSHPShieldWidget::NativeConstruct()
{
	Super::NativeConstruct();
	//실제 값이 들어오기 전 기본상태
	ResetHealthAndShield();
	RefreshPortrait();
}
