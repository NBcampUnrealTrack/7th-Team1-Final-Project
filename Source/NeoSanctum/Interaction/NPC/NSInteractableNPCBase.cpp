// Copyright 2026 One Team. All rights reserved.

#include "NSInteractableNPCBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Components/SphereComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"


ANSInteractableNPCBase::ANSInteractableNPCBase()
{
	PrimaryActorTick.bCanEverTick = false;
	
	DetectionCollision =
  CreateDefaultSubobject<USphereComponent>(TEXT("DetectionCollision"));
	DetectionCollision->SetupAttachment(GetRootComponent());
	DetectionCollision->SetSphereRadius(DetectionRadius);
	DetectionCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

bool ANSInteractableNPCBase::CanInteract_Implementation(APlayerController* Interactor) const
{
	if (!Interactor)
	{
		return false;
	}
	const APlayerState* PS = Interactor->PlayerState;
	if (!PS)
	{
		return false;
	}
	
	const UNSPlayerProgressComponent* Progress = PS->FindComponentByClass<UNSPlayerProgressComponent>();
	if (!Progress)
	{
		return false;
	}
	return Progress->IsNPCUnlocked(NPCId);
}

FText ANSInteractableNPCBase::GetPromptText_Implementation() const
{
	return PromptText;
}

