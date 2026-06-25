// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "NSCompanionProgressionComponent.generated.h"


class ANSBaseCompanionAI;
class UNSCompanionCatalog;
class UNSCompanionDefinition;

UCLASS(ClassGroup=(NEOSANCTUM), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSCompanionProgressionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UNSCompanionCatalog> Catalog;
	
	void SetOwnedCompanion(ANSBaseCompanionAI* Owner);
	
	// (이용호 추가) 허브 드론 즉시 변환용
	void ApplySelectedAndNodes(
		UNSCompanionDefinition* SelectedDefinition,
		const TMap<FGameplayTag, int32>& NodeLevels);
	// 노드 레벨 적용
	void ApplyNodeLevels(const TMap<FGameplayTag, int32>& NodeLevels);
	
protected:
	UPROPERTY()
	TObjectPtr<ANSBaseCompanionAI> OwnedCompanion;
};
