// Copyright 2026 One Team. All rights reserved.

#include "NSSkillSlotWidget.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NeoSanctum/Type/NSSkillCooldownTypes.h"
#include "NeoSanctum/Data/UI/NSSkillUIData.h"
#include "NeoSanctum/Data/UI/NSCharacterSkillUISet.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"


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

void UNSSkillSlotWidget::SetInputKeyText(const FText& NewInputText)
{
	if (InputKeyText)
	{
		InputKeyText->SetText(NewInputText);
		InputKeyText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	
	if (InputKeyIcon)
	{
		InputKeyIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSSkillSlotWidget::SetInputKeyIcon(UTexture2D* NewInputIcon)
{
	if (InputKeyIcon && NewInputIcon)
	{
		InputKeyIcon->SetBrushFromTexture(NewInputIcon);
		InputKeyIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (InputKeyText)
	{
		InputKeyText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSSkillSlotWidget::SetInputDisplayData(const FNSInputDisplayData& NewInputDisplayData)
{	
	if (!NewInputDisplayData.bShowInputDisplay)
	{
		if (InputKeyText)
		{
			InputKeyText->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (InputKeyIcon)
		{
			InputKeyIcon->SetVisibility(ESlateVisibility::Collapsed);
		}

		return;
	}

	if (NewInputDisplayData.bUseInputIcon)
	{
		if (InputKeyText)
		{
			InputKeyText->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (!InputKeyIcon)
		{
			return;
		}

		UTexture2D* LoadedIcon =
			NewInputDisplayData.InputIcon.Get();

		if (!LoadedIcon)
		{
			InputKeyIcon->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}

		InputKeyIcon->SetBrushFromTexture(LoadedIcon);
		InputKeyIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		return;
	}

	if (InputKeyIcon)
	{
		InputKeyIcon->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (InputKeyText)
	{
		InputKeyText->SetText(NewInputDisplayData.InputText);
		InputKeyText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UNSSkillSlotWidget::SetSkillUIData(
	FDataTableRowHandle NewSkillUIDataRow,
	const FNSInputDisplayData& NewInputDisplayData)
{
	//캐릭터 변경 시 DT_SkillUI의 다른 Row로 교체해 아이콘과 스킬 태그를 갱신한다.
	SkillUIDataRow = NewSkillUIDataRow;

	ApplySkillUIData();
	SetInputDisplayData(NewInputDisplayData);

	//캐릭터 변경 직후 이전 스킬의 쿨타임 표시가 남지 않도록 초기화한다.
	CooldownDuration = 0.0f;
	RemainingCooldown = 0.0f;
	bCooldownTickActive = false;
	
	UpdateSkillCooldownFromASC();
}

void UNSSkillSlotWidget::HandleCooldownMessage(
	FGameplayTag Channel,
	const FNSSkillCooldownMessage& Message)
{
	if (Message.SkillSlotTag.IsValid() && Message.SkillSlotTag != SkillSlotTag)
	{
		return;
	}

	ApplySkillCooldownUIData(Message.CooldownData);
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
	

	if (!SkillIcon)
	{
		return;
	}

	if (SkillUIData->SkillIcon.IsNull())
	{
		return;
	}

	UTexture2D* LoadedSkillIcon = SkillUIData->SkillIcon.Get();
	if (!LoadedSkillIcon)
	{
		return;
	}

	SkillIcon->SetBrushFromTexture(LoadedSkillIcon, true);
	SkillIcon->InvalidateLayoutAndVolatility();
	
}

void UNSSkillSlotWidget::CacheOwnerASC()
{
	//스킬 쿨타임과 대쉬 횟수는 PlayerState의 ASC/AttributeSet에 있는 OwningPlayer에서 PlayerState에서 찾음
	APlayerController* PlayerController = GetOwningPlayer();
	if (!IsValid(PlayerController))
	{
		return;
	}
	ANSPlayerState* NSPlayerState = PlayerController->GetPlayerState<ANSPlayerState>();
	if (!IsValid(NSPlayerState))
	{
		return;
	}
	CachedASC = Cast<UNSAbilitySystemComponent>(
		NSPlayerState->GetAbilitySystemComponent());
}

void UNSSkillSlotWidget::UpdateCooldownDisplay(float NewRemainingCooldown, float NewCooldownDuration)
{
	//음수 값이 UI에 들어오지 못하게 보정
	RemainingCooldown = FMath::Max(NewRemainingCooldown, 0.0f);
	CooldownDuration = FMath::Max(NewCooldownDuration, 0.0f);
	
	//남은 시간이나 전체시간이 없으면 쿨타임이 끝난 상태로 처리
	if (RemainingCooldown <= 0.0f || CooldownDuration <= 0.0f)
	{
		ResetCooldown();
		return;
	}
	//머티리얼은 1.0에서 시작해 0.0으로 줄어드는걸 사용
	const float CooldownRatio = RemainingCooldown/ CooldownDuration;
	
	if (CooldownMID)
	{
		CooldownMID->SetScalarParameterValue(
			TEXT("CooldownRatio"),
			CooldownRatio);
	}
	if (CooldownOverlay)
	{
		CooldownOverlay->SetVisibility(ESlateVisibility::Visible);
	}
	if (CooldownText)
	{
		//숫자를 소수점 두번째 자리까지 사용
		CooldownText->SetText(
		FText::FromString(
			FString::Printf(
				TEXT("%.2f"),
				RemainingCooldown)));
		CooldownText->SetVisibility(ESlateVisibility::Visible);
	}
}

void UNSSkillSlotWidget::UpdateChargeDisplay(int32 CurrentCharge, int32 MaxCharge)
{
	if (!ChargeText)
	{
		return;
	}
	//최대 충전수가 1이하인 스킬은 충전택스트를 숨김
	if (MaxCharge <= 1)
	{
		ChargeText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	//GMS로 충전 수를 받은 경우 현재/최대 충전 수를 함께 표시
	ChargeText->SetText(FText::Format(
	NSLOCTEXT("SkillSlotWidget", "ChargeFormat", "{0}/{1}"),
	FText::AsNumber(CurrentCharge),
	FText::AsNumber(MaxCharge)
	));
	
	ChargeText->SetVisibility(ESlateVisibility::Visible);
}

void UNSSkillSlotWidget::UpdateSkillCooldownFromASC()
{
	if (!IsValid(CachedASC))
	{
		CacheOwnerASC();
	}

	if (!IsValid(CachedASC) || !SkillSlotTag.IsValid())
	{
		ResetCooldown();
		UpdateChargeDisplay(0, 0);
		return;
	}

	FSkillCooldownUIData CooldownData;
	if (!CachedASC->GetSkillCooldownUIData(SkillSlotTag, CooldownData))
	{
		ResetCooldown();
		UpdateChargeDisplay(0, 0);
		return;
	}

	ApplySkillCooldownUIData(CooldownData);
}

void UNSSkillSlotWidget::ApplySkillCooldownUIData(
	const FSkillCooldownUIData& CooldownData)
{
	//ASC가 계산한 현재 충전 수를 UI에 반영한다.
	UpdateChargeDisplay(
		CooldownData.CurrentCount,
		CooldownData.MaxCount);

	if (!CooldownData.bIsRecharging)
	{
		ResetCooldown();
		//재충전 중이 아니면 Tick에서 ASC 쿨타임 상태를 조회하지 않는다.
		bCooldownTickActive = false;
		return;
	}
	//재충전 중일 때만 남은 시간과 전체 시간을 화면에 반영한다.
	UpdateCooldownDisplay(
		CooldownData.RemainingTime,
		CooldownData.TotalTime);
	//GMS로 쿨타임 시작을 받은 뒤에는 Tick에서 ASC의 최신 남은 시간을 다시 조회한다.
	bCooldownTickActive = true;
}

void UNSSkillSlotWidget::ApplyInputDisplayVisibility()
{
	if (bShowInputDisplay)
	{
		return;
	}

	if (InputKeyText)
	{
		InputKeyText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (InputKeyIcon)
	{
		InputKeyIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSSkillSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	ApplySkillUIData();
	CacheOwnerASC();
	ApplyInputDisplayVisibility();

	if (CooldownOverlay)
	{
		CooldownMID = CooldownOverlay->GetDynamicMaterial();
	}

	ResetCooldown();
	UpdateChargeDisplay(0, 0);

	UGameplayMessageSubsystem& MessageSubsystem =
		UGameplayMessageSubsystem::Get(this);
	
	CooldownListenerHandle = 
		MessageSubsystem.RegisterListener<FNSSkillCooldownMessage>(
			NSGameplayTags::Message_UI_SkillCooldown_Changed,
			this,
			&ThisClass::HandleCooldownMessage);
	
	UpdateSkillCooldownFromASC();
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

	if (!bCooldownTickActive)
	{
		return;
	}

	RemainingCooldown = FMath::Max(
		RemainingCooldown - InDeltaTime,
		0.0f);

	UpdateCooldownDisplay(
		RemainingCooldown,
		CooldownDuration);

	if (RemainingCooldown <= 0.0f)
	{
		ResetCooldown();
		bCooldownTickActive = false;
	}
}