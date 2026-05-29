// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"
#include "NeoSanctum/System/Component/NSDissolveComponent.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameModeBase.h"

ANSEnemyCharacterBase::ANSEnemyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	AttributeSet = CreateDefaultSubobject<UNSMonsterAttributeSet>(TEXT("AttributeSet"));

	DissolveComponent = CreateDefaultSubobject<UNSDissolveComponent>(TEXT("DissolveComponent"));
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

void ANSEnemyCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANSEnemyCharacterBase, bIsDead);
}

void ANSEnemyCharacterBase::Die()
{
	if (bIsDead) return;

	if (HasAuthority())
	{
		bIsDead = true;
		OnRep_bIsDead();
		
		// (이용호 추가) 죽을 때 게임모드에 알림
		AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
		if (GameMode && GameMode->Implements<UNSRunGameModeInterface>())
		{
			INSRunGameModeInterface::Execute_NotifyEnemyKilled(GameMode, this);
		}

		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->UnPossess();
		}

		if (ASC && DeathAbilityClass)
		{
			ASC->TryActivateAbilityByClass(DeathAbilityClass);
		}
	}
}

void ANSEnemyCharacterBase::OnRep_bIsDead()
{
	// 물리 캡슐 콜리전 비활성화
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

	if (GetMesh())
	{
		// 애니메이션 인스턴스 중단
		GetMesh()->bPauseAnims = true;

		// 콜리전 프로필을 Ragdoll로 변경
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));

		// 스켈레탈 메시의 물리 시뮬레이션을 활성화
		GetMesh()->SetSimulatePhysics(true);

		// 디졸브 효과 적용
		if (DissolveComponent)
		{
			DissolveComponent->StartDissolve();
		}
	}
}
