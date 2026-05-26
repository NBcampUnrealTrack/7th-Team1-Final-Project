// Copyright 2026 One Team. All rights reserved.


#include "NSBaseCompanionAI.h"
#include "Components/SceneComponent.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"


// Sets default values
ANSBaseCompanionAI::ANSBaseCompanionAI()
{
	PrimaryActorTick.bCanEverTick = false;
	
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


