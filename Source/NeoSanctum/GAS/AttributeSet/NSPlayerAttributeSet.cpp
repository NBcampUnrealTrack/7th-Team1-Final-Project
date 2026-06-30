// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "Net/UnrealNetwork.h"

void UNSPlayerAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, ShieldRechargeRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, ShieldRechargeCooldown, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, DashCount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxDashCount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, DashRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, Ammo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxSkill1Count, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, Skill1Count, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxSkill2Count, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, Skill2Count, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, MaxSkill3Count, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNSPlayerAttributeSet, Skill3Count, COND_None, REPNOTIFY_Always);
}

void UNSPlayerAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxShield());
	}
	else if (Attribute == GetMaxShieldAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetShieldRechargeRateAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetShieldRechargeCooldownAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetDashCountAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxDashCount());
	}
	else if (Attribute == GetMaxDashCountAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetDashRegenRateAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetAmmoAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxAmmo());
	}
	else if (Attribute == GetMaxAmmoAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetMaxSkill1CountAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetSkill1CountAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxSkill1Count());
	}
	else if (Attribute == GetMaxSkill2CountAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetSkill2CountAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxSkill2Count());
	}
	else if (Attribute == GetMaxSkill3CountAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetSkill3CountAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxSkill3Count());
	}
}

void UNSPlayerAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetShieldAttribute())
	{
		SetShield(FMath::Clamp(GetShield(), 0.0f, GetMaxShield()));

		if (GetShield() >= GetMaxShield())
		{
			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				FGameplayTagContainer RechargingTags;
				RechargingTags.AddTag(NSGameplayTags::State_Shield_Recharging);
				ASC->RemoveActiveEffectsWithGrantedTags(RechargingTags);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetMaxShieldAttribute())
	{
		SetMaxShield(FMath::Max(GetMaxShield(), 0.0f));
		SetShield(FMath::Clamp(GetShield(), 0.0f, GetMaxShield()));

		if (GetShield() >= GetMaxShield())
		{
			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				FGameplayTagContainer RechargingTags;
				RechargingTags.AddTag(NSGameplayTags::State_Shield_Recharging);
				ASC->RemoveActiveEffectsWithGrantedTags(RechargingTags);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetShieldRechargeRateAttribute())
	{
		SetShieldRechargeRate(FMath::Max(GetShieldRechargeRate(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetShieldRechargeCooldownAttribute())
	{
		SetShieldRechargeCooldown(FMath::Max(GetShieldRechargeCooldown(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetDashCountAttribute())
	{
		SetDashCount(FMath::Clamp(GetDashCount(), 0.0f, GetMaxDashCount()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxDashCountAttribute())
	{
		SetMaxDashCount(FMath::Max(GetMaxDashCount(), 0.0f));
		SetDashCount(FMath::Clamp(GetDashCount(), 0.0f, GetMaxDashCount()));
	}
	else if (Data.EvaluatedData.Attribute == GetDashRegenRateAttribute())
	{
		SetDashRegenRate(FMath::Max(GetDashRegenRate(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetAmmoAttribute())
	{
		SetAmmo(FMath::Clamp(GetAmmo(), 0.0f, GetMaxAmmo()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxAmmoAttribute())
	{
		SetMaxAmmo(FMath::Max(GetMaxAmmo(), 0.0f));
		SetAmmo(FMath::Clamp(GetAmmo(), 0.0f, GetMaxAmmo()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxSkill1CountAttribute())
	{
		SetMaxSkill1Count(FMath::Max(GetMaxSkill1Count(), 0.0f));
		SetSkill1Count(FMath::Clamp(GetSkill1Count(), 0.0f, GetMaxSkill1Count()));
	}
	else if (Data.EvaluatedData.Attribute == GetSkill1CountAttribute())
	{
		SetSkill1Count(FMath::Clamp(GetSkill1Count(), 0.0f, GetMaxSkill1Count()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxSkill2CountAttribute())
	{
		SetMaxSkill2Count(FMath::Max(GetMaxSkill2Count(), 0.0f));
		SetSkill2Count(FMath::Clamp(GetSkill2Count(), 0.0f, GetMaxSkill2Count()));
	}
	else if (Data.EvaluatedData.Attribute == GetSkill2CountAttribute())
	{
		SetSkill2Count(FMath::Clamp(GetSkill2Count(), 0.0f, GetMaxSkill2Count()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxSkill3CountAttribute())
	{
		SetMaxSkill3Count(FMath::Max(GetMaxSkill3Count(), 0.0f));
		SetSkill3Count(FMath::Clamp(GetSkill3Count(), 0.0f, GetMaxSkill3Count()));
	}
	else if (Data.EvaluatedData.Attribute == GetSkill3CountAttribute())
	{
		SetSkill3Count(FMath::Clamp(GetSkill3Count(), 0.0f, GetMaxSkill3Count()));
	}
}

float UNSPlayerAttributeSet::HandlePreHealthDamage(
	float DamageAmount,
	const FGameplayEffectModCallbackData& Data)
{
	// 데미지를 받으면 강제로 ShieldRecharge관련 태그들을 리셋하고 다시 시작
	ResetShieldRechargeFlowOnDamage();
	
	// 데미지를 받은 시점에 데미지를 받았다는 트리거를 이벤트 태그로 발송
	ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(Data.Target.GetAvatarActor());
	if (PlayerCharacter)
	{
		PlayerCharacter->ApplyReactiveGameplayEffect(NSGameplayTags::Event_Common_DamageTaken);
	}

	const float CurrentShield = GetShield();
	
	if (CurrentShield <= 0.0f)
	{
		return DamageAmount;
	}
	
	const float AbsorbedDamage = FMath::Min(CurrentShield, DamageAmount);
	const float NewShield = CurrentShield - AbsorbedDamage;
	SetShield(NewShield);

	if (NewShield <= 0.0f)
	{
		if (PlayerCharacter)
		{
			PlayerCharacter->ApplyReactiveGameplayEffect(NSGameplayTags::Event_Common_Shield_Broken);
		}
	}

	// 쉴드 감소 시점에도 Notify : 쉴드가 0 이하로 떨어지면 Broken, 그 외는 ShieldHit 타입으로
	NotifyHitTakenFeedback(
		Data,
		NewShield <= 0.0f ? ENSHitTakenFeedbackType::ShieldBroken : ENSHitTakenFeedbackType::ShieldHit,
		AbsorbedDamage);
	
	return DamageAmount - AbsorbedDamage;
}

void UNSPlayerAttributeSet::ResetShieldRechargeFlowOnDamage() const
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	FGameplayTagContainer RechargeTags;
	RechargeTags.AddTag(NSGameplayTags::State_Shield_Recharging);
	RechargeTags.AddTag(NSGameplayTags::State_Shield_RechargeCooldown);
	ASC->RemoveActiveEffectsWithGrantedTags(RechargeTags);
}

void UNSPlayerAttributeSet::NotifyHitTakenFeedbackAfterHealthDamage(
	const FGameplayEffectModCallbackData& Data,
	const float PreviousHealth) const
{
	const float AppliedHealthDamage = FMath::Max(PreviousHealth - GetHealth(), 0.0f);
	if (AppliedHealthDamage <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	
	// Health 감소 시점에도 Notify : 일반적으로 HealthHit 타입
	NotifyHitTakenFeedback(Data, ENSHitTakenFeedbackType::HealthHit, AppliedHealthDamage);
}

void UNSPlayerAttributeSet::NotifyHitTakenFeedback(
	const FGameplayEffectModCallbackData& Data,
	const ENSHitTakenFeedbackType FeedbackType,
	const float DamageAmount) const
{
	if (DamageAmount <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	
	AActor* TargetActor = Data.Target.GetAvatarActor();
	APawn* TargetPawn = Cast<APawn>(TargetActor);
	if (!TargetActor || !TargetPawn)
	{
		return;
	}
	
	ANSPlayerController* PlayerController = Cast<ANSPlayerController>(TargetPawn->GetController());
	if (!PlayerController)
	{
		return;
	}
	
	FNSHitTakenFeedbackContext FeedbackContext;
	FeedbackContext.FeedbackType = FeedbackType;
	FeedbackContext.DamageAmount = DamageAmount;
	FeedbackContext.HitLocation = TargetActor->GetActorLocation();
	// Shield의 현재 %
	FeedbackContext.ShieldRatio = GetMaxShield() > 0.0f
		                              ? FMath::Clamp(GetShield() / GetMaxShield(), 0.0f, 1.0f)
		                              : 0.0f;
	// Health의 현재 %
	FeedbackContext.HealthRatio = GetMaxHealth() > 0.0f
		                              ? FMath::Clamp(GetHealth() / GetMaxHealth(), 0.0f, 1.0f)
		                              : 0.0f;
	
	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
	if (const FHitResult* HitResult = EffectContext.GetHitResult())
	{
		FeedbackContext.HitLocation = HitResult->ImpactPoint;
	}
	
	if (AActor* InstigatorActor = EffectContext.GetInstigator())
	{
		FeedbackContext.InstigatorLocation = InstigatorActor->GetActorLocation();
	}
	
	// 컨트롤러 - 피격 피드백 컴포넌트에서 피격 피드백 재생까지 이어지는 흐름 진입
	PlayerController->Client_PlayHitTakenFeedback(FeedbackContext);
}

void UNSPlayerAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, Shield, OldShield);
}

void UNSPlayerAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, MaxShield, OldMaxShield);
}

void UNSPlayerAttributeSet::OnRep_ShieldRechargeRate(const FGameplayAttributeData& OldShieldRechargeRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, ShieldRechargeRate, OldShieldRechargeRate);
}

void UNSPlayerAttributeSet::OnRep_ShieldRechargeCooldown(const FGameplayAttributeData& OldShieldRechargeCooldown)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, ShieldRechargeCooldown, OldShieldRechargeCooldown);
}

void UNSPlayerAttributeSet::OnRep_DashCount(const FGameplayAttributeData& OldDashCount)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, DashCount, OldDashCount);
}

void UNSPlayerAttributeSet::OnRep_MaxDashCount(const FGameplayAttributeData& OldMaxDashCount)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, MaxDashCount, OldMaxDashCount);
}

void UNSPlayerAttributeSet::OnRep_DashRegenRate(const FGameplayAttributeData& OldDashRegenRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, DashRegenRate, OldDashRegenRate);
}

void UNSPlayerAttributeSet::OnRep_Ammo(const FGameplayAttributeData& OldAmmo)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, Ammo, OldAmmo);
}

void UNSPlayerAttributeSet::OnRep_MaxAmmo(const FGameplayAttributeData& OldMaxAmmo)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, MaxAmmo, OldMaxAmmo);
}
void UNSPlayerAttributeSet::OnRep_MaxSkill1Count(const FGameplayAttributeData& OldMaxSkill1Count)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, MaxSkill1Count, OldMaxSkill1Count);
}

void UNSPlayerAttributeSet::OnRep_Skill1Count(const FGameplayAttributeData& OldSkill1Count)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, Skill1Count, OldSkill1Count);
}

void UNSPlayerAttributeSet::OnRep_MaxSkill2Count(const FGameplayAttributeData& OldMaxSkill2Count)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, Skill2Count, OldMaxSkill2Count);
}

void UNSPlayerAttributeSet::OnRep_Skill2Count(const FGameplayAttributeData& OldSkill2Count)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, Skill2Count, OldSkill2Count);
}

void UNSPlayerAttributeSet::OnRep_MaxSkill3Count(const FGameplayAttributeData& OldMaxSkill3Count)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, Skill3Count, OldMaxSkill3Count);
}

void UNSPlayerAttributeSet::OnRep_Skill3Count(const FGameplayAttributeData& OldSkill3Count)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNSPlayerAttributeSet, Skill2Count, OldSkill3Count);
}
