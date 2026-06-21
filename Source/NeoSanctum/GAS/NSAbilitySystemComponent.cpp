// Copyright 2026 One Team. All rights reserved.


#include "NSAbilitySystemComponent.h"

#include "GameplayAbility/GA_SkillBase.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "NeoSanctum/Tag/NSGameplayTags_Slot.h"
#include "Stats/NSCombatStatComponent.h"

void UNSAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	// ASC가 Ability를 순회하는 동안 목록 변경을 막는 스코프 락
	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.Ability)
		{
			continue;
		}

		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		// 입력상태를 즉시 실행하지 않고 큐에 저장
		InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
		InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
	}
}

void UNSAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	// ASC가 Ability를 순회하는 동안 목록 변경을 막는 스코프 락
	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.Ability)
		{
			continue;
		}

		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
		InputHeldSpecHandles.Remove(AbilitySpec.Handle);
	}
}

void UNSAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	if (bGamePaused)
	{
		ClearAbilityInput();
		return;
	}

	TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

	// 입력을 유지하는 동안 반복 활성화되는 Ability 처리
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);

		if (!AbilitySpec || !AbilitySpec->Ability || AbilitySpec->IsActive())
		{
			continue;
		}

		const UGA_SkillBase* SkillAbility = Cast<UGA_SkillBase>(AbilitySpec->Ability);
		if (!SkillAbility)
		{
			continue;
		}

		if (SkillAbility->GetActivationPolicy() == ENSAbilityActivationPolicy::WhileInputActive)
		{
			AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
		}
	}

	// 이번 프레임에 막 눌린 입력 처리
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);

		if (!AbilitySpec || !AbilitySpec->Ability)
		{
			continue;
		}

		AbilitySpec->InputPressed = true;

		if (AbilitySpec->IsActive())
		{
			// 이미 실행 중인 Ability라면 입력 이벤트만 전달
			AbilitySpecInputPressed(*AbilitySpec);
			continue;
		}

		const UGA_SkillBase* SkillAbility = Cast<UGA_SkillBase>(AbilitySpec->Ability);
		const bool bShouldActivateOnPress = !SkillAbility ||
			SkillAbility->GetActivationPolicy() == ENSAbilityActivationPolicy::OnInputTriggered;

		if (bShouldActivateOnPress)
		{
			AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
		}
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(SpecHandle);
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);

		if (!AbilitySpec || !AbilitySpec->Ability)
		{
			continue;
		}

		AbilitySpec->InputPressed = false;

		if (AbilitySpec->IsActive())
		{
			AbilitySpecInputReleased(*AbilitySpec);
		}
	}

	// Held는 유지해야 하므로 Pressed/Released 이벤트만 비움
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UNSAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

