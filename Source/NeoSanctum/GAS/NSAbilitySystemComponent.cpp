// Copyright 2026 One Team. All rights reserved.


#include "NSAbilitySystemComponent.h"

#include "GameplayAbility/GA_SkillBase.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"
#include "NeoSanctum/Tag/NSGameplayTags_Slot.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "Stats/NSCombatStatComponent.h"

void UNSAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
	
	AbilityFailedCallbacks.AddUObject(this, &ThisClass::HandleAbilityFailed);
}

void UNSAbilitySystemComponent::HandleAbilityFailed(
	const UGameplayAbility* FailedAbility, const FGameplayTagContainer& FailureTags)
{
	const UGA_SkillBase* SkillAbility = Cast<UGA_SkillBase>(FailedAbility);
	
	if (!SkillAbility 
		|| !SkillAbility->ShouldRequestReloadOnEmptyAmmo()
		|| !FailureTags.HasTagExact(NSGameplayTags::Ability_ActivateFail_OutOfAmmo)
		|| HasMatchingGameplayTag(NSGameplayTags::State_Reloading))
	{
		return;
	}
	
	APawn* AvatarPawn = Cast<APawn>(GetAvatarActor());
	
	if (!IsValid(AvatarPawn) || !AvatarPawn->IsLocallyControlled())
	{
		return;
	}
	
	FGameplayEventData ReloadEventData;
	ReloadEventData.EventTag = NSGameplayTags::Event_Common_RequestReload;
	ReloadEventData.Instigator = AvatarPawn;
	ReloadEventData.Target = AvatarPawn;
	
	const int32 ActivateAbilityCount = HandleGameplayEvent(ReloadEventData.EventTag, &ReloadEventData);
	
	NS_ACTOR_LOG(AvatarPawn, LogNSGAS, Log,
		"빈 탄창 재장전 이벤트 전달. 활성화수={ActivatedAbilityCount}",
		("ActivatedAbilityCount", ActivatedAbilityCount)
	);
}

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

	// 연속 충전 재시작에 사용할 Cooldown 저장
	CacheSkillRechargeCooldown(SkillSlotTag, Cooldown);

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
	const FActiveGameplayEffectHandle RechargeEffectHandle =
		ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	if (!RechargeEffectHandle.IsValid())
	{
		return;
	}

	BroadcastSkillCooldownUIData(SkillSlotTag);

	// Duration 만료 시 SkillCount 회복 처리
	if (FOnActiveGameplayEffectRemoved_Info* RemovedDelegate =
		OnGameplayEffectRemoved_InfoDelegate(RechargeEffectHandle))
	{
		RemovedDelegate->AddUObject(
			this,
			&ThisClass::HandleSkillRechargeEffectRemoved,
			SkillSlotTag
		);
	}
}

