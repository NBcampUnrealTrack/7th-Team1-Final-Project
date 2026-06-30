// Copyright 2026 One Team. All rights reserved.


#include "NSHitTakenFeedbackWidget.h"

#include "Components/Image.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"

void UNSHitTakenFeedbackWidget::HandleHitTakenFeedback(const FNSHitTakenFeedbackContext& Context)
{
	// 유효한 피격 타입만 화면 연출로 연결
	if (Context.FeedbackType == ENSHitTakenFeedbackType::None)
	{
		return;
	}

	PlayHitTakenFeedback(Context);
}

void UNSHitTakenFeedbackWidget::PlayHitTakenFeedback(const FNSHitTakenFeedbackContext& Context)
{
	// 피격 타입에 맞는 이미지와 애니메이션을 찾아 재생
	UImage* FeedbackImage = GetHitTakenFeedbackImage(Context.FeedbackType);
	UWidgetAnimation* FeedbackAnimation = GetHitTakenFeedbackAnimation(Context.FeedbackType);

	UpdateLowHealthVignette(Context.HealthRatio, Context.ShieldRatio);

	if (!FeedbackImage && !FeedbackAnimation)
	{
		return;
	}

	HideHitTakenFeedbackImages();
	UpdateLowHealthVignette(Context.HealthRatio, Context.ShieldRatio);

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

void UNSHitTakenFeedbackWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 기본 상태에서는 순간 피격 이미지를 숨기고 상태성 연출을 초기화
	HideHitTakenFeedbackImages();
	UpdateLowHealthVignette(1.0f, 1.0f);
	SetShieldRechargingFeedbackVisible(false);
	
	// GMS 피격 피드백 바인딩
	HitTakenFeedbackListenerHandle =
		UGameplayMessageSubsystem::Get(this).RegisterListener<FNSHitTakenFeedbackMessage>(
			NSGameplayTags::Message_UI_HitTakenFeedback,
			this,
			&ThisClass::HandleHitTakenFeedbackMessage);
	
	// GMS 피격 상태성 피드백 바인딩
	HitTakenFeedbackStateListenerHandle =
		UGameplayMessageSubsystem::Get(this).RegisterListener<FNSHitTakenFeedbackStateMessage>(
			NSGameplayTags::Message_UI_HitTakenFeedback_State,
			this,
			&ThisClass::HandleHitTakenFeedbackStateMessage);
	
	// GMS에서 바이탈(Health와 Shield) 변화 바인딩
	HitTakenFeedbackVitalsListenerHandle =
		UGameplayMessageSubsystem::Get(this).RegisterListener<FNSHitTakenFeedbackVitalsMessage>(
			NSGameplayTags::Message_UI_HitTakenFeedback_Vitals,
			this,
			&ThisClass::HandleHitTakenFeedbackVitalsMessage);
}

void UNSHitTakenFeedbackWidget::NativeDestruct()
{
	// 제거된 위젯으로 콜백이 들어오지 않도록 리스너를 정리
	HitTakenFeedbackListenerHandle.Unregister();
	HitTakenFeedbackStateListenerHandle.Unregister();
	HitTakenFeedbackVitalsListenerHandle.Unregister();

	Super::NativeDestruct();
}

void UNSHitTakenFeedbackWidget::HandleHitTakenFeedbackMessage(
	FGameplayTag Channel,
	const FNSHitTakenFeedbackMessage& Message)
{
	// GMS 메시지를 위젯 연출 함수로 전달
	HandleHitTakenFeedback(Message.Context);
}

void UNSHitTakenFeedbackWidget::HandleHitTakenFeedbackStateMessage(
	FGameplayTag Channel,
	const FNSHitTakenFeedbackStateMessage& Message)
{
	// 상태성 피드백 메시지를 위젯 표시 상태로 반영
	SetHitTakenFeedbackStateVisible(Message.StateType, Message.bActive);
}

void UNSHitTakenFeedbackWidget::HandleHitTakenFeedbackVitalsMessage(
	FGameplayTag Channel,
	const FNSHitTakenFeedbackVitalsMessage& Message)
{
	UpdateLowHealthVignette(Message.HealthRatio, Message.ShieldRatio);
}

