// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Custom Trace Channel 모음
 * 그냥 ECC_GameTraceChannel1 같은 식으로 사용할 수도 있으나
 * 아무래도 어떤 기능을 하는 Channel인지 명확하지 않아 따로 상수화시켜서 사용할 수 있게 함
 **/
namespace NSCollisionChannels
{
	constexpr ECollisionChannel Player = ECC_GameTraceChannel1;
	constexpr ECollisionChannel EnemyProjectile = ECC_GameTraceChannel2;
	constexpr ECollisionChannel Enemy = ECC_GameTraceChannel3;
	constexpr ECollisionChannel DestructibleObject = ECC_GameTraceChannel4;
	constexpr ECollisionChannel PlayerConstruct = ECC_GameTraceChannel5;

	constexpr ECollisionChannel CombatSight = ECC_GameTraceChannel6;
	constexpr ECollisionChannel EnemyWeaponTrace = ECC_GameTraceChannel7;
	constexpr ECollisionChannel ProjectileTrace = ECC_GameTraceChannel8;
	constexpr ECollisionChannel ExplosionTrace = ECC_GameTraceChannel9;
	constexpr ECollisionChannel PlayerWeaponTrace = ECC_GameTraceChannel10;
	constexpr ECollisionChannel DroneGround = ECC_GameTraceChannel11;
	constexpr ECollisionChannel DroneAvoid = ECC_GameTraceChannel12;
	constexpr ECollisionChannel EnemyMissile = ECC_GameTraceChannel13;
}
