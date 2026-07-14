// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerWorldStatusViewModel.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"

// 대상 PlayerState와 ASC를 연결하는 함수
bool UNSPlayerWorldStatusViewModel::Initialize(ANSPlayerState* InPlayerState)
{
	Shutdown();

	if (!IsValid(InPlayerState))
	{
		return false;
	}

	UAbilitySystemComponent* ASC = InPlayerState->GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return false;
	}

	TargetPlayerState = InPlayerState;
	TargetASC = ASC;

	BindAttributeDelegates();
	RefreshStatus();

	return true;
}

// Attribute Delegate 구독을 해제하고 참조를 정리하는 함수
void UNSPlayerWorldStatusViewModel::Shutdown()
{
	UnbindAttributeDelegates();

	TargetPlayerState.Reset();
	TargetASC.Reset();
	CachedStatus = FNSPlayerWorldStatusData();
	OnStatusChanged.Clear();
}

// 현재 Attribute 값을 읽어 UI 상태값을 갱신하는 함수
void UNSPlayerWorldStatusViewModel::RefreshStatus()
{
	ANSPlayerState* PlayerState = TargetPlayerState.Get();
	UAbilitySystemComponent* ASC = TargetASC.Get();
	if (!IsValid(PlayerState) || !IsValid(ASC))
	{
		return;
	}

	const float Health = ASC->GetNumericAttribute(UNSBaseAttributeSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(UNSBaseAttributeSet::GetMaxHealthAttribute());

	CachedStatus.PlayerId = PlayerState->GetPlayerId();
	CachedStatus.PlayerName = FText::FromString(PlayerState->GetPlayerName());
	CachedStatus.HealthPercent = MaxHealth > KINDA_SMALL_NUMBER
		                             ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f)
		                             : 0.0f;
	CachedStatus.bIsDead = PlayerState->IsDead() || Health <= 0.0f;
	CachedStatus.bVisible = !CachedStatus.bIsDead && MaxHealth > KINDA_SMALL_NUMBER;

	OnStatusChanged.Broadcast(CachedStatus);
}

// Attribute 변경 Delegate를 구독하는 함수
void UNSPlayerWorldStatusViewModel::BindAttributeDelegates()
{
	UAbilitySystemComponent* ASC = TargetASC.Get();
	if (!IsValid(ASC))
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
}

// Attribute 변경 Delegate 구독을 해제하는 함수
void UNSPlayerWorldStatusViewModel::UnbindAttributeDelegates()
{
	UAbilitySystemComponent* ASC = TargetASC.Get();
	if (!IsValid(ASC))
	{
		HealthChangedHandle.Reset();
		MaxHealthChangedHandle.Reset();
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
}

// Attribute 변경 시 UI 상태값을 갱신하는 함수
void UNSPlayerWorldStatusViewModel::HandleAttributeChanged(const FOnAttributeChangeData& Data)
{
	RefreshStatus();
}
