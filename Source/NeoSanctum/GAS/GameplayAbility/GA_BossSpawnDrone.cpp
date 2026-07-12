// Copyright 2026 One Team. All rights reserved.


#include "GA_BossSpawnDrone.h"

#include "NeoSanctum/Character/Enemy/NSBossMotherShip.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"

UGA_BossSpawnDrone::UGA_BossSpawnDrone()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Enemy_MotherShip_SpawnDrone);
	SetAssetTags(AssetTags);
	
	//ActivationBlockedTags.AddTag(NSGameplayTags::State_Enemy_MotherShip_Charge);
	
	HitCheckEventTag = NSGameplayTags::Event_Enemy_Hit;
}

bool UGA_BossSpawnDrone::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;

	const ANSBossMotherShip* BossMotherShip = Cast<ANSBossMotherShip>(GetAvatarActorFromActorInfo());
	return BossMotherShip && BossMotherShip->HasSpawnWorkReady();
}

void UGA_BossSpawnDrone::HandleAttackEvent(const FGameplayEventData& Payload)
{
	Super::HandleAttackEvent(Payload);
	UE_LOG(LogTemp, Warning, TEXT("HandleAttackEvent called !"));
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor) return;
	
	ANSBossMotherShip* BossMotherShip = Cast<ANSBossMotherShip>(AvatarActor);
	if (!BossMotherShip || !BossMotherShip->HasAuthority()) return;
	
	UE_LOG(LogTemp, Warning, TEXT("SpawnEnemyDrone called !"));
	BossMotherShip->SpawnMaturedDrones();
}