void UNSHitTakenFeedbackWidget::HideHitTakenFeedbackImages() const
{
	// 순간 피격 이미지만 숨기고 상태성 비네트/재충전 이미지는 유지
	const TArray<UImage*> FeedbackImages =
	{
		ShieldHitFeedbackImage,
		HealthHitFeedbackImage
	};

	for (UImage* FeedbackImage : FeedbackImages)
	{
		if (FeedbackImage)
		{
			FeedbackImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UNSHitTakenFeedbackWidget::UpdateLowHealthVignette(
	const float HealthRatio,
	const float ShieldRatio)
{
	// 낮은 체력 구간에서만 비네트를 유지
	if (!LowHealthVignetteImage)
	{
		return;
	}

	if (HealthRatio > LowHealthThreshold || ShieldRatio > LowShieldThresholdForLowHealth)
	{
		LowHealthVignetteImage->SetVisibility(ESlateVisibility::Collapsed);
		if (bLowHealthVignetteActive && LowHealthVignetteAnimation)
		{
			StopAnimation(LowHealthVignetteAnimation);
		}
		bLowHealthVignetteActive = false;
		return;
	}

	FLinearColor VignetteColor = FLinearColor::Black;
	VignetteColor.A = LowHealthVignetteOpacity;
	LowHealthVignetteImage->SetColorAndOpacity(VignetteColor);
	LowHealthVignetteImage->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (!bLowHealthVignetteActive && LowHealthVignetteAnimation)
	{
		PlayAnimation(LowHealthVignetteAnimation, 0.0f, 0);
	}
	bLowHealthVignetteActive = true;
}

void UNSHitTakenFeedbackWidget::SetHitTakenFeedbackStateVisible(
	const ENSHitTakenFeedbackStateType StateType,
	const bool bVisible)
{
	// 상태성 피드백 종류에 맞는 UI를 켜거나 끔
	switch (StateType)
	{
	case ENSHitTakenFeedbackStateType::ShieldRecharging:
		SetShieldRechargingFeedbackVisible(bVisible);
		break;
	case ENSHitTakenFeedbackStateType::None:
	default:
		break;
	}
}

void UNSHitTakenFeedbackWidget::SetShieldRechargingFeedbackVisible(const bool bVisible)
{
	// Shield 재충전 상태에 따라 이미지와 반복 애니메이션을 제어
	if (ShieldRechargingFeedbackImage)
	{
		ShieldRechargingFeedbackImage->SetVisibility(
			bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (!ShieldRechargingFeedbackAnimation)
	{
		bShieldRechargingFeedbackActive = bVisible;
		return;
	}

	if (bVisible)
	{
		if (!bShieldRechargingFeedbackActive)
		{
			PlayAnimation(ShieldRechargingFeedbackAnimation, 0.0f, 0);
		}
	}
	else
	{
		if (bShieldRechargingFeedbackActive)
		{
			StopAnimation(ShieldRechargingFeedbackAnimation);
		}
	}

	bShieldRechargingFeedbackActive = bVisible;
}

UImage* UNSHitTakenFeedbackWidget::GetHitTakenFeedbackImage(
	const ENSHitTakenFeedbackType FeedbackType) const
{
	// UI에서 처리하는 피격 타입만 이미지 반환
	switch (FeedbackType)
	{
	case ENSHitTakenFeedbackType::ShieldHit:
		return ShieldHitFeedbackImage;
	case ENSHitTakenFeedbackType::HealthHit:
		return HealthHitFeedbackImage;
	case ENSHitTakenFeedbackType::ShieldBroken:
	case ENSHitTakenFeedbackType::None:
	default:
		return nullptr;
	}
}

UWidgetAnimation* UNSHitTakenFeedbackWidget::GetHitTakenFeedbackAnimation(
	const ENSHitTakenFeedbackType FeedbackType) const
{
	// UI에서 처리하는 피격 타입만 애니메이션 반환
	switch (FeedbackType)
	{
	case ENSHitTakenFeedbackType::ShieldHit:
		return ShieldHitFeedbackAnimation;
	case ENSHitTakenFeedbackType::HealthHit:
		return HealthHitFeedbackAnimation;
	case ENSHitTakenFeedbackType::ShieldBroken:
	case ENSHitTakenFeedbackType::None:
	default:
		return nullptr;
	}
}
