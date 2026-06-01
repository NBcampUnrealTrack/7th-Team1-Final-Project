// Copyright 2026 One Team. All rights reserved.


#include "GA_EnemyAttackBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Combat/Weapon/NSEnemyWeaponBase.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
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

	AttackTraceDistance = 100.0f;
	AttackTraceRadius = 8.0f;

	HitCheckEventTag = NSGameplayTags::Event_Enemy_Hit;
}

void UGA_EnemyAttackBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo,
                                          const FGameplayAbilityActivationInfo ActivationInfo,
                                          const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bHasHitThisAttack = false;

	if (ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		if (ANSEnemyCharacterBase* EnemyChar = Cast<ANSEnemyCharacterBase>(ActorInfo->AvatarActor.Get()))
		{
			EnemyData = EnemyChar->GetEnemyData();
			if (EnemyData)
			{
				AttackTraceDistance = EnemyData->MaxAttackRange;
			}
		}
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (!AttackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, HitCheckEventTag, nullptr, false, false);

	if (EventTask)
	{
		EventTask->EventReceived.AddDynamic(this, &UGA_EnemyAttackBase::OnHitCheckEventReceived);
		EventTask->ReadyForActivation();
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		TEXT("AttackMontageTask"),
		AttackMontage,
		1.0f,
		NAME_None,
		false,
		1.0f,
		0.0f);

	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &UGA_EnemyAttackBase::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_EnemyAttackBase::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_EnemyAttackBase::OnMontageInterrupted);

		MontageTask->ReadyForActivation();
	}
}

void UGA_EnemyAttackBase::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_EnemyAttackBase::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_EnemyAttackBase::OnHitCheckEventReceived(FGameplayEventData Payload)
{
	if (bHasHitThisAttack) return;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!AvatarActor || !World) return;


	// 물리 충돌 판정
	FVector StartLocation = AvatarActor->GetActorLocation();
	FVector ForwardVector = AvatarActor->GetActorForwardVector();
	FVector EndLocation = StartLocation + (ForwardVector * AttackTraceDistance);

	ANSEnemyWeaponBase* AttachedWeapon = nullptr;
	TArray<AActor*> AttachedActors;
	AvatarActor->GetAttachedActors(AttachedActors);
	for (AActor* Actor : AttachedActors)
	{
		if (ANSEnemyWeaponBase* Weapon = Cast<ANSEnemyWeaponBase>(Actor))
		{
			AttachedWeapon = Weapon;
			break;
		}
	}

	if (AttachedWeapon)
	{
		if (UStaticMeshComponent* WeaponMesh = AttachedWeapon->GetComponentByClass<UStaticMeshComponent>())
		{
			FVector SocketStart = WeaponMesh->GetSocketLocation(TEXT("TraceStart"));
			FVector SocketEnd = WeaponMesh->GetSocketLocation(TEXT("TraceEnd"));

			if (!SocketStart.IsZero() || !SocketEnd.IsZero())
			{
				StartLocation = SocketStart;
				EndLocation = SocketEnd;
			}
		}
	}

	FHitResult HitResult;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(AttackTraceRadius);
	FCollisionQueryParams QueryParams;

	// 공격자 자신은 판정에서 제외
	QueryParams.AddIgnoredActor(AvatarActor);

	if (AttachedWeapon)
	{
		QueryParams.AddIgnoredActor(AttachedWeapon);
	}

	bool bHit = World->SweepSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		FQuat::Identity,
		ECC_Pawn, // 캐릭터 레이어 검사
		SphereShape,
		QueryParams
	);

#if !UE_BUILD_SHIPPING
	FColor DebugColor = bHit ? FColor::Red : FColor::Green;
	DrawDebugLine(World, StartLocation, EndLocation, DebugColor,
	              false, 1.0f, 0, 3.0f);
	DrawDebugSphere(World, StartLocation, AttackTraceRadius, 8, DebugColor,
	                false, 1.0f);
	DrawDebugSphere(World, EndLocation, AttackTraceRadius, 8, DebugColor,
	                false, 1.0f);
#endif

	// GE 적용
	if (bHit && HitResult.GetActor())
	{
		AActor* TargetActor = HitResult.GetActor();

		if (IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(TargetActor))
		{
			UAbilitySystemComponent* TargetASC = TargetInterface->GetAbilitySystemComponent();
			UAbilitySystemComponent* AIASC = GetAbilitySystemComponentFromActorInfo();

			if (TargetASC && AIASC && DamageEffectClass)
			{
				bHasHitThisAttack = true;

				// GE 발생 정보 생성
				FGameplayEffectContextHandle EffectContext = AIASC->MakeEffectContext();
				EffectContext.AddHitResult(HitResult);

				FGameplayEffectSpecHandle NewSpecHandle = AIASC->MakeOutgoingSpec(
					DamageEffectClass, 1.0f, EffectContext);
				if (NewSpecHandle.IsValid())
				{
					// 타겟 ASC에 GE 적용
					AIASC->ApplyGameplayEffectSpecToTarget(*NewSpecHandle.Data.Get(), TargetASC);
					UE_LOG(LogTemp, Log, TEXT("타겟 %s에게 GE 적용"), *TargetActor->GetName());
				}
			}
		}
	}

	if (GameplayCueTag.IsValid() && bHit)
	{
		GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(GameplayCueTag, FGameplayCueParameters());
	}
}
