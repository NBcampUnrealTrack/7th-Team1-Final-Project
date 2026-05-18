// Copyright 2026 One Team. All rights reserved.


#include "NSCompanionAbilitySystemComponent.h"


UNSCompanionAbilitySystemComponent::UNSCompanionAbilitySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}


void UNSCompanionAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


void UNSCompanionAbilitySystemComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

