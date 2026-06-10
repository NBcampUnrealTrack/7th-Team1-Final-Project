// Copyright 2026 One Team. All rights reserved.


#include "GA_EnemyAttackBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GenericTeamAgentInterface.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"

UGA_EnemyAttackBase::UGA_EnemyAttackBase()
{
	// Advanced 세팅 설정
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;

	bServerRespectsRemoteAbilityCancellation = false;
	bRetriggerInstancedAbility = false;

	HitCheckEventTag = NSGameplayTags::Event_Enemy_Hit;
}

void UGA_EnemyAttackBase::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo) ||
		!IsValid(AttackMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	InitializeAttack();

	UAbilityTask_WaitGameplayEvent* EventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, HitCheckEventTag, nullptr, false, false);

	if (IsValid(EventTask))
	{
		EventTask->EventReceived.AddDynamic(
			this, &ThisClass::OnAttackEventReceived);
		EventTask->ReadyForActivation();
	}

	if (!PlayAttackMontage())
	{
		CancelAttackAbility();
	}
}

bool UGA_EnemyAttackBase::PlayAttackMontage()
{
	if (!IsValid(AttackMontage))
	{
		return false;
	}

	PrepareForAttackMontage();

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			TEXT("AttackMontageTask"),
			AttackMontage,
			1.0f,
			NAME_None,
			false,
			1.0f,
			0.0f);

	if (!IsValid(MontageTask))
	{
		return false;
	}

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);

	MontageTask->ReadyForActivation();
	return true;
}

void UGA_EnemyAttackBase::InitializeAttack()
{
}

void UGA_EnemyAttackBase::PrepareForAttackMontage()
{
}

void UGA_EnemyAttackBase::HandleAttackEvent(const FGameplayEventData& Payload)
{
}

void UGA_EnemyAttackBase::HandleAttackMontageCompleted()
{
	FinishAttackAbility();
}

void UGA_EnemyAttackBase::OnAttackEventReceived(FGameplayEventData Payload)
{
	HandleAttackEvent(Payload);
}

void UGA_EnemyAttackBase::OnMontageCompleted()
{
	HandleAttackMontageCompleted();
}

void UGA_EnemyAttackBase::OnMontageInterrupted()
{
	CancelAttackAbility();
}

void UGA_EnemyAttackBase::FinishAttackAbility()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_EnemyAttackBase::CancelAttackAbility()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

bool UGA_EnemyAttackBase::TryApplyDamageToTarget(AActor* TargetActor, const FHitResult& HitResult)
{
	AActor* SourceActor = GetAvatarActorFromActorInfo();

	if (!IsValid(SourceActor) ||
		!IsValid(TargetActor) ||
		TargetActor == SourceActor)
	{
		return false;
	}

	const IGenericTeamAgentInterface* SourceTeam = Cast<IGenericTeamAgentInterface>(SourceActor);
	const IGenericTeamAgentInterface* TargetTeam = Cast<IGenericTeamAgentInterface>(TargetActor);

	if (SourceTeam && TargetTeam &&
		SourceTeam->GetGenericTeamId() == TargetTeam->GetGenericTeamId())
	{
		return false;
	}

	IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(TargetActor);

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = TargetInterface
		                                     ? TargetInterface->GetAbilitySystemComponent()
		                                     : nullptr;

	if (!IsValid(SourceASC) ||
		!IsValid(TargetASC) ||
		!DamageEffectClass)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();

	EffectContext.AddSourceObject(SourceActor);
	EffectContext.AddHitResult(HitResult);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, EffectContext);

	if (!SpecHandle.IsValid())
	{
		return false;
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	if (GameplayCueTag.IsValid())
	{
		SourceASC->ExecuteGameplayCue(GameplayCueTag, FGameplayCueParameters());
	}

	return true;
}
