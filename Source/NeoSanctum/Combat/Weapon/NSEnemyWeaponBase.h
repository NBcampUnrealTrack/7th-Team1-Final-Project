// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "NSEnemyWeaponBase.generated.h"

class UAnimInstance;
class USkeletalMeshComponent;

USTRUCT(BlueprintType)
struct FWeaponConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Config")
	TSubclassOf<UAnimInstance> AnimBlueprintClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Config")
	FName EquipSocketName = TEXT("WeaponSocket_R");

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Config")
	FTransform RelativeTransform;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Config|IK")
	bool bUseLeftHandIKWhileEquipped = false;
};

UCLASS()
class NEOSANCTUM_API ANSEnemyWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	ANSEnemyWeaponBase();

	FORCEINLINE const FWeaponConfig& GetWeaponConfig() const { return WeaponConfig; }

	void StartDissolve();

	bool TryGetLeftHandIKTransform(FTransform& OutTransform) const;
	
	bool TryGetMuzzleTransform(FTransform& OutTransform) const;
	
	bool ShouldUseLeftHandIKWhileEquipped() const { return WeaponConfig.bUseLeftHandIKWhileEquipped; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(EditDefaultsOnly)
	FWeaponConfig WeaponConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|IK")
	FName LeftHandIKSocketName = TEXT("S_LeftHandIK");
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Projectile")
	FName MuzzleSocketName = TEXT("S_Muzzle");

private:
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	TObjectPtr<class UNSDissolveComponent> DissolveComponent;
};
