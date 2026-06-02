// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSEnemyWeaponBase.generated.h"

class UGameplayAbility;
class UAnimInstance;
class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct FWeaponConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Config")
	TSubclassOf<UGameplayAbility> WeaponAbility;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Config")
	TSubclassOf<UAnimInstance> AnimBlueprintClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Config")
	FName EquipSocketName = TEXT("WeaponSocket_R");

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Config")
	FTransform RelativeTransform;
};

UCLASS()
class NEOSANCTUM_API ANSEnemyWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	ANSEnemyWeaponBase();

	FORCEINLINE const FWeaponConfig& GetWeaponConfig() const { return WeaponConfig; }

	void StartDissolve();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(EditDefaultsOnly)
	FWeaponConfig WeaponConfig;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	TObjectPtr<class UNSDissolveComponent> DissolveComponent;
};
