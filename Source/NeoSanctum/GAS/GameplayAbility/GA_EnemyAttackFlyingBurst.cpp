// Copyright 2026 One Team. All rights reserved.


#include "GA_EnemyAttackFlyingBurst.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "NeoSanctum/AI/Enemy/Controller/NSBossAIController.h"
#include "NeoSanctum/AI/Enemy/Controller/NSEnemyDroneAIController.h"
#include "NeoSanctum/Character/Enemy/NSEnemyPawnBase.h"
#include "NeoSanctum/Combat/Component/NSEnemyPartComponent.h"
#include "NeoSanctum/Combat/Projectile/NSProjectileManagerComponent.h"
#include "NeoSanctum/Combat/Projectile/NSProjectileTypes.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_EnemyAttackFlyingBurst::UGA_EnemyAttackFlyingBurst()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Enemy_FlyingBurstAttack);
	SetAssetTags(AssetTags);
	
	ActivationOwnedTags.AddTag(NSGameplayTags::State_Enemy_Combat);
	
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}

void UGA_EnemyAttackFlyingBurst::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	UGameplayAbility::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo,true,true);
		return;
	}
	
	CurrentVolley = 0;
	
	if (AttackMontage)
	{
		PlayAttackMontage();
	}
	
	FireOneVolley();
}

void UGA_EnemyAttackFlyingBurst::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsValid(PendingDelayTask))
	{
		PendingDelayTask->EndTask();
		PendingDelayTask = nullptr;
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_EnemyAttackFlyingBurst::HandleAttackMontageCompleted()
{
	// 흐름:
	// 의도적으로 비워둠. 부모는 여기서 FinishAttackAbility() 를 부르지만,
	// 버스트 종료 판단은 FireOneVolley 의 카운트/ WaitDelay 루프가 전담하므로
	// 몽타주 길이와 버스트 길이를 분리하기 위해 아무 것도 하지 않는다. (Super 호출 금지)
}

void UGA_EnemyAttackFlyingBurst::FireOneVolley()
{
	ANSEnemyPawnBase* NSEnemyPawn = Cast<ANSEnemyPawnBase>(GetAvatarActorFromActorInfo());
	if (!NSEnemyPawn || !NSEnemyPawn->HasAuthority())
	{
		CancelAttackAbility();
		return;
	}
	
	AActor* TargetActor = nullptr;
	ANSBossAIController* BossAIC = Cast<ANSBossAIController>(NSEnemyPawn->GetController());
	if (BossAIC)
	{
		TargetActor = BossAIC->GetCurrentAttackActor();
		if (!IsValid(TargetActor))
		{
			TargetActor = BossAIC->GetCurrentTargetActor();
		}
	}
	else if (ANSEnemyDroneAIController* EnemyDroneAIC = Cast<ANSEnemyDroneAIController>(NSEnemyPawn->GetController()))
	{
		TargetActor = EnemyDroneAIC->GetCurrentTargetActor();
	}
	
	if (!IsValid(TargetActor))
	{
		CancelAttackAbility();
		return;
	}
	
	const FNSEnemyAttackRow* Row = NSEnemyPawn->GetCurrentAttackRow();
	if (Row == nullptr)
	{
		CancelAttackAbility();
		return;
	}
	
	UNSEnemyPartComponent* Parts = NSEnemyPawn->GetPartComponent();
	TArray<FTransform> Muzzles;
	if (Parts)
	{
		Parts->GetMuzzleTransformsByAttackId(Row->AttackId, Muzzles);
	}
	
	if (Muzzles.IsEmpty())
	{
		CancelAttackAbility();
		return;
	}
	
	UWorld* World = GetWorld();
	ANSRunGameState* GS = World ? World->GetGameState<ANSRunGameState>() : nullptr;
	UNSProjectileManagerComponent* PM = GS ? GS->GetProjectileManagerComponent() : nullptr;
	if (!PM)
	{
		CancelAttackAbility();
		return;
	}
	
	const FVector AimLocation = TargetActor->GetActorLocation();
	for (const FTransform& Muzzle : Muzzles)
	{
		const FVector Start = Muzzle.GetLocation();
		FVector Dir = (AimLocation - Start).GetSafeNormal();
		if (Dir.IsNearlyZero()) Dir = Muzzle.GetRotation().GetForwardVector().GetSafeNormal();
		if (Dir.IsNearlyZero()) continue;
		
		FNSProjectileFireRequest Req;
		Req.StartLocation=Start;
		Req.Direction=Dir;
		Req.Speed=ProjectileSpeed;
		Req.MaxLifeTime=ProjectileMaxLifeTime;
		Req.Radius=ProjectileRadius;
		Req.TraceChannel=ProjectileTraceChannel;
		Req.SourceActor=NSEnemyPawn;
		Req.DamageEffectClass = DamageEffectClass;
		Req.Damage = CalculateProjectileDamage(*Row);
		PM->FireProjectile(Req); 
	}
	
	++CurrentVolley;
	if (CurrentVolley >= BurstCount)
	{
		FinishAttackAbility();
		return;
	}
	else
	{
		ScheduleNextVolley();
	}
}

void UGA_EnemyAttackFlyingBurst::ScheduleNextVolley()
{
	PendingDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, ShotInterval);
	if (!IsValid(PendingDelayTask))
	{
		FinishAttackAbility();
		return;
	}
	PendingDelayTask->OnFinish.AddDynamic(this, &ThisClass::OnShotDelayFinished);
	PendingDelayTask->ReadyForActivation();
}

void UGA_EnemyAttackFlyingBurst::OnShotDelayFinished()
{
	PendingDelayTask = nullptr;
	FireOneVolley();
}

float UGA_EnemyAttackFlyingBurst::CalculateProjectileDamage(const FNSEnemyAttackRow& AttackRow) const
{
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return -1.0f;
	}

	const float SourceBaseDamage =
		SourceASC->GetNumericAttribute(UNSBaseAttributeSet::GetBaseDamageAttribute());

	return FMath::Max(SourceBaseDamage * AttackRow.DamageScale, 0.0f);
}