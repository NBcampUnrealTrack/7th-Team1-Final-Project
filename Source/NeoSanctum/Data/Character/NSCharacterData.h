// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "NeoSanctum/Data/Ability/NSCharacterDataTypes.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NSCharacterData.generated.h"

class ANSWeaponBase;
class UGameplayEffect;
class UAnimInstance;

/**
 * 특정 상황 태그가 발생했을 때 캐릭터에게 적용할 반응형 Gameplay Effect를 정의.
 */
USTRUCT(BlueprintType)
struct FNSReactiveGameplayEffectData
{
	GENERATED_BODY()

	// 반응형 GE를 실행할 상황 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|GAS")
	FGameplayTag TriggerTag;

	// 상황 태그가 발생했을 때 적용할 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|GAS",
		meta = (AssetBundles = "CommonData"))
	TSoftClassPtr<UGameplayEffect> EffectClass;
};

/**
 * 캐릭터 초기화시 필요한 PrimaryAsset입니다.
 */
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSCharacterData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	
public:
	// UI에서 클래스 선택시 구분하는 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	FGameplayTag CharacterTag;

	// 캐릭터 리더 스켈레톤 + 상시 표시되는 베이스 바디. DefaultVisualParts 미입력 슬롯의 fallback 역할도 겸함.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Visual",
		meta = (AssetBundles = "CommonData"))
	TSoftObjectPtr<USkeletalMesh> BaseLeaderMesh;

	// 장착 파츠가 없는 슬롯을 기본으로 채울 시각 파츠 목록. 시각 전용이라 GE/GA는 적용 안됨
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Visual")
	TArray<FNSDefaultVisualPartEntry> DefaultVisualParts;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Visual",
		meta = (AssetBundles = "CommonData"))
	TSoftClassPtr<UAnimInstance> AnimClass;

	// 캐릭터별 상체 Anim Layer
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Animation",
		meta = (AssetBundles = "CommonData"))
	TSoftClassPtr<UAnimInstance> UpperBodyAnimLayerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|GAS")
	TArray<FNSCharacterAbilityData> DefaultAbilities;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|GAS",
		meta = (AssetBundles = "CommonData"))
	TArray<TSoftClassPtr<UGameplayEffect>> DefaultGameplayEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|GAS")
	TArray<FNSReactiveGameplayEffectData> ReactiveGameplayEffects;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Weapon",
		meta = (AssetBundles = "CommonData"))
	TSoftClassPtr<ANSWeaponBase> DefaultWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Weapon",
		meta = (AssetBundles = "CommonData"))
	TSoftClassPtr<ANSWeaponBase> DefaultLeftHandWeaponClass;
};
