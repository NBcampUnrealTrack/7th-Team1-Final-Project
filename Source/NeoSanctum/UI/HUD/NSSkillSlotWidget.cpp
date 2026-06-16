// Copyright 2026 One Team. All rights reserved.

#include "NSSkillSlotWidget.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NSSkillCooldownMessage.h"
#include "NeoSanctum/Data/UI/NSSkillUIData.h"
#include "AbilitySystemComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"

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
	//메시지에 담긴 스킬 태그와 쿨타임을 하나의 컨테이너로 모아 슬롯 태그 쿼리와 비교
	FGameplayTagContainer MessageTags;

	if (Message.SkillTag.IsValid())
	{
		MessageTags.AddTag(Message.SkillTag);
	}

	if (Message.CooldownTag.IsValid())
	{
		MessageTags.AddTag(Message.CooldownTag);
	}
	
	//이 슬롯이 반응해야하는 태그가 아니면 다른 스킬의 쿨타임 메시지이므로 무시
	if (!CooldownTagQuery.IsEmpty() && !CooldownTagQuery.Matches(MessageTags))
	{
		return;
	}
	
	//충전형 스킬은 메시지에 포함된 충전수를 갱신
	UpdateChargeDisplay(
		Message.CurrentCharge,
		Message.MaxCharge);
	// ameplay 쪽에서 전달한 실제 쿨타임 시간으로 UI 카운트다운을 시작한다.
	StartCooldown(Message.CooldownDuration);
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
	//매 프레임 PlayerState를 다시 찾지 않도록 ASC를 캐싱
	CachedASC = NSPlayerState->GetAbilitySystemComponent();
}

void UNSSkillSlotWidget::UpdateCooldownFromASC()
{
	//위젯 생성 시점에 playerState가 아직 준비가 필요할때만 캐싱
	if (!IsValid(CachedASC))
	{
		CacheOwnerASC();
	}
	//ASC가 없거나 쿨타임 태그 조건이 없으면 쿨타임을 표시하지않음
	if (!IsValid(CachedASC) || CooldownTagQuery.IsEmpty())
	{
		ResetCooldown();
		return;
	}
	
	//슬롯에 설정된 태그 쿼리와 일치하는 Active GameplayEffect를 쿨타임으로 조회
	FGameplayEffectQuery CooldownQuery;
	CooldownQuery.OwningTagQuery = CooldownTagQuery;

	const TArray<TPair<float, float>> CooldownTimes =
		CachedASC->GetActiveEffectsTimeRemainingAndDuration(CooldownQuery);
	//일치하는 쿨타임 GE가 없으면 쿨타임 UI를 숨김
	if (CooldownTimes.IsEmpty())
	{
		ResetCooldown();
		return;
	}

	float BestRemainingTime = 0.0f;
	float BestDuration = 0.0f;

	//가장 오래 남은 값을 대표로 사용
	for (const TPair<float, float>& CooldownTime : CooldownTimes)
	{
		const float RemainingTime = CooldownTime.Key;
		const float Duration = CooldownTime.Value;

		if (RemainingTime > BestRemainingTime)
		{
			BestRemainingTime = RemainingTime;
			BestDuration = Duration;
		}
	}

	UpdateCooldownDisplay(BestRemainingTime, BestDuration);
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

void UNSSkillSlotWidget::UpdateDashChargeFromASC()
{
	if (!ChargeText)
	{
		return;
	}

	//대쉬 횟수는 ASC의 PlayerAttributeSet에 있으므로 ASC가 없으면 다시 캐싱
	if (!IsValid(CachedASC))
	{
		CacheOwnerASC();
	}

	if (!IsValid(CachedASC))
	{
		ChargeText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	//대쉬는 별도 쿨타임 GE가 아니라 DashCount Attribute로 사용 가능 횟수를 관리
	const UNSPlayerAttributeSet* PlayerAttributeSet =
		CachedASC->GetSet<UNSPlayerAttributeSet>();

	if (!PlayerAttributeSet)
	{
		ChargeText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const int32 CurrentCharge =
		FMath::Max<int32>(FMath::FloorToInt(PlayerAttributeSet->GetDashCount()), 0);

	ChargeText->SetText(FText::AsNumber(CurrentCharge));
	ChargeText->SetVisibility(ESlateVisibility::Visible);
}
void UNSSkillSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	ApplySkillUIData();
	CacheOwnerASC();
	ResetCooldown();
	UpdateChargeDisplay(0, 0);
	
	// WBP의 CooldownOverlay Brush에 적용된 머티리얼을 동적 인스턴스로 가져온다
	if (CooldownOverlay)
	{
		CooldownMID = CooldownOverlay->GetDynamicMaterial();
	}
	ResetCooldown();
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
	
	UpdateCooldownFromASC();
	
	if (bShowChargeText)
	{
		UpdateDashChargeFromASC();
	}
	else
	{
		UpdateChargeDisplay(0, 0);
	}

}
