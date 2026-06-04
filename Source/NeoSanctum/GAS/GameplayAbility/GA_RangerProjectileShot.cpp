// Copyright 2026 One Team. All rights reserved.


#include "GA_RangerProjectileShot.h"

#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Combat/Projectile/NSRangerProjectile.h"
#include "NeoSanctum/Combat/Weapon/NSWeaponBase.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"

UGA_RangerProjectileShot::UGA_RangerProjectileShot()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Ranger_ProjectileShot);
	SetAssetTags(AssetTags);
	
	ActivationPolicy = ENSAbilityActivationPolicy::OnInputTriggered;
}

void UGA_RangerProjectileShot::ActivateAbility(
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
	
	// 실제 데미지용 Projectile은 서버 권한에서만 스폰
	if (ActorInfo->IsNetAuthority())
	{
		TrySpawnProjectile();
	}
	
	if (bKeepAbilityActiveForDebug)
	{
		UWorld* World = GetWorld();
		
		if (World)
		{
			const float Duration = FMath::Max(DebugActiveDuration, 0.1f);
			
			// GameplayDebugger에서 Active 상태를 확인하는 타이머
			World->GetTimerManager().SetTimer(
				DebugEndAbilityTimerHandle,
				this,
				&ThisClass::FinishDebugAbility,
				Duration,
				false
			);
			
			return;
		}
	}
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_RangerProjectileShot::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DebugEndAbilityTimerHandle);
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_RangerProjectileShot::TrySpawnProjectile() const
{
	UWorld* World = GetWorld();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	
	if (!World || !IsValid(AvatarActor))
	{
		return false;
	}
	
	if (!ProjectileClass)
	{
		NS_ACTOR_LOG(AvatarActor, LogNSGAS, Warning, "ProjectileShot 실패. ProjectileClass가 설정되지 않음");
		return false;
	}
	
	FTransform MuzzleTransform;
	
	if (!TryGetAttackOriginTransform(MuzzleTransform))
	{
		// 무기 소켓을 찾지 못하면 캐릭터 전방으로 임시 발사
		MuzzleTransform = FTransform(
			AvatarActor->GetActorRotation(),
			AvatarActor->GetActorLocation() + AvatarActor->GetActorForwardVector() * 100.0f
		);
	}
	
	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	
	FVector AimPoint = MuzzleLocation + MuzzleTransform.GetRotation().GetForwardVector() * TraceRange;
	
	if (!TryGetAimPoint(AimPoint))
	{
		// 조준점 계산에 실패하면 총구 Forward 기준으로 발사
		AimPoint = MuzzleLocation + MuzzleTransform.GetRotation().GetForwardVector() * TraceRange;
	}
	
	const FVector LaunchDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
	
	if (LaunchDirection.IsNearlyZero())
	{
		NS_ACTOR_LOG(AvatarActor, LogNSGAS, Warning, "ProjectileShot 실패. 발사 방향을 계산할 수 없음");
		
		return false;
	}
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = AvatarActor;
	SpawnParameters.Instigator = Cast<APawn>(AvatarActor);
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	ANSRangerProjectile* Projectile = World->SpawnActor<ANSRangerProjectile>(
		ProjectileClass,
		MuzzleLocation,
		LaunchDirection.Rotation(),
		SpawnParameters
	);
	
	if (!IsValid(Projectile))
	{
		return false;
	}
	
	Projectile->LaunchProjectile(LaunchDirection);
	return true;
}

bool UGA_RangerProjectileShot::TryGetAttackOriginTransform(FTransform& OutTransform) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(AvatarActor);
	
	if (!IsValid(PlayerCharacter))
	{
		return false;
	}
	
	const ANSWeaponBase* CurrentWeapon = PlayerCharacter->GetCurrentWeapon();
	
	if (!IsValid(CurrentWeapon))
	{
		return false;
	}
	
	return CurrentWeapon->TryGetAttackOriginTransform(OutTransform);
}

bool UGA_RangerProjectileShot::TryGetAimPoint(FVector& OutAimPoint) const
{
	UWorld* World = GetWorld();
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(AvatarActor);
	
	if (!World || !IsValid(PlayerCharacter))
	{
		return false;
	}
	
	FVector TraceStart;
	
	if (!PlayerCharacter->TryGetAimTraceStartLocation(TraceStart))
	{
		return false;
	}
	
	const FVector TraceEnd = TraceStart + PlayerCharacter->GetControlRotation().Vector() * TraceRange;
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(RangerProjectileAimTrace), 
		false,
		PlayerCharacter
	);
	
	const ANSWeaponBase* CurrentWeapon = PlayerCharacter->GetCurrentWeapon();
	
	if (IsValid(CurrentWeapon))
	{
		QueryParams.AddIgnoredActor(CurrentWeapon);
	}
	
	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		TraceChannel,
		QueryParams
	);
	
	// 맞은 지점이 있으면 그 지점으로, 없으면 최대 사거리 지점으로 발사 방향 설정
	OutAimPoint = bHit ? HitResult.ImpactPoint : TraceEnd;
	return true;
}

void UGA_RangerProjectileShot::FinishDebugAbility()
{
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		false
	);
}
