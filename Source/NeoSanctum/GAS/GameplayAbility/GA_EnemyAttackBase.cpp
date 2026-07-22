// Copyright 2026 One Team. All rights reserved.


#include "GA_EnemyAttackBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GenericTeamAgentInterface.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"

UGA_EnemyAttackBase::UGA_EnemyAttackBase()
{
	// Advanced 세팅 설정
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;

	bServerRespectsRemoteAbilityCancellation = false;
	bRetriggerInstancedAbility = false;

	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Enemy_Attack);
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(NSGameplayTags::State_Enemy_HitReacting);

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

void UGA_EnemyAttackBase::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	RemovePatternDefenseEffect();

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UGA_EnemyAttackBase::BeginPatternDefenseWindow(ENSPatternDefenseWindow Window)
{
	if (PatternDefenseWindow != Window)
	{
		return;
	}

	ApplyPatternDefenseEffect();
}

void UGA_EnemyAttackBase::EndPatternDefenseWindow(ENSPatternDefenseWindow Window)
{
	if (PatternDefenseWindow != Window)
	{
		return;
	}

	RemovePatternDefenseEffect();
}

void UGA_EnemyAttackBase::ApplyPatternDefenseEffect()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (!ASC || !IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	if (!PatternDefenseEffect || PatternDefenseBonus <= 0.0f)
	{
		return;
	}

	RemovePatternDefenseEffect();

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddInstigator(AvatarActor, AvatarActor);
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle =
		ASC->MakeOutgoingSpec(PatternDefenseEffect, GetAbilityLevel(), EffectContext);

	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(
		NSGameplayTags::Effect_SetByCaller_Defense_Add,
		PatternDefenseBonus);

	PatternDefenseEffectHandle =
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void UGA_EnemyAttackBase::RemovePatternDefenseEffect()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (ASC && PatternDefenseEffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(PatternDefenseEffectHandle);
	}

	PatternDefenseEffectHandle = FActiveGameplayEffectHandle();
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
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);

	BeginPatternDefenseWindow(ENSPatternDefenseWindow::AttackMontage);

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
	EndPatternDefenseWindow(ENSPatternDefenseWindow::AttackMontage);
	HandleAttackMontageCompleted();
}

void UGA_EnemyAttackBase::OnMontageBlendOut()
{
	EndPatternDefenseWindow(ENSPatternDefenseWindow::AttackMontage);
}

void UGA_EnemyAttackBase::OnMontageInterrupted()
{
	EndPatternDefenseWindow(ENSPatternDefenseWindow::AttackMontage);
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

	EffectContext.AddInstigator(SourceActor, SourceActor);
	EffectContext.AddSourceObject(this);
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

AActor* UGA_EnemyAttackBase::FindNearestKnownTarget(const APawn* AttackerPawn) const
{
	// 공격자 유효성 검사
	if (!IsValid(AttackerPawn))
	{
		return nullptr;
	}

	// ThreatComponent 찾아서 가져오기
	UNSEnemyThreatComponent* ThreatComponent = AttackerPawn->FindComponentByClass<UNSEnemyThreatComponent>();
	if (!ThreatComponent)
	{
		return nullptr;
	}

	// 공격대상 후보 변수 등록
	TArray<AActor*> Candidates;
	ThreatComponent->GetKnownTargets(Candidates);

	// 가장 가까운 공격대상을 확정할 변수 추가
	const FVector PawnLocation = AttackerPawn->GetActorLocation();
	AActor* NearestTarget = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();

	// 후보군들을 돌며 가장 가까운대상 반환
	for (AActor* Candidate : Candidates)
	{
		if (!IsValid(Candidate))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(PawnLocation, Candidate->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestTarget = Candidate;
		}
	}

	// 결정된 가장 가까운 대상 리턴
	return NearestTarget;
}
