// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSDestructibleObjectBase.h"
#include "NSBossControlDevice.generated.h"

UCLASS()
class NEOSANCTUM_API ANSBossControlDevice : public ANSDestructibleObjectBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANSBossControlDevice();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
