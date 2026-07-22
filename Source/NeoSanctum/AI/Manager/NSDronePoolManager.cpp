// Copyright 2026 One Team. All rights reserved.


#include "NSDronePoolManager.h"

#include "NeoSanctum/Character/Enemy/NSEnemyDrone.h"

ANSEnemyDrone* UNSDronePoolManager::GetPooledDrone(TSubclassOf<ANSEnemyDrone> DroneClass, UNSEnemyData* EnemyData,
                                                   const FVector& Location, const FRotator& Rotation, const FNSDifficultyScale& Scale)
{
	if (!GetWorld() || !DroneClass || !EnemyData) return nullptr;
	
	FNSDronePoolArray& Pool = PoolMap.FindOrAdd(EnemyData);
	
	for (ANSEnemyDrone* Drone : Pool.Drones)
	{
		if (Drone && Drone->IsInPool())
		{
			Drone->SetNetDormancy(DORM_Awake);
			Drone->SetDifficultyScale(Scale);
			Drone->PrepareForReuse(Location, Rotation);
			Drone->FlushNetDormancy();
			return Drone;
		}
	}
	
	// 풀 부족하면 신규 생성 (BeginPlay 전 데이터 주입)
	FTransform SpawnTransform(Rotation, Location);

	ANSEnemyDrone* NewEnemyDrone = GetWorld()->SpawnActorDeferred<ANSEnemyDrone>(
		DroneClass, SpawnTransform,
		nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (NewEnemyDrone)
	{
		NewEnemyDrone->SetEnemyData(EnemyData);
		NewEnemyDrone->SetDifficultyScale(Scale); 
		NewEnemyDrone->FinishSpawning(SpawnTransform);
		Pool.Drones.Add(NewEnemyDrone);
		return NewEnemyDrone;
	}

	return nullptr;
}

void UNSDronePoolManager::ReturnDroneToPool(ANSEnemyDrone* Drone)
{
	if (!Drone) return;
	Drone->DeactivateForPool();
	Drone->SetNetDormancy(DORM_DormantAll);
	Drone->FlushNetDormancy();
}
