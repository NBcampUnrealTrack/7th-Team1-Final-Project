// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
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

	if (ASC)
	{
		ASC->InitAbilityActorInfo(this, this);

		if (HasAuthority() && AttributeSet)
		{
			AttributeSet->SetMaxHealth(100.0f);
			AttributeSet->SetHealth(100.0f);

			AttributeSet->SetDefense(10.0f);
			AttributeSet->SetBaseDamage(20.0f);
		}

		// 서버에서만 능력 부여
		if (HasAuthority())
		{
			if (AttackAbilityClass)
			{
				ASC->GiveAbility(FGameplayAbilitySpec(AttackAbilityClass, 1, -1));
			}

			if (DeathAbilityClass)
			{
				ASC->GiveAbility(FGameplayAbilitySpec(DeathAbilityClass, 1, -1));
			}
		}
	}
}

void ANSEnemyCharacterBase::HandleDeath()
{
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->UnPossess();
	}

	// 물리 캡슐 콜리전 비활성화
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

	// 메쉬 콜리전 프로파일 조정
	GetMesh()->SetCollisionProfileName(TEXT("NoCollision"));
}

void ANSEnemyCharacterBase::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	if (ASC && DeathAbilityClass)
	{
		ASC->TryActivateAbilityByClass(DeathAbilityClass);
	}
}
