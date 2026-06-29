// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/AI/Base/NSBaseDroneAI.h"
#include "NSCompanionDroneAI.generated.h"

UCLASS()
class NEOSANCTUM_API ANSCompanionDroneAI : public ANSBaseDroneAI
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ANSCompanionDroneAI();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
