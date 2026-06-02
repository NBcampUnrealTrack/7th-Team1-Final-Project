// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NSMonsterPoolManager.generated.h"



USTRUCT()
struct FNSMonsterPoolArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<ACharacter>> Monsters;
};

UCLASS()
class NEOSANCTUM_API UNSMonsterPoolManager : public UObject
{
	GENERATED_BODY()
	
public:
	ACharacter* GetPooledMonster(UClass* MonsterClass, const FVector& Location, const FRotator& Rotation);
	void ReturnMonsterToPool(ACharacter* Monster);

private:
	UPROPERTY(Transient)
	TMap<UClass*, FNSMonsterPoolArray> PoolMap;
	
};