bool UNSAbilitySystemComponent::GetSkillCooldownUIData(
	const FGameplayTag& SkillSlotTag,
	FSkillCooldownUIData& OutData) const
{
	// UI가 바로 사용할 수 있도록 슬롯 기준 쿨다운 상태를 구성
	OutData = FSkillCooldownUIData();
	OutData.SkillSlotTag = SkillSlotTag;

	if (!SkillSlotTag.IsValid())
	{
		return false;
	}

	if (!TryGetSkillCountForSlot(SkillSlotTag, OutData.CurrentCount, OutData.MaxCount))
	{
		return false;
	}

	const FGameplayTag RechargeEffectTag = GetRechargeEffectTagForSlot(SkillSlotTag);
	if (!RechargeEffectTag.IsValid())
	{
		return true;
	}

	// Recharge GE의 남은 시간과 전체 시간을 조회
	FGameplayTagContainer RechargeEffectTags;
	RechargeEffectTags.AddTag(RechargeEffectTag);

	FGameplayEffectQuery RechargeQuery;
	RechargeQuery.OwningTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(RechargeEffectTags);

	const TArray<TPair<float, float>> RechargeTimes =
		GetActiveEffectsTimeRemainingAndDuration(RechargeQuery);

	for (const TPair<float, float>& RechargeTime : RechargeTimes)
	{
		const float RemainingTime = RechargeTime.Key;
		const float TotalTime = RechargeTime.Value;

		if (RemainingTime > OutData.RemainingTime)
		{
			OutData.RemainingTime = FMath::Max(RemainingTime, 0.0f);
			OutData.TotalTime = FMath::Max(TotalTime, 0.0f);
		}
	}

	OutData.bIsRecharging = OutData.RemainingTime > 0.0f && OutData.TotalTime > 0.0f;
	if (OutData.bIsRecharging)
	{
		// 0.0에서 시작해 1.0으로 차오르는 형태의 진행도
		OutData.NormalizedProgress = FMath::Clamp(
			1.0f - (OutData.RemainingTime / OutData.TotalTime),
			0.0f,
			1.0f
		);
	}

	return true;
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

bool UNSAbilitySystemComponent::TryGetSkillCountForSlot(
	const FGameplayTag& SkillSlotTag,
	int32& OutCurrentCount,
	int32& OutMaxCount) const
{
	// 슬롯 태그를 PlayerAttributeSet의 SkillCount Attribute로 매핑
	OutCurrentCount = 0;
	OutMaxCount = 0;

	const UNSPlayerAttributeSet* PlayerAttributeSet = GetSet<UNSPlayerAttributeSet>();
	if (!PlayerAttributeSet)
	{
		return false;
	}

	if (SkillSlotTag.MatchesTagExact(NSGameplayTags::SkillSlot_Skill1))
	{
		OutCurrentCount = FMath::Max(FMath::FloorToInt(PlayerAttributeSet->GetSkill1Count()), 0);
		OutMaxCount = FMath::Max(FMath::FloorToInt(PlayerAttributeSet->GetMaxSkill1Count()), 0);
		return true;
	}

	if (SkillSlotTag.MatchesTagExact(NSGameplayTags::SkillSlot_Skill2))
	{
		OutCurrentCount = FMath::Max(FMath::FloorToInt(PlayerAttributeSet->GetSkill2Count()), 0);
		OutMaxCount = FMath::Max(FMath::FloorToInt(PlayerAttributeSet->GetMaxSkill2Count()), 0);
		return true;
	}

	if (SkillSlotTag.MatchesTagExact(NSGameplayTags::SkillSlot_Skill3))
	{
		OutCurrentCount = FMath::Max(FMath::FloorToInt(PlayerAttributeSet->GetSkill3Count()), 0);
		OutMaxCount = FMath::Max(FMath::FloorToInt(PlayerAttributeSet->GetMaxSkill3Count()), 0);
		return true;
	}

	return false;
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

void UNSAbilitySystemComponent::HandleSkillRechargeEffectRemoved(
	const FGameplayEffectRemovalInfo& RemovalInfo,
	FGameplayTag SkillSlotTag)
{
	// 강제 제거된 GE는 충전 완료로 보지 않음
	if (RemovalInfo.bPrematureRemoval)
	{
		return;
	}

	FinishSkillRecharge(SkillSlotTag);
}

void UNSAbilitySystemComponent::FinishSkillRecharge(const FGameplayTag& SkillSlotTag)
{
	if (!IsOwnerActorAuthoritative())
	{
		return;
	}

	// 충전 하나를 회복
	AddSkillCountForSlot(SkillSlotTag, 1.0f);
	BroadcastSkillCooldownUIData(SkillSlotTag);

	if (IsSkillCountFull(SkillSlotTag))
	{
		return;
	}

	// 아직 최대치가 아니면 다음 충전을 이어서 시작
	StartSkillRecharge(SkillSlotTag, GetCachedSkillRechargeCooldown(SkillSlotTag));
}

void UNSAbilitySystemComponent::BroadcastSkillCooldownUIData(const FGameplayTag& SkillSlotTag) const
{
	// GMS로 전달할 슬롯 쿨다운 상태를 구성
	FSkillCooldownUIData CooldownData;
	if (!GetSkillCooldownUIData(SkillSlotTag, CooldownData))
	{
		return;
	}

	FNSSkillCooldownMessage Message;
	Message.SkillSlotTag = SkillSlotTag;
	Message.CooldownData = CooldownData;

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(
		NSGameplayTags::Message_UI_SkillCooldown_Changed,
		Message
	);
}

void UNSAbilitySystemComponent::AddSkillCountForSlot(const FGameplayTag& SkillSlotTag, float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	const UNSPlayerAttributeSet* PlayerAttributeSet = GetSet<UNSPlayerAttributeSet>();
	if (!PlayerAttributeSet)
	{
		return;
	}

	if (SkillSlotTag.MatchesTagExact(NSGameplayTags::SkillSlot_Skill1))
	{
		// Skill1Count를 MaxSkill1Count 이하로 회복
		const float NewValue = FMath::Min(
			PlayerAttributeSet->GetSkill1Count() + Amount,
			PlayerAttributeSet->GetMaxSkill1Count()
		);
		SetNumericAttributeBase(UNSPlayerAttributeSet::GetSkill1CountAttribute(), NewValue);
		return;
	}

	if (SkillSlotTag.MatchesTagExact(NSGameplayTags::SkillSlot_Skill2))
	{
		// Skill2Count를 MaxSkill2Count 이하로 회복
		const float NewValue = FMath::Min(
			PlayerAttributeSet->GetSkill2Count() + Amount,
			PlayerAttributeSet->GetMaxSkill2Count()
		);
		SetNumericAttributeBase(UNSPlayerAttributeSet::GetSkill2CountAttribute(), NewValue);
		return;
	}

	if (SkillSlotTag.MatchesTagExact(NSGameplayTags::SkillSlot_Skill3))
	{
		// Skill3Count를 MaxSkill3Count 이하로 회복
		const float NewValue = FMath::Min(
			PlayerAttributeSet->GetSkill3Count() + Amount,
			PlayerAttributeSet->GetMaxSkill3Count()
		);
		SetNumericAttributeBase(UNSPlayerAttributeSet::GetSkill3CountAttribute(), NewValue);
	}
}

void UNSAbilitySystemComponent::CacheSkillRechargeCooldown(const FGameplayTag& SkillSlotTag, float Cooldown)
{
	if (!SkillSlotTag.IsValid() || Cooldown <= 0.0f)
	{
		return;
	}

	CachedSkillRechargeCooldowns.FindOrAdd(SkillSlotTag) = Cooldown;
}

float UNSAbilitySystemComponent::GetCachedSkillRechargeCooldown(const FGameplayTag& SkillSlotTag) const
{
	// 저장된 값이 없으면 재시작하지 않도록 0 반환
	const float* Cooldown = CachedSkillRechargeCooldowns.Find(SkillSlotTag);
	if (!Cooldown)
	{
		return 0.0f;
	}

	return *Cooldown;
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
