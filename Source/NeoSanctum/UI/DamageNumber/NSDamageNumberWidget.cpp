// Copyright 2026 One Team. All rights reserved.


#include "NSDamageNumberWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/TextBlock.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"

namespace
{
	const FVector2D DamageNumberPopupDirections[] =
	{
		FVector2D(-0.95f, -0.55f).GetSafeNormal(),
		FVector2D(-0.65f, -0.80f).GetSafeNormal(),
		FVector2D(-0.30f, -0.95f).GetSafeNormal(),
		FVector2D(0.30f, -0.95f).GetSafeNormal(),
		FVector2D(0.65f, -0.80f).GetSafeNormal(),
		FVector2D(0.95f, -0.55f).GetSafeNormal()
	};

	const FName CriticalIconColorParameter(TEXT("FontColor"));

	constexpr float PopupMotionDuration = 0.8f;
	constexpr float PopupRiseDurationRatio = 0.65f;
	constexpr float PopupRiseDistance = 44.0f;
	constexpr float PopupSettleDistance = 14.0f;
}

void UNSDamageNumberWidget::SetDamageNumber(
	const FNSDamageNumberFeedbackContext& Context, const FVector2D& DisplayOffset)
{
	DisplayDamage = FMath::RoundToInt(Context.DamageAmount);
	bCritical = Context.bIsCritical;

	if (DamageText)
	{
		// 위로 올라가는 애니메이션과 겹치지 않게 시작 위치는 Canvas Slot에서 나눔.
		if (UCanvasPanelSlot* DamageTextSlot = Cast<UCanvasPanelSlot>(DamageText->Slot))
		{
			DamageTextSlot->SetPosition(DisplayOffset);
		}

		const FLinearColor DisplayColor = ResolveDamageNumberColor(Context);
		const bool bShowCriticalIcon = ShouldShowCriticalIcon(Context);

		DamageText->SetText(FText::AsNumber(DisplayDamage));
		DamageText->SetColorAndOpacity(DisplayColor);
		if (bCritical)
		{
			// 크리티컬은 WBP 기본 폰트를 유지하되 외곽선은 뺌.
			FSlateFontInfo CriticalFont = DamageText->GetFont();
			CriticalFont.OutlineSettings.OutlineSize = 0;
			DamageText->SetFont(CriticalFont);
		}

		if (CriticalIcon)
		{
			CriticalIcon->SetVisibility(bShowCriticalIcon
				? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

			if (bShowCriticalIcon)
			{
				// 아이콘 Material도 숫자와 같은 색을 받음.
				if (UMaterialInstanceDynamic* IconMaterial = CriticalIcon->GetDynamicMaterial())
				{
					IconMaterial->SetVectorParameterValue(CriticalIconColorParameter, DisplayColor);
				}
			}
		}

		StartPopupMotion();
		DamageText->SetRenderScale(bCritical ? CriticalRenderScale : NormalRenderScale);
	}

	// WBP에 애니메이션이 있으면 숫자가 뜨는 연출만 재생.
	if (PopupAnimation)
	{
		PlayAnimation(PopupAnimation);
	}
}

void UNSDamageNumberWidget::StartPopupMotion()
{
	if (!DamageText)
	{
		return;
	}

	// 숫자마다 위쪽 여섯 방향 중 하나를 골라 움직임.
	const int32 DirectionIndex = FMath::RandRange(0, UE_ARRAY_COUNT(DamageNumberPopupDirections) - 1);

	PopupMotionDirection = DamageNumberPopupDirections[DirectionIndex];
	PopupMotionElapsedTime = 0.0f;
	bPopupMotionActive = true;
	DamageText->SetRenderTranslation(FVector2D::ZeroVector);
}

FLinearColor UNSDamageNumberWidget::ResolveDamageNumberColor(const FNSDamageNumberFeedbackContext& Context) const
{
	if (Context.TargetType == ENSHitFeedbackTargetType::DestructibleObject)
	{
		return Context.bIsCritical ? DestructibleCriticalDamageColor : DestructibleNormalDamageColor;
	}

	return Context.bIsCritical ? CriticalDamageColor : NormalDamageColor;
}

bool UNSDamageNumberWidget::ShouldShowCriticalIcon(const FNSDamageNumberFeedbackContext& Context) const
{
	return Context.bIsCritical &&
		(Context.TargetType == ENSHitFeedbackTargetType::Enemy ||
			Context.TargetType == ENSHitFeedbackTargetType::DestructibleObject);
}

void UNSDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bPopupMotionActive || !DamageText)
	{
		return;
	}

	PopupMotionElapsedTime += InDeltaTime;

	const float MotionAlpha = FMath::Clamp(PopupMotionElapsedTime / PopupMotionDuration, 0.0f, 1.0f);

	float TravelDistance = 0.0f;
	if (MotionAlpha <= PopupRiseDurationRatio)
	{
		const float RiseAlpha = MotionAlpha / PopupRiseDurationRatio;
		TravelDistance = FMath::InterpEaseOut(0.0f, PopupRiseDistance, RiseAlpha, 2.0f);
	}
	else
	{
		const float SettleAlpha = (MotionAlpha - PopupRiseDurationRatio) / (1.0f - PopupRiseDurationRatio);

		// 꼭지점에서 출발 지점으로 조금 내려앉음.
		TravelDistance = FMath::InterpEaseOut(
			PopupRiseDistance,
			PopupRiseDistance - PopupSettleDistance,
			SettleAlpha,
			2.0f
		);
	}

	DamageText->SetRenderTranslation(PopupMotionDirection * TravelDistance);

	if (MotionAlpha >= 1.0f)
	{
		bPopupMotionActive = false;
	}
}
