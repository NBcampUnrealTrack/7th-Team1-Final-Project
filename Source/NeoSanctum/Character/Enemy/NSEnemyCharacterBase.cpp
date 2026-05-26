// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"

ANSEnemyCharacterBase::ANSEnemyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
	
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
    AttributeSet = CreateDefaultSubobject<UNSMonsterAttributeSet>(TEXT("AttributeSet"));
}

void ANSEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
	// 서버에서만 능력 부여
	if (HasAuthority() && ASC && AttackAbilityClass)
	{
		ASC->GiveAbility(FGameplayAbilitySpec(AttackAbilityClass, 1, -1));
	}
	
}
