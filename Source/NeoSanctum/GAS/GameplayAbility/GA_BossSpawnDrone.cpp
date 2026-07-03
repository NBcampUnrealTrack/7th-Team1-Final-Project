// Copyright 2026 One Team. All rights reserved.


#include "GA_BossSpawnDrone.h"

#include "NeoSanctum/Character/Enemy/NSBossMotherShip.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"

UGA_BossSpawnDrone::UGA_BossSpawnDrone()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Enemy_MotherShip_SpawnDrone);
	SetAssetTags(AssetTags);
	
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Enemy_MotherShip_Charge);
}

void UGA_BossSpawnDrone::HandleAttackEvent(const FGameplayEventData& Payload)
{
	Super::HandleAttackEvent(Payload);
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor) return;
	
	ANSBossMotherShip* BossMotherShip = Cast<ANSBossMotherShip>(AvatarActor);
	if (!BossMotherShip || !BossMotherShip->HasAuthority()) return;
	
	BossMotherShip->SpawnEnemyDrone();
}


