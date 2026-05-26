// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSWeaponBase.h"
#include "NSRangerWeapon.generated.h"

UCLASS()
class NEOSANCTUM_API ANSRangerWeapon : public ANSWeaponBase
{
	GENERATED_BODY()

public:
	virtual bool TryGetAttackOriginTransform(FTransform& OutTransform) const override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName AttackOriginComponentName = TEXT("Body");
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName AttackOriginSocketName = TEXT("Muzzle");
};
