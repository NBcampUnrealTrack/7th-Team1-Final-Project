// Copyright 2026 One Team. All rights reserved.


#include "GA_RangerAutoFire.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "DrawDebugHelpers.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"

UGA_RangerAutoFire::UGA_RangerAutoFire()
{
}

void UGA_RangerAutoFire::ActivateAbility(
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
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	// 입력 즉시 첫 발을 발사한 뒤, 이후로는 타이머로 반복
	FireOnce();
	StartAutoFire();
	
	// Input.BaseAttack이 Release되면 연사 종료
	UAbilityTask_WaitInputRelease* ReleaseTask =
		UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnInputReleased);
	ReleaseTask->ReadyForActivation();
}

void UGA_RangerAutoFire::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	StopAutoFire();
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_RangerAutoFire::OnInputReleased(float TimeHeld)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_RangerAutoFire::StartAutoFire()
{
	if (FireInterval <= 0.0f)
	{
		return;
	}
	
	UWorld* World = GetWorld();
	
	if (!World)
	{
		return;
	}
	
	// 실제 발사 판정은 FireOnce 내부에서 서버 권한으로 제한
	World->GetTimerManager().SetTimer(
		AutoFireTimerHandle,
		this,
		&ThisClass::FireOnce,
		FireInterval,
		true
	);
}

void UGA_RangerAutoFire::StopAutoFire()
{
	UWorld* World = GetWorld();
	
	if (!World)
	{
		return;
	}
	
	World->GetTimerManager().ClearTimer(AutoFireTimerHandle);
}

void UGA_RangerAutoFire::FireOnce()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	
	if (!AvatarActor)
	{
		return;
	}
	
	// 데미지 판정은 서버에서만 처리
	if (!AvatarActor->HasAuthority())
	{
		return;
	}
	
	// GameplayCue도 일단 서버에서만 실행(리슨 서버에서 테스트)
	ExecuteMuzzleFireCue();
	PerformHitscan();
}

void UGA_RangerAutoFire::PerformHitscan()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	
	if (!AvatarActor)
	{
		return;
	}
	
	UWorld* World = GetWorld();
	
	if (!World)
	{
		return;
	}
	
	APawn* Pawn = Cast<APawn>(AvatarActor);
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	
	const FVector TraceStart = Pawn ? Pawn->GetPawnViewLocation() : AvatarActor->GetActorLocation();
	
	const FRotator AimRotation = Controller ? Controller->GetControlRotation() : AvatarActor->GetActorRotation();
	
	const FVector TraceDirection = AimRotation.Vector();
	const FVector TraceEnd = TraceStart + TraceDirection * TraceRange;
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);
	
	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		TraceChannel,
		QueryParams
	);
	

	// 서버 판정 확인요 디버그 라인
	if (bDrawDebugHitscan)
	{
		const FVector DebugEnd = bHit ? HitResult.ImpactPoint : TraceEnd;
		// 초록: 허공에 발사, 빨강: 대상에 명중
		const FColor DebugColor = bHit ? FColor::Red : FColor::Green;

		DrawDebugLine(
			World,
			TraceStart,
			DebugEnd,
			DebugColor,
			false,
			DebugLineDuration,
			0,
			DebugLineThickness
		);

		if (bHit)
		{
			DrawDebugPoint(
				World,
				HitResult.ImpactPoint,
				12.0f,
				FColor::Red,
				false,
				DebugLineDuration
			);
		}
	}

	if (!bHit)
	{
		return;
	}
	
	ApplyDamageToActor(HitResult.GetActor());
}

void UGA_RangerAutoFire::ApplyDamageToActor(AActor* TargetActor)
{
	if (!TargetActor || !DamageEffectClass)
	{
		return;
	}
	
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	
	if (!SourceASC || !TargetASC)
	{
		return;
	}
	
	FGameplayEffectSpecHandle DamageSpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	
	if (!DamageSpecHandle.IsValid())
	{
		return;
	}
	
	// GE_Damage -> GEC_DamageExecution -> Damage Meta Attribute 흐름으로 데미지 전달
	SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
}

void UGA_RangerAutoFire::ExecuteMuzzleFireCue()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	
	if (!ASC || !AvatarActor)
	{
		return;
	}

	FTransform MuzzleTransform;

	if (!TryGetMuzzleTransform(MuzzleTransform))
	{
		// 소캣을 못 찾으면 캐릭터 전방 위치로 임시 처리
		MuzzleTransform = FTransform(
			AvatarActor->GetActorRotation(),
			AvatarActor->GetActorLocation() + AvatarActor->GetActorForwardVector() * 100.0f
		);
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Instigator = AvatarActor;
	CueParameters.EffectCauser = AvatarActor;
	CueParameters.Location = MuzzleTransform.GetLocation();
	CueParameters.Normal = MuzzleTransform.GetRotation().GetForwardVector();

	ASC->ExecuteGameplayCue(NSGameplayTags::GameplayCue_Ranger_MuzzleFire, CueParameters);
}

bool UGA_RangerAutoFire::TryGetMuzzleTransform(FTransform& OutMuzzleTransform) const
{
	// TODO: 주현님이랑 의논해서 무기 캐싱 받아오기.
	AActor* WeaponActor = nullptr/*GetWeaponActor()*/;

	if (!WeaponActor)
	{
		return false;
	}

	TArray<UStaticMeshComponent*> MeshComponents;
	WeaponActor->GetComponents<UStaticMeshComponent>(MeshComponents);

	for (UStaticMeshComponent* MeshComponent : MeshComponents)
	{
		// Body에 만든 Muzzle 소켓만 찾으면 됨
		if (MeshComponent && MeshComponent->DoesSocketExist(MuzzleSocketName))
		{
			OutMuzzleTransform = MeshComponent->GetSocketTransform(MuzzleSocketName);
			return true;
		}
	}

	return false;
}
