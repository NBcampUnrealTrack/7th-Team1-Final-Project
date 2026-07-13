// Copyright 2026 One Team. All rights reserved.

#include "NSMonsterStatusViewModel.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"

bool UNSMonsterStatusViewModel::Initialize(AActor* InTargetActor)
{
	Shutdown();

	if (!IsValid(InTargetActor))
	{
		return false;
	}

	IAbilitySystemInterface* AbilitySystemInterface =
		Cast<IAbilitySystemInterface>(InTargetActor);
	if (!AbilitySystemInterface)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = AbilitySystemInterface->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	TargetActor = InTargetActor;
	TargetASC = ASC;

	ApplyDefaultNormalPolicy();
	BindAttributeDelegates();
	RefreshStatus();

	return true;
}

void UNSMonsterStatusViewModel::Shutdown()
{
	UnbindAttributeDelegates();

	TargetActor.Reset();
	TargetASC.Reset();
	CachedStatus = FNSMonsterUIStatus();
	OnStatusChanged.Clear();
}

void UNSMonsterStatusViewModel::ApplyDefaultNormalPolicy()
{
	CachedStatus.bShowName = false;
	CachedStatus.bShowHealth = true;
	CachedStatus.bShowHealthText = false;
	CachedStatus.bShowShield = false;
	CachedStatus.bShowShieldText = false;
	CachedStatus.bShowHitGauge = true;
	CachedStatus.bShowHitGaugeText = false;

	if (const AActor* Actor = TargetActor.Get())
	{
		if (const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(Actor))
		{
			if (const UNSEnemyData* EnemyData = EnemyAgent->GetEnemyData())
			{
				CachedStatus.MonsterName = FText::FromName(EnemyData->EnemyId.GetTagName());
			}
		}
	}
}

void UNSMonsterStatusViewModel::RefreshStatus()
{
	UAbilitySystemComponent* ASC = TargetASC.Get();
	if (!ASC)
	{
		return;
	}

	const float Health = ASC->GetNumericAttribute(UNSBaseAttributeSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(UNSBaseAttributeSet::GetMaxHealthAttribute());
	const float Shield = ASC->GetNumericAttribute(UNSMonsterAttributeSet::GetShieldAttribute());
	const float MaxShield = ASC->GetNumericAttribute(UNSMonsterAttributeSet::GetMaxShieldAttribute());
	const float HitGauge = ASC->GetNumericAttribute(UNSMonsterAttributeSet::GetHitGaugeAttribute());
	const float MaxHitGauge = ASC->GetNumericAttribute(UNSMonsterAttributeSet::GetMaxHitGaugeAttribute());

	CachedStatus.HealthPercent = MaxHealth > KINDA_SMALL_NUMBER
		                             ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f)
		                             : 0.0f;

	CachedStatus.ShieldPercent = MaxShield > KINDA_SMALL_NUMBER
		                             ? FMath::Clamp(Shield / MaxShield, 0.0f, 1.0f)
		                             : 0.0f;

	CachedStatus.HitGaugePercent = MaxHitGauge > KINDA_SMALL_NUMBER
		                               ? FMath::Clamp(HitGauge / MaxHitGauge, 0.0f, 1.0f)
		                               : 0.0f;

	CachedStatus.HealthText = MakeValueText(Health, MaxHealth);
	CachedStatus.ShieldText = MakeValueText(Shield, MaxShield);
	CachedStatus.HitGaugeText = MakeValueText(HitGauge, MaxHitGauge);

	CachedStatus.bShowShield = CachedStatus.bShowShield && MaxShield > KINDA_SMALL_NUMBER;
	CachedStatus.bShowHitGauge = CachedStatus.bShowHitGauge && MaxHitGauge > KINDA_SMALL_NUMBER;

	OnStatusChanged.Broadcast(CachedStatus);
}

void UNSMonsterStatusViewModel::BindAttributeDelegates()
{
	UAbilitySystemComponent* ASC = TargetASC.Get();
	if (!ASC)
	{
		return;
	}

	HealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(
		UNSBaseAttributeSet::GetHealthAttribute()).AddUObject(
		this,
		&ThisClass::HandleAttributeChanged);

	MaxHealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(
		UNSBaseAttributeSet::GetMaxHealthAttribute()).AddUObject(
		this,
		&ThisClass::HandleAttributeChanged);

	ShieldChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(
		UNSMonsterAttributeSet::GetShieldAttribute()).AddUObject(
		this,
		&ThisClass::HandleAttributeChanged);

	MaxShieldChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(
		UNSMonsterAttributeSet::GetMaxShieldAttribute()).AddUObject(
		this,
		&ThisClass::HandleAttributeChanged);

	HitGaugeChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(
		UNSMonsterAttributeSet::GetHitGaugeAttribute()).AddUObject(
		this,
		&ThisClass::HandleAttributeChanged);

	MaxHitGaugeChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(
		UNSMonsterAttributeSet::GetMaxHitGaugeAttribute()).AddUObject(
		this,
		&ThisClass::HandleAttributeChanged);
}

void UNSMonsterStatusViewModel::UnbindAttributeDelegates()
{
	UAbilitySystemComponent* ASC = TargetASC.Get();
	if (!ASC)
	{
		return;
	}

	if (HealthChangedHandle.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSBaseAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
		HealthChangedHandle.Reset();
	}

	if (MaxHealthChangedHandle.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSBaseAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthChangedHandle);
		MaxHealthChangedHandle.Reset();
	}

	if (ShieldChangedHandle.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSMonsterAttributeSet::GetShieldAttribute()).Remove(ShieldChangedHandle);
		ShieldChangedHandle.Reset();
	}

	if (MaxShieldChangedHandle.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSMonsterAttributeSet::GetMaxShieldAttribute()).Remove(MaxShieldChangedHandle);
		MaxShieldChangedHandle.Reset();
	}

	if (HitGaugeChangedHandle.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSMonsterAttributeSet::GetHitGaugeAttribute()).Remove(HitGaugeChangedHandle);
		HitGaugeChangedHandle.Reset();
	}

	if (MaxHitGaugeChangedHandle.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSMonsterAttributeSet::GetMaxHitGaugeAttribute()).Remove(MaxHitGaugeChangedHandle);
		MaxHitGaugeChangedHandle.Reset();
	}
}

void UNSMonsterStatusViewModel::HandleAttributeChanged(const FOnAttributeChangeData& Data)
{
	RefreshStatus();
}

FText UNSMonsterStatusViewModel::MakeValueText(float CurrentValue, float MaxValue) const
{
	return FText::FromString(FString::Printf(
		TEXT("%d / %d"),
		FMath::RoundToInt(CurrentValue),
		FMath::RoundToInt(MaxValue)));
}
