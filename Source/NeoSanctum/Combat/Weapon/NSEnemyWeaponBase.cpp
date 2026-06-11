// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyWeaponBase.h"

#include "NeoSanctum/System/Component/NSDissolveComponent.h"


ANSEnemyWeaponBase::ANSEnemyWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	DissolveComponent = CreateDefaultSubobject<UNSDissolveComponent>(TEXT("DissolveComponent"));
}

void ANSEnemyWeaponBase::StartDissolve()
{
	if (DissolveComponent)
	{
		DissolveComponent->StartDissolve();
	}
}

bool ANSEnemyWeaponBase::TryGetLeftHandIKTransform(FTransform& OutTransform) const
{
	if (!IsValid(WeaponMesh) ||
		LeftHandIKSocketName.IsNone() ||
		!WeaponMesh->DoesSocketExist(LeftHandIKSocketName))
	{
		return false;
	}

	OutTransform = WeaponMesh->GetSocketTransform(LeftHandIKSocketName,RTS_World);

	return true;
}

bool ANSEnemyWeaponBase::TryGetMuzzleTransform(FTransform& OutTransform) const
{
	if (!IsValid(WeaponMesh) ||
		MuzzleSocketName.IsNone() ||
		!WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		return false;
	}
	
	OutTransform = WeaponMesh->GetSocketTransform(MuzzleSocketName, RTS_World);
	
	return true;
}
