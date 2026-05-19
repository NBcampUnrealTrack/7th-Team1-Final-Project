// Copyright 2026 One Team. All rights reserved.


#include "NSDroneAI.h"
#include "Components/StaticMeshComponent.h"

ANSDroneAI::ANSDroneAI()
{
	PrimaryActorTick.bCanEverTick = false;
	
}

void ANSDroneAI::BeginPlay()
{
	Super::BeginPlay();
	
	if (AbilitySystemComponent && AttributeSet)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("ASC And ATS Valid"));
	}
	
}


