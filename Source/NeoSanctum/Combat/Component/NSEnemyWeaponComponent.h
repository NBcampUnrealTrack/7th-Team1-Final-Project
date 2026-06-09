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

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	ANSEnemyWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

public:
	// 몬스터 무기 스폰 후 착용
	UFUNCTION(BlueprintCallable, Category = "Weapon System")
	void EquipWeapon();
	//(이용호 추가) 무기 디스폰용
	UFUNCTION(BlueprintCallable, Category = "Weapon System")
	void UnEquipWeapon();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient, Replicated)
	TObjectPtr<ANSEnemyWeaponBase> CurrentWeapon;

	void OnOwnerDead();
};
