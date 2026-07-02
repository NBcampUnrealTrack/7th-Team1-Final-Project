// Copyright 2026 One Team. All rights reserved.


#include "NSFlyingLocomotionComponent.h"


UNSFlyingLocomotionComponent::UNSFlyingLocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UNSFlyingLocomotionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UNSFlyingLocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

