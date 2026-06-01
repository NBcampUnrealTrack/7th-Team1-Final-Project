// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyWeaponBase.h"

#include "NeoSanctum/System/Component/NSDissolveComponent.h"


ANSEnemyWeaponBase::ANSEnemyWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;

	DissolveComponent = CreateDefaultSubobject<UNSDissolveComponent>(TEXT("DissolveComponent"));

	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANSEnemyWeaponBase::StartDissolve()
{
	if (DissolveComponent)
	{
		DissolveComponent->StartDissolve();
	}
}
