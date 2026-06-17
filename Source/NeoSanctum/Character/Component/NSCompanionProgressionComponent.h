// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "NSCompanionProgressionComponent.generated.h"


class ANSBaseCompanionAI;
class UNSCompanionCatalog;

UCLASS(ClassGroup=(NEOSANCTUM), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSCompanionProgressionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UNSCompanionCatalog> Catalog;
	
	// 업그레이드
	UFUNCTION(Server, Reliable)
	void ServerTryUpgrade(FGameplayTag NodeTag);
	
	// 해금 판정 게이팅
	UFUNCTION(BlueprintPure)
	bool CanSelect(FGameplayTag CompanionTag) const;
	
	UFUNCTION(Server, Reliable)
	void ServerTrySelect(FGameplayTag CompanionTag);
	
	void SetOwnedCompanion(ANSBaseCompanionAI* Owner);
	
protected:
	UPROPERTY()
	TObjectPtr<ANSBaseCompanionAI> OwnedCompanion;
	
	// 세이브 데이터를 위한 진행상태 데이터
	UPROPERTY()
	FGameplayTag SelectedCompanionTag; // 선택중인 대상
	UPROPERTY()
	TMap<FGameplayTag, int32> NodeLevels; // 노드별 레벨
	UPROPERTY()
	TMap<FGameplayTag, int32> CompanionUpgradeCounts; // 누적 업그레이드
};
