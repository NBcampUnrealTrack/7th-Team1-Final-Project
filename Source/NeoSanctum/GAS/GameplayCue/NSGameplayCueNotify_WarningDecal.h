// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "NSGameplayCueNotify_WarningDecal.generated.h"

UCLASS()
class NEOSANCTUM_API ANSGameplayCueNotify_WarningDecal : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANSGameplayCueNotify_WarningDecal();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
