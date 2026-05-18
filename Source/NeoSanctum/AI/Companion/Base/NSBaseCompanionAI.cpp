// Copyright 2026 One Team. All rights reserved.


#include "NSBaseCompanionAI.h"
#include "Components/SceneComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

// Sets default values
ANSBaseCompanionAI::ANSBaseCompanionAI()
{
	PrimaryActorTick.bCanEverTick = false;
	RootScene = CreateDefaultSubobject<USceneComponent>("SceneComponent");
	SetRootComponent(RootScene);
	
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMesh");
	MeshComponent->SetupAttachment(RootScene);
	
	MovementComp = CreateDefaultSubobject<UFloatingPawnMovement>("FloatingPawnMovement");
	MovementComp->UpdatedComponent = RootScene;
	
}


