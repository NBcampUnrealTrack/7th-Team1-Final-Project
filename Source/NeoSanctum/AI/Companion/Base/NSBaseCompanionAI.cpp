// Copyright 2026 One Team. All rights reserved.


#include "NSBaseCompanionAI.h"
#include "Components/SceneComponent.h"
#include "NeoSanctum/GAS/NSCompanionAbilitySystemComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSCompanionAttributeSet.h"
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
	
	CompanionAbilitySystemComponent = CreateDefaultSubobject<UNSCompanionAbilitySystemComponent>("AbilitySystemComponent");
	
	CompanionAttributeSet = CreateDefaultSubobject<UNSCompanionAttributeSet>("AttributeSet");
}

void ANSBaseCompanionAI::BeginPlay()
{
	Super::BeginPlay();
	
	if (CompanionAbilitySystemComponent)
	{
		CompanionAbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}


