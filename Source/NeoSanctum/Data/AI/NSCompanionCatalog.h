// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "NSCompanionCatalog.generated.h"

class UNSCompanionDefinition;

UCLASS()
class NEOSANCTUM_API UNSCompanionCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// 트리에 등장하는 모든 드론 정의
	UPROPERTY(EditDefaultsOnly)
	TArray<TObjectPtr<UNSCompanionDefinition>> Companions;
	
	// 태그로 정의 찾기
	UNSCompanionDefinition* FindByTag(FGameplayTag InCompanionTag) const;
};
