// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSGameplayCueNotify_Sustainable.h"
#include "NSGameplayCueNotify_Cloak.generated.h"

UCLASS()
class NEOSANCTUM_API ANSGameplayCueNotify_Cloak : public ANSGameplayCueNotify_Sustainable
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANSGameplayCueNotify_Cloak();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
