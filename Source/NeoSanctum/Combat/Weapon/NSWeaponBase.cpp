// Copyright 2026 One Team. All rights reserved.


#include "NSWeaponBase.h"

#include "Components/SceneComponent.h"

ANSWeaponBase::ANSWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bNetUseOwnerRelevancy = true;
	SetReplicates(true);
	
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

bool ANSWeaponBase::TryGetLeftHandIKTransform(FTransform& OutTransform) const
{
	if (LeftHandIKSocketName.IsNone())
	{
		return false;
	}

	TArray<USceneComponent*> SceneComponents;
	GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (!IsValid(SceneComponent) || !SceneComponent->DoesSocketExist(LeftHandIKSocketName))
		{
			continue;
		}

		OutTransform = SceneComponent->GetSocketTransform(LeftHandIKSocketName, RTS_World);
		return true;
	}

	return false;
}
