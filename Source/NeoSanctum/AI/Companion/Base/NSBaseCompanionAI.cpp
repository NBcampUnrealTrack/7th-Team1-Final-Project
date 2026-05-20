// Copyright 2026 One Team. All rights reserved.


#include "NSBaseCompanionAI.h"
#include "Components/SceneComponent.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
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
	
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	
	AttributeSet = CreateDefaultSubobject<UAttributeSet>("AttributeSet");
}

void ANSBaseCompanionAI::BeginPlay()
{
	Super::BeginPlay();
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}


