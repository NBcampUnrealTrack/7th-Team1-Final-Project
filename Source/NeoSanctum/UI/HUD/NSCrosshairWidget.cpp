// Copyright 2026 One Team. All rights reserved.


#include "NSCrosshairWidget.h"
#include "Components/Image.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"
#include "NeoSanctum/UI/Options/NSUISettingsSubsystem.h"

void UNSCrosshairWidget::ShowCrosshair()
{
	//조준점이 필요할때 다시 표시
	if (CrosshairImage)
	{
		CrosshairImage->SetVisibility(ESlateVisibility::Visible);
	}
}

void UNSCrosshairWidget::HideCrosshair()
{
	//메뉴, 컷신, 사망 상태등 조준점이 필요없는 상황에서 숨김
	if (CrosshairImage)
	{
		CrosshairImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSCrosshairWidget::SetCrosshairColor(FLinearColor Newcolor)
{
	//상태에따라 색상변경
	if (CrosshairImage)
	{
		CrosshairImage->SetColorAndOpacity(Newcolor);
	}
}

void UNSCrosshairWidget::PlayAttackFeedback(const ENSCrosshairAttackFeedbackType FeedbackType)
{
	// 피드백 타입에 맞는 이미지와 애니메이션을 재생
	if (FeedbackType == ENSCrosshairAttackFeedbackType::None)
	{
		return;
	}

	UImage* FeedbackImage = GetAttackFeedbackImage(FeedbackType);
	UWidgetAnimation* FeedbackAnimation = GetAttackFeedbackAnimation(FeedbackType);
	if (!FeedbackImage && !FeedbackAnimation)
	{
		return;
	}

	HideAttackFeedbackImages();

	if (FeedbackImage)
	{
		FeedbackImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (FeedbackAnimation)
	{
		StopAnimation(FeedbackAnimation);
		PlayAnimation(FeedbackAnimation);
	}
}

void UNSCrosshairWidget::HandleAttackFeedbackMessage(
	FGameplayTag Channel,
	const FNSCrosshairAttackFeedbackMessage& Message)
{
	// GMS 메시지를 실제 위젯 피드백 재생으로 연결
	PlayAttackFeedback(Message.FeedbackType);
}

void UNSCrosshairWidget::HideAttackFeedbackImages() const
{
	// 애니메이션 시작 전에 이전 피드백 이미지를 정리
	const TArray<UImage*> FeedbackImages =
	{
		NormalHitFeedbackImage,
		CriticalHitFeedbackImage,
		HeadShotFeedbackImage,
		ShieldBarrierHitFeedbackImage,
		KillFeedbackImage,
		DestroyFeedbackImage
	};

	for (UImage* FeedbackImage : FeedbackImages)
	{
		if (FeedbackImage)
		{
			FeedbackImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

UImage* UNSCrosshairWidget::GetAttackFeedbackImage(const ENSCrosshairAttackFeedbackType FeedbackType) const
{
	// 피드백 타입에 대응하는 Image 위젯 반환
	switch (FeedbackType)
	{
	case ENSCrosshairAttackFeedbackType::CriticalAttack:
		return CriticalHitFeedbackImage;
	case ENSCrosshairAttackFeedbackType::HeadShot:
		return HeadShotFeedbackImage;
	case ENSCrosshairAttackFeedbackType::ShieldBarrierAttack:
		return ShieldBarrierHitFeedbackImage;
	case ENSCrosshairAttackFeedbackType::Kill:
		return KillFeedbackImage;
	case ENSCrosshairAttackFeedbackType::Destroy:
		return DestroyFeedbackImage;
	case ENSCrosshairAttackFeedbackType::NormalAttack:
	default:
		return NormalHitFeedbackImage;
	}
}

UWidgetAnimation* UNSCrosshairWidget::GetAttackFeedbackAnimation(
	const ENSCrosshairAttackFeedbackType FeedbackType) const
{
	// 피드백 타입에 대응하는 Widget Animation 반환
	switch (FeedbackType)
	{
	case ENSCrosshairAttackFeedbackType::CriticalAttack:
		return CriticalHitFeedbackAnimation;
	case ENSCrosshairAttackFeedbackType::HeadShot:
		return HeadShotFeedbackAnimation;
	case ENSCrosshairAttackFeedbackType::ShieldBarrierAttack:
		return ShieldBarrierHitFeedbackAnimation;
	case ENSCrosshairAttackFeedbackType::Kill:
		return KillFeedbackAnimation;
	case ENSCrosshairAttackFeedbackType::Destroy:
		return DestroyFeedbackAnimation;
	case ENSCrosshairAttackFeedbackType::NormalAttack:
	default:
		return NormalHitFeedbackAnimation;
	}
}

void UNSCrosshairWidget::HandleCrosshairColorChanged(const FLinearColor& NewColor)
{
	SetCrosshairColor(NewColor);
}

void UNSCrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();
	//기본 상태에서 조준점 보임
	ShowCrosshair();
	HideAttackFeedbackImages();
	
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		UNSUISettingsSubsystem* Settings =
			GameInstance->GetSubsystem<
				UNSUISettingsSubsystem>();
		
		if (Settings)
		{
			CachedUISettingsSubsystem = Settings;
			
			SetCrosshairColor(
				Settings->GetCrosshairColor());
			
			Settings->OnCrosshairColorChanged.RemoveAll(this);
			Settings->OnCrosshairColorChanged.AddUObject(
				this,
				&ThisClass::HandleCrosshairColorChanged);
		}
	}

	AttackFeedbackListenerHandle =
		UGameplayMessageSubsystem::Get(this).RegisterListener<FNSCrosshairAttackFeedbackMessage>(
			NSGameplayTags::Message_UI_Crosshair_AttackFeedback,
			this,
			&ThisClass::HandleAttackFeedbackMessage);
}

void UNSCrosshairWidget::NativeDestruct()
{
	AttackFeedbackListenerHandle.Unregister();

	if (CachedUISettingsSubsystem.IsValid())
	{
		CachedUISettingsSubsystem->OnCrosshairColorChanged.RemoveAll(this);

		CachedUISettingsSubsystem.Reset();
	}
	
	Super::NativeDestruct();
}
