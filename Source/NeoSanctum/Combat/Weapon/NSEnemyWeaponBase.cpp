// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyWeaponBase.h"


ANSEnemyWeaponBase::ANSEnemyWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;

	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
