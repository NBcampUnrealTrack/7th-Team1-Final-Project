// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"

ANSEnemyCharacterBase::ANSEnemyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
	
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
    AttributeSet = CreateDefaultSubobject<UNSMonsterAttributeSet>(TEXT("AttributeSet"));
}

void ANSEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}
