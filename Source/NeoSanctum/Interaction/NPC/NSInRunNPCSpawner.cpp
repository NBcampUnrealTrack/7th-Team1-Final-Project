// Copyright 2026 One Team. All rights reserved.


#include "NSInRunNPCSpawner.h"
#include "NSInteractableNPCBase.h"

ANSInRunNPCSpawner::ANSInRunNPCSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// 런타임에도 존재하는 실제 루트 (에디터에서 위치 조정 가능하도록)
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ANSInRunNPCSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	SpawnRandomRescueNPC();
}

void ANSInRunNPCSpawner::SpawnRandomRescueNPC()
{
	if (SpawnableNPCClasses.IsEmpty())
	{
		return;
	}

	const TSubclassOf<ANSInteractableNPCBase> ChosenClass{SpawnableNPCClasses[FMath::RandHelper(SpawnableNPCClasses.Num())]};
	if (!ChosenClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	GetWorld()->SpawnActor<ANSInteractableNPCBase>(ChosenClass, GetActorTransform(), SpawnParams);
}
