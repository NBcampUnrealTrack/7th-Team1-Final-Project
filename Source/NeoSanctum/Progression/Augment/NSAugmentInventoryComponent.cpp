// Copyright 2026 One Team. All rights reserved.

#include "NSAugmentInventoryComponent.h"

UNSAugmentInventoryComponent::UNSAugmentInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSAugmentInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UNSAugmentInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

