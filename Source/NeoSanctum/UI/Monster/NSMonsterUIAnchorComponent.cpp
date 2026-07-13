// Copyright 2026 One Team. All rights reserved.

#include "NSMonsterUIAnchorComponent.h"

UNSMonsterUIAnchorComponent::UNSMonsterUIAnchorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
}

FVector UNSMonsterUIAnchorComponent::GetAnchorLocation() const
{
	return GetComponentLocation();
}
