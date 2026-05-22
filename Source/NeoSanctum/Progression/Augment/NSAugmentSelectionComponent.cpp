// Copyright 2026 One Team. All rights reserved.


#include "NSAugmentSelectionComponent.h"

UNSAugmentSelectionComponent::UNSAugmentSelectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSAugmentSelectionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UNSAugmentSelectionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

