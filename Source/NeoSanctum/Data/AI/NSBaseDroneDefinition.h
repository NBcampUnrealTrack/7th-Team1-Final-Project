// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSBaseDroneDefinition.generated.h"

class USkeletalMesh;
class UNSCompanionAbilitySet;
class UGameplayEffect;

UCLASS()
class NEOSANCTUM_API UNSBaseDroneDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 이 드론이 사용할 스켈레탈 메시 (스폰 시 비동기 로드 대상이라 SoftPtr)
	UPROPERTY(EditDefaultsOnly, Category="Drone|Visual")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	// 스폰 시 ASC에 부여할 능력 세트 (발사/공격 등)
	UPROPERTY(EditDefaultsOnly, Category="Drone|Ability")
	TObjectPtr<UNSCompanionAbilitySet> AbilitySet;

	// 타입별 기본 스탯을 세팅하는 Instant GameplayEffect
	UPROPERTY(EditDefaultsOnly, Category="Drone|Stats")
	TSubclassOf<UGameplayEffect> TypeStatsEffect;
};
