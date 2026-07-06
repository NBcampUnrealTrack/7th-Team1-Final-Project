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

void UGA_BossSpawnDrone::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ANSBossMotherShip* Boss = Cast<ANSBossMotherShip>(GetAvatarActorFromActorInfo()))
	{
		Boss->NotifySummonEnded();
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


