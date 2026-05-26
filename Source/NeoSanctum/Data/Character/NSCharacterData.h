// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "NeoSanctum/Data/Ability/NSCharacterDataTypes.h"
#include "NSCharacterData.generated.h"

class ANSWeaponBase;
class UGameplayEffect;

/**
 * 캐릭터 초기화시 필요한 PrimaryAsset입니다.
 */
UCLASS()
class NEOSANCTUM_API UNSCharacterData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	
public:
	// UI에서 클래스 선택시 구분하는 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	FGameplayTag CharacterTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Visual")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Visual")
	TSoftClassPtr<UAnimInstance> AnimClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|GAS")
	TSoftClassPtr<UGameplayEffect> InitialAttributeEffect;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|GAS")
	TArray<FNSCharacterAbilityData> DefaultAbilities;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Weapon")
	TSoftClassPtr<ANSWeaponBase> DefaultWeaponClass;
};
