// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Custom Collision Profile 모음
 */
namespace NSCollisionProfiles
{
	const inline FName PlayerCharacter(TEXT("NS_PlayerCharacter"));
	const inline FName EnemyCharacter(TEXT("NS_EnemyCharacter"));
	const inline FName DestructibleObject(TEXT("NS_DestructibleObject"));
	const inline FName PlayerTurret(TEXT("NS_PlayerTurret"));
	const inline FName PlayerBarrier(TEXT("NS_PlayerBarrier"));
	const inline FName Projectile(TEXT("NS_Projectile"));
	const inline FName DroneGround(TEXT("NS_DroneGround"));
	const inline FName DroneAvoidance(TEXT("NS_DroneAvoidance"));
}
