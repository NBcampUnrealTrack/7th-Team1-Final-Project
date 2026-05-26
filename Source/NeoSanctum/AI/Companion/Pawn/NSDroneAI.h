// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "NSDroneAI.generated.h"

class USphereComponent;

UCLASS()
class NEOSANCTUM_API ANSDroneAI : public ANSBaseCompanionAI
{
	GENERATED_BODY()

public:
	ANSDroneAI();
	
	virtual void GetActorEyesViewPoint( FVector& Location, FRotator& Rotation ) const override;
	
protected:
	virtual void BeginPlay() override;

private:
	ANSDroneAIController* DroneAIController;
	
	UBlackboardComponent* DroneAIBBComponent;
};