void UNSAbilitySystemComponent::StartSkillRecharge(const FGameplayTag& SkillSlotTag, float Cooldown)
{
	if (!IsOwnerActorAuthoritative())
	{
		return;
	}

	if (!SkillSlotTag.IsValid() || Cooldown <= 0.0f)
	{
		return;
	}

	if (IsSkillRechargeActive(SkillSlotTag))
	{
		return;
	}

	if (IsSkillCountFull(SkillSlotTag))
	{
		return;
	}

	TSubclassOf<UGameplayEffect> RechargeGEClass = GetRechargeGEClassForSlot(SkillSlotTag);
	if (!RechargeGEClass)
	{
		return;
	}

	FGameplayEffectContextHandle Context = MakeEffectContext();
	Context.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(RechargeGEClass, 1.0f, Context);

	if (!SpecHandle.IsValid())
	{
		return;
	}
	
	// 이미 찾아 둔 RechargeGE의 Duration을 Cooldown으로 적용
	SpecHandle.Data->SetDuration(Cooldown, true);
	ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

bool UNSAbilitySystemComponent::IsSkillRechargeActive(const FGameplayTag& SkillSlotTag) const
{
	const FGameplayTag RechargeEffectTag = GetRechargeEffectTagForSlot(SkillSlotTag);
	if (!RechargeEffectTag.IsValid())
	{
		return false;
	}

	return HasMatchingGameplayTag(RechargeEffectTag);
}

bool UNSAbilitySystemComponent::IsSkillCountFull(const FGameplayTag& SkillSlotTag) const
{
	const UNSPlayerAttributeSet* PlayerAttributeSet = GetSet<UNSPlayerAttributeSet>();
	if (!PlayerAttributeSet)
	{
		return true;
	}

	if (SkillSlotTag.MatchesTagExact(NSGameplayTags::SkillSlot_Skill1))
	{
		return PlayerAttributeSet->GetSkill1Count() >= PlayerAttributeSet->GetMaxSkill1Count();
	}

	if (SkillSlotTag.MatchesTagExact(NSGameplayTags::SkillSlot_Skill2))
	{
		return PlayerAttributeSet->GetSkill2Count() >= PlayerAttributeSet->GetMaxSkill2Count();
	}

	if (SkillSlotTag.MatchesTagExact(NSGameplayTags::SkillSlot_Skill3))
	{
		return PlayerAttributeSet->GetSkill3Count() >= PlayerAttributeSet->GetMaxSkill3Count();
	}

	return true;
}

TSubclassOf<UGameplayEffect> UNSAbilitySystemComponent::GetRechargeGEClassForSlot(
	const FGameplayTag& SkillSlotTag) const
{
	if (SkillSlotTag.MatchesTagExact(NSGameplayTags::SkillSlot_Skill1))
	{
		return Skill1RechargeGEClass;
	}

	if (SkillSlotTag.MatchesTagExact(NSGameplayTags::SkillSlot_Skill2))
	{
		return Skill2RechargeGEClass;
	}

	if (SkillSlotTag.MatchesTagExact(NSGameplayTags::SkillSlot_Skill3))
	{
		return Skill3RechargeGEClass;
	}
	
	return nullptr;
}

FGameplayTag UNSAbilitySystemComponent::GetRechargeEffectTagForSlot(const FGameplayTag& SkillSlotTag) const
{
	if (SkillSlotTag.MatchesTagExact(NSGameplayTags::SkillSlot_Skill1))
	{
		return NSGameplayTags::Effect_Recharge_Skill1Count;
	}

	if (SkillSlotTag.MatchesTagExact(NSGameplayTags::SkillSlot_Skill2))
	{
		return NSGameplayTags::Effect_Recharge_Skill2Count;
	}

	if (SkillSlotTag.MatchesTagExact(NSGameplayTags::SkillSlot_Skill3))
	{
		return NSGameplayTags::Effect_Recharge_Skill3Count;
	}

	return FGameplayTag();
}

bool UNSAbilitySystemComponent::TryGetBaseAbilityStat(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag,
	float& OutValue) const
{
	const ANSPlayerState* NSPlayerState = GetOwner<ANSPlayerState>();

	if (!NSPlayerState)
	{
		return false;
	}

	const UNSCombatStatComponent* CombatStatComponent = NSPlayerState->GetCombatStatComponent();

	if (!CombatStatComponent)
	{
		return false;
	}

	return CombatStatComponent->TryGetBaseAbilityStat(AbilityTag, StatTag, OutValue);
}

bool UNSAbilitySystemComponent::TryGetFinalAbilityStat(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag,
	float& OutValue) const
{
	const ANSPlayerState* NSPlayerState = GetOwner<ANSPlayerState>();
	
	if (!NSPlayerState)
	{
		return false;
	}
	
	const UNSCombatStatComponent* CombatStatComponent = NSPlayerState->GetCombatStatComponent();
	
	if (!CombatStatComponent)
	{
		return false;
	}
	
	return CombatStatComponent->TryGetFinalAbilityStat(AbilityTag, StatTag, OutValue);
}

bool UNSAbilitySystemComponent::IsAbilityStatModifiable(
	const FGameplayTag& AbilityTag,
	const FGameplayTag& StatTag) const
{
	const ANSPlayerState* NSPlayerState = GetOwner<ANSPlayerState>();

	if (!NSPlayerState)
	{
		return false;
	}

	const UNSCombatStatComponent* CombatStatComponent = NSPlayerState->GetCombatStatComponent();

	if (!CombatStatComponent)
	{
		return false;
	}

	return CombatStatComponent->IsAbilityStatModifiable(AbilityTag, StatTag);
}

FGuid UNSAbilitySystemComponent::AddTemporaryCombatStatModifier(
	const FGameplayTag& TargetAbilityTag,
	const FGameplayTag& StatTag,
	ENSCombatStatModifierOperation Operation,
	float Value,
	float Duration) const
{
	const ANSPlayerState* NSPlayerState = GetOwner<ANSPlayerState>();

	if (!NSPlayerState)
	{
		return FGuid();
	}

	UNSCombatStatComponent* CombatStatComponent = NSPlayerState->GetCombatStatComponent();

	if (!CombatStatComponent)
	{
		return FGuid();
	}

	return CombatStatComponent->AddTemporaryCombatStatModifier(
		TargetAbilityTag,
		StatTag,
		Operation,
		Value,
		Duration
	);
}

void UNSAbilitySystemComponent::RemoveTemporaryCombatStatModifier(FGuid Handle) const
{
	const ANSPlayerState* NSPlayerState = GetOwner<ANSPlayerState>();

	if (!NSPlayerState)
	{
		return;
	}

	UNSCombatStatComponent* CombatStatComponent = NSPlayerState->GetCombatStatComponent();

	if (!CombatStatComponent)
	{
		return;
	}

	CombatStatComponent->RemoveTemporaryCombatStatModifier(Handle);
}
