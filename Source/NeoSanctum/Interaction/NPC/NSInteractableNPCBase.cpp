// Copyright 2026 One Team. All rights reserved.

#include "NSInteractableNPCBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Components/SphereComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "NeoSanctum/System/Minimap/NSMinimapIconComponent.h"


ANSInteractableNPCBase::ANSInteractableNPCBase()
{
	PrimaryActorTick.bCanEverTick = false;
	
	DetectionCollision =
  CreateDefaultSubobject<USphereComponent>(TEXT("DetectionCollision"));
	DetectionCollision->SetupAttachment(GetRootComponent());
	DetectionCollision->SetSphereRadius(DetectionRadius);
	DetectionCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	PromptAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("PromptAnchor"));
	PromptAnchor->SetupAttachment(GetRootComponent());
	PromptAnchor->SetRelativeLocation(FVector(0.f, 0.f, 100.f));

	MinimapIconComponent = CreateDefaultSubobject<UNSMinimapIconComponent>(TEXT("MinimapIconComponent"));
	MinimapIconComponent->SetShowOnMinimap(false);
}

void ANSInteractableNPCBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터에서 DetectionRadius를 바꾸면 스피어 반경도 즉시 반영
	if (DetectionCollision)
	{
		DetectionCollision->SetSphereRadius(DetectionRadius);
	}
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

FVector ANSInteractableNPCBase::GetPromptWorldLocation_Implementation() const
{
	if (PromptAnchor)
	{
		return PromptAnchor->GetComponentLocation();
	}
	return GetActorLocation();
}

