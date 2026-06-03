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
	
	NS_LOG(LogNS, Warning, "인풋 눌림 4");
	
	// 실제 데미지용 Projectile은 서버 권한에서만 스폰
	if (ActorInfo->IsNetAuthority())
	{
		TrySpawnProjectile();
	}
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
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
		NS_ACTOR_LOG(AvatarActor, LogNSGAS, Warning,
			"ProjectileShot 실패. ProjectileClass가 설정되지 않음");
		return false;
	}
	
	FTransform MuzzleTransform;
	
	if (!TryGetAttackOriginTransform(MuzzleTransform))
	{
		// 무기 소켓을 찾이 못하면 캐릭터 전방으로 임시 발사
		MuzzleTransform = FTransform(
			AvatarActor->GetActorRotation(),
			AvatarActor->GetActorLocation() + AvatarActor->GetActorForwardVector() * 100.0f
		);
	}
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = AvatarActor;
	SpawnParameters.Instigator = Cast<APawn>(AvatarActor);
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	ANSRangerProjectile* Projectile = World->SpawnActor<ANSRangerProjectile>(
		ProjectileClass,
		MuzzleTransform.GetLocation(),
		MuzzleTransform.GetRotation().Rotator(),
		SpawnParameters
	);
	
	return IsValid(Projectile);
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
