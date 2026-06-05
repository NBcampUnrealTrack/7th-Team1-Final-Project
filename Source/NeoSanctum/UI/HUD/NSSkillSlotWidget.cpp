// Copyright 2026 One Team. All rights reserved.

#include "NSSkillSlotWidget.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NSSkillCooldownMessage.h"
#include "NeoSanctum/Data/UI/NSSkillUIData.h"

void UNSSkillSlotWidget::StartCooldown(float NewCooldownDuration)
{
	//음수 시간 방지용 코드
	CooldownDuration = FMath::Max(NewCooldownDuration, 0.0f);
	RemainingCooldown = CooldownDuration;
	
	if (CooldownDuration <= 0.0f)
	{
		ResetCooldown();
		return;
	}
	//쿨타임 시작시 쿨타임 초기화상태로 시작
	if (CooldownMID)
	{
		CooldownMID->SetScalarParameterValue(
			TEXT("CooldownRatio"),
			1.0f);
	}
	
	if (CooldownOverlay)
	{
		CooldownOverlay->SetVisibility(ESlateVisibility::Visible);
	}
	
	if (CooldownText)
	{
		CooldownText->SetText(
			FText::FromString(
				FString::Printf(
					TEXT("%.2f"),
					RemainingCooldown)));

		CooldownText->SetVisibility(ESlateVisibility::Visible);
	}
}

void UNSSkillSlotWidget::ResetCooldown()
{
	CooldownDuration = 0.0f;
	RemainingCooldown = 0.0f;
	//쿨타임이 아닐땐 쿨타임 UI를 숨김
	if (CooldownMID)
	{
		CooldownMID->SetScalarParameterValue(
			TEXT("CooldownRatio"),
			0.0f);
	}
	if (CooldownOverlay)
	{
		CooldownOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	if (CooldownText)
	{
		CooldownText->SetVisibility(ESlateVisibility::Collapsed);
	}
}
void UNSSkillSlotWidget::HandleCooldownMessage(
	FGameplayTag Channel,
	const FNSSkillCooldownMessage& Message)
{
	if (BoundSkillTag.IsValid() &&
		Message.SkillTag.IsValid() &&
		!Message.SkillTag.MatchesTagExact(BoundSkillTag))
	{
		return;
	}

	CurrentCharge = Message.CurrentCharge;
	MaxCharge = Message.MaxCharge;

	UpdateChargeText(CurrentCharge, MaxCharge);

	if (MaxCharge <= 0 || Message.CooldownDuration <= 0.0f)
	{
		PendingCooldownCount = 0;
		ResetCooldown();
		return;
	}

	PendingCooldownCount =
		FMath::Max(MaxCharge - CurrentCharge, 0);

	CooldownDuration = Message.CooldownDuration;

	if (PendingCooldownCount <= 0)
	{
		ResetCooldown();
		return;
	}

	if (RemainingCooldown <= 0.0f)
	{
		StartCooldown(CooldownDuration);
	}
}

void UNSSkillSlotWidget::ApplySkillUIData()
{
	const FNSSkillUIData* SkillUIData =
		SkillUIDataRow.GetRow<FNSSkillUIData>(
			TEXT("ApplySkillUIData"));
	if (!SkillUIData)
	{
		return;
	}
	BoundSkillTag = SkillUIData->SkillTag;
	if (SkillIcon)
	{
		UTexture2D* LoadedIcon =
			SkillUIData->SkillIcon.LoadSynchronous();
		if (LoadedIcon)
		{
			SkillIcon->SetBrushFromTexture(LoadedIcon);	
		}
	}
}

void UNSSkillSlotWidget::UpdateChargeText(
	int32 NewCurrentCharge,
	int32 NewMaxCharge)
{
	if (!ChargeText)
	{
		return;
	}

	if (NewMaxCharge <= 0)
	{
		ChargeText->SetVisibility(ESlateVisibility::Visible);
		return;
	}

	ChargeText->SetVisibility(ESlateVisibility::Visible);

	ChargeText->SetText(
		FText::FromString(
			FString::Printf(
				TEXT("%d"),
				NewCurrentCharge)));
}

void UNSSkillSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	ApplySkillUIData();
	
	// WBP의 CooldownOverlay Brush에 적용된 머티리얼을 동적 인스턴스로 가져온다
	if (CooldownOverlay)
	{
		CooldownMID = CooldownOverlay->GetDynamicMaterial();
	}
	ResetCooldown();
	if (ChargeText)
	{
		ChargeText->SetVisibility(ESlateVisibility::Collapsed);
	}
	// GMS 채널을 구독하여 스킬 쿨타임 시작 메시지를 받는다
	UGameplayMessageSubsystem& MessageSubsystem =
		UGameplayMessageSubsystem::Get(this);
	
	CooldownListenerHandle = 
		MessageSubsystem.RegisterListener<FNSSkillCooldownMessage>(
			TAG_Message_UI_SkillCooldown_Start,
			this,
			&ThisClass::HandleCooldownMessage);
}

void UNSSkillSlotWidget::NativeDestruct()
{
	// 제거된 위젯에 콜백이 호출되지 않도록 리스너를 정리한다
	CooldownListenerHandle.Unregister();
	
	Super::NativeDestruct();
}


void UNSSkillSlotWidget::NativeTick(
	const FGeometry& InGeometry,
	float InDeltaTime)
{
	Super::NativeTick(InGeometry, InDeltaTime);

	if (RemainingCooldown <= 0.0f)
	{
		return;
	}

	RemainingCooldown = FMath::Max(
		RemainingCooldown - InDeltaTime,
		0.0f);

	const float CooldownRatio =
		CooldownDuration > 0.0f
			? RemainingCooldown / CooldownDuration
			: 0.0f;

	if (CooldownMID)
	{
		CooldownMID->SetScalarParameterValue(
			TEXT("CooldownRatio"),
			CooldownRatio);
	}

	if (CooldownText)
	{
		CooldownText->SetText(
			FText::FromString(
				FString::Printf(
					TEXT("%.2f"),
					RemainingCooldown)));
	}

	if (RemainingCooldown <= 0.0f)
	{
		if (PendingCooldownCount > 0)
		{
			--PendingCooldownCount;

			if (CurrentCharge < MaxCharge)
			{
				++CurrentCharge;
				UpdateChargeText(CurrentCharge, MaxCharge);
			}

			if (PendingCooldownCount > 0)
			{
				StartCooldown(CooldownDuration);
				return;
			}
		}

		ResetCooldown();
	}
}