// Copyright 2026 One Team. All rights reserved.

#include "GA_Guard.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimInstance.h"
#include "NeoSanctum/Combat/Weapon/Summon/NSBarrierBase.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_Guard::UGA_Guard()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Vanguard_Guard);
	SetAssetTags(AssetTags);
	SkillAbilityTag = NSGameplayTags::Ability_Vanguard_Guard;

	ActivationPolicy = ENSAbilityActivationPolicy::OnInputTriggered;
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Vanguard_Guarding);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Vanguard_Attacking);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dashing);
}

void UGA_Guard::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!ActorInfo->AbilitySystemComponent.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!HasValidBarrierConfig())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Barrier 생성에 필요한 스탯은 활성화 시점에 고정
	if (!TryResolveBarrierStats(CachedBarrierRadius, CachedBarrierDuration))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Guard 유지 상태 적용 후 몽타주 이벤트를 기다림
	AddGuardStateTags();
	ApplyGuardMoveSpeedEffect();
	StartGuardEventTask();

	if (!PlayGuardMontage())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UGA_Guard::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Ability 종료 시 활성 Task와 지속 효과를 정리
	if (GuardMontageTask)
	{
		GuardMontageTask->EndTask();
		GuardMontageTask = nullptr;
	}

	if (GuardEventTask)
	{
		GuardEventTask->EndTask();
		GuardEventTask = nullptr;
	}

	RemoveGuardMoveSpeedEffect();
	RemoveGuardStateTags();

	if (bDestroyBarrierOnEnd)
	{
		DestroyActiveGuardBarrier();
	}
	else
	{
		if (IsValid(ActiveGuardBarrier))
		{
			ActiveGuardBarrier->OnDestroyed.RemoveDynamic(this, &ThisClass::OnGuardBarrierDestroyed);
		}

		ActiveGuardBarrier = nullptr;
	}

	CachedBarrierRadius = 0.0f;
	CachedBarrierDuration = 0.0f;
	bGuardEndingFromInputRelease = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Guard::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	// 입력 해제는 한 번만 처리
	if (bGuardEndingFromInputRelease)
	{
		return;
	}

	// 종료 섹션이 있으면 몽타주 종료 연출을 먼저 재생
	bGuardEndingFromInputRelease = true;
	if (TryJumpToGuardEndSection())
	{
		return;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_Guard::OnGuardEventReceived(FGameplayEventData Payload)
{
	// Notify 시점에 Barrier 생성
	SpawnGuardBarrier();
}

void UGA_Guard::OnGuardMontageCompleted()
{
	// End 섹션까지 끝나면 Ability 종료
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Guard::OnGuardMontageInterrupted()
{
	// 중단된 경우 취소 종료로 처리
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_Guard::OnGuardBarrierDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor != ActiveGuardBarrier)
	{
		return;
	}

	ActiveGuardBarrier = nullptr;

	// Barrier가 먼저 사라지면 Guard 자세도 종료
	if (bGuardEndingFromInputRelease)
	{
		return;
	}

	bGuardEndingFromInputRelease = true;
	if (TryJumpToGuardEndSection())
	{
		return;
	}

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Guard::StartGuardEventTask()
{
	GuardEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		NSGameplayTags::Event_Vanguard_Guard,
		nullptr,
		false,
		true
	);

	if (!GuardEventTask)
	{
		return;
	}

	// 이벤트 바인딩
	GuardEventTask->EventReceived.AddDynamic(this, &ThisClass::OnGuardEventReceived);
	GuardEventTask->ReadyForActivation();
}

bool UGA_Guard::PlayGuardMontage()
{
	if (!GuardMontage)
	{
		return false;
	}

	const float MontagePlayRate = FMath::Max(GuardMontagePlayRate, 0.01f);
	GuardMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		GuardMontage,
		MontagePlayRate,
		NAME_None,
		true
	);

	if (!GuardMontageTask)
	{
		return false;
	}

	// BlendOut은 End 섹션 전환 중에도 발생할 수 있으므로 완료 처리하지 않음
	GuardMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnGuardMontageCompleted);
	GuardMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnGuardMontageInterrupted);
	GuardMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnGuardMontageInterrupted);
	GuardMontageTask->ReadyForActivation();

	return true;
}

bool UGA_Guard::TryJumpToGuardEndSection() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr;
	if (!AnimInstance || !GuardMontage || GuardEndSectionName.IsNone())
	{
		return false;
	}

	// 현재 Guard 몽타주가 재생 중일 때만 섹션 전환
	if (!AnimInstance->Montage_IsPlaying(GuardMontage))
	{
		return false;
	}

	AnimInstance->Montage_JumpToSection(GuardEndSectionName, GuardMontage);
	return true;
}

bool UGA_Guard::TryResolveBarrierStats(float& OutBarrierRadius, float& OutBarrierDuration) const
{
	return TryGetBarrierRadius(OutBarrierRadius) && TryGetBarrierDuration(OutBarrierDuration);
}

void UGA_Guard::SpawnGuardBarrier()
{
	// Guard 중 Barrier는 하나만 유지
	if (IsValid(ActiveGuardBarrier))
	{
		return;
	}

	// Barrier Health 등 SetByCaller 값은 스폰 직전에 만들어둠
	RebuildSetByCallerMagnitudes();
	ActiveGuardBarrier = SpawnBarrierActor(
		GetCurrentActorInfo(),
		CachedBarrierRadius,
		CachedBarrierDuration,
		GetSetByCallerMagnitudes()
	);

	if (!IsValid(ActiveGuardBarrier))
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
		return;
	}

	ActiveGuardBarrier->OnDestroyed.AddDynamic(this, &ThisClass::OnGuardBarrierDestroyed);
}

void UGA_Guard::AddGuardStateTags()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// 중복 추가를 피하기 위해 태그 카운트를 1로 고정
		ASC->SetLooseGameplayTagCount(NSGameplayTags::State_Vanguard_Guarding, 1);
		ASC->SetLooseGameplayTagCount(NSGameplayTags::State_Vanguard_BarrierAttackWindow, 1);
		bGuardStateTagsAdded = true;
	}
}

void UGA_Guard::RemoveGuardStateTags()
{
	if (!bGuardStateTagsAdded)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// RemoveLooseGameplayTag Error를 해결하기 위해 카운트를 명시적으로 0으로 설정
		ASC->SetLooseGameplayTagCount(NSGameplayTags::State_Vanguard_Guarding, 0);
		ASC->SetLooseGameplayTagCount(NSGameplayTags::State_Vanguard_BarrierAttackWindow, 0);
	}

	bGuardStateTagsAdded = false;
}

void UGA_Guard::ApplyGuardMoveSpeedEffect()
{
	if (!GuardMoveSpeedEffectClass || GuardMoveSpeedEffectHandle.IsValid())
	{
		return;
	}

	// Guard 중 이동속도 감소 효과는 Ability 종료 시 직접 제거함
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle =
		MakeOutgoingGameplayEffectSpec(GuardMoveSpeedEffectClass, GetAbilityLevel());

	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return;
	}

	GuardMoveSpeedEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void UGA_Guard::RemoveGuardMoveSpeedEffect()
{
	if (!GuardMoveSpeedEffectHandle.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveActiveGameplayEffect(GuardMoveSpeedEffectHandle);
	}

	GuardMoveSpeedEffectHandle.Invalidate();
}

void UGA_Guard::DestroyActiveGuardBarrier()
{
	ANSBarrierBase* BarrierToDestroy = ActiveGuardBarrier;
	ActiveGuardBarrier = nullptr;
	
	// 이미 파괴되어있으므로 return
	if (!IsValid(BarrierToDestroy))
	{
		return;
	}
	
	// 배리어 파괴 바인딩
	BarrierToDestroy->OnDestroyed.RemoveDynamic(this, &ThisClass::OnGuardBarrierDestroyed);

	if (BarrierToDestroy->HasAuthority())
	{
		BarrierToDestroy->Destroy();
	}
}
