// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSInRunNPCSpawner.generated.h"

class ANSInteractableNPCBase;

UCLASS()
class NEOSANCTUM_API ANSInRunNPCSpawner : public AActor
{
	GENERATED_BODY()

public:
	ANSInRunNPCSpawner();

protected:
	virtual void BeginPlay() override;

	// 이 방에서 랜덤으로 스폰될 NPC 후보 목록 (구출 NPC 외 다른 NPC도 등록 가능)
	UPROPERTY(EditAnywhere, Category = "NPCClass")
	TArray<TSubclassOf<ANSInteractableNPCBase>> SpawnableNPCClasses;

private:
	void SpawnRandomRescueNPC();
};
