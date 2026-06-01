// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSEnemyWeaponComponent.generated.h"

class ANSEnemyWeaponBase;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSEnemyWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSEnemyWeaponComponent();
	
	// 몬스터 무기 스폰 후 착용
	UFUNCTION(BlueprintCallable, Category = "Weapon System")
	void EquipWeapon();
	
protected:
	UPROPERTY(Transient)
	TObjectPtr<ANSEnemyWeaponBase> CurrentWeapon;
};
