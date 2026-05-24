// Copyright 2026 One Team. All rights reserved.


#include "NSWeaponBase.h"


ANSWeaponBase::ANSWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	
	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	SetRootComponent(WeaponRoot);
}

FName ANSWeaponBase::GetAttachSocketName() const
{
	return AttachSocketName;
}

bool ANSWeaponBase::TryGetAttackOriginTransform(FTransform& OutTransform) const
{
	return false;
}
