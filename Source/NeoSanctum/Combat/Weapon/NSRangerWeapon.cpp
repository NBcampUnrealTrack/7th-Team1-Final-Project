// Copyright 2026 One Team. All rights reserved.


#include "NSRangerWeapon.h"


bool ANSRangerWeapon::TryGetAttackOriginTransform(FTransform& OutTransform) const
{
	if (AttackOriginComponentName.IsNone() || AttackOriginSocketName.IsNone())
	{
		return false;
	}
	
	TArray<UStaticMeshComponent*> StaticMeshComponents;
	GetComponents<UStaticMeshComponent>(StaticMeshComponents);
	
	for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
	{
		if (!IsValid(StaticMeshComponent))
		{
			continue;
		}
		
		if (StaticMeshComponent->GetFName() != AttackOriginComponentName)
		{
			continue;
		}
		
		if (!StaticMeshComponent->DoesSocketExist(AttackOriginSocketName))
		{
			return false;
		}
		
		OutTransform = StaticMeshComponent->GetSocketTransform(AttackOriginSocketName);
		return true;
	}
	
	return false;
}
