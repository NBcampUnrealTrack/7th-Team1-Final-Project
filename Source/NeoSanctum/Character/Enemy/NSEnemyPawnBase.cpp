// Copyright 2026 One Team. All rights reserved.

#include "NSEnemyPawnBase.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NeoSanctum/Collision/NSCollisionProfiles.h"
#include "NeoSanctum/Combat/Component/NSEnemyAttackComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyCombatComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyCoreComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyTargetComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"

ANSEnemyPawnBase::ANSEnemyPawnBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	CollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->SetCollisionProfileName(NSCollisionProfiles::EnemyCharacter);

	EnemyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("EnemyMesh"));
	EnemyMesh->SetupAttachment(CollisionComponent);
	EnemyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UNSMonsterAttributeSet>(TEXT("AttributeSet"));

	CombatComponent = CreateDefaultSubobject<UNSEnemyCombatComponent>(TEXT("CombatComponent"));
	CoreComponent = CreateDefaultSubobject<UNSEnemyCoreComponent>(TEXT("CoreComponent"));
	PhaseComponent = CreateDefaultSubobject<UNSEnemyPhaseComponent>(TEXT("PhaseComponent"));
	AttackComponent = CreateDefaultSubobject<UNSEnemyAttackComponent>(TEXT("AttackComponent"));
	TargetComponent = CreateDefaultSubobject<UNSEnemyTargetComponent>(TEXT("TargetComponent"));
	ThreatComponent = CreateDefaultSubobject<UNSEnemyThreatComponent>(TEXT("ThreatComponent"));
	StateComponent = CreateDefaultSubobject<UNSEnemyStateComponent>(TEXT("StateComponent"));
}

void ANSEnemyPawnBase::BeginPlay()
{
	Super::BeginPlay();

	if (ASC)
	{
		ASC->InitAbilityActorInfo(this, this);
	}

	if (StateComponent)
	{
		StateComponent->InitState(AttributeSet);
		StateComponent->OnDeadStateChanged.AddUObject(
			this,
			&ThisClass::HandleDeadStateChanged);
	}

	InitializeFromData(true);

	if (CoreComponent)
	{
		CoreComponent->OnEnemyDataChanged.AddUObject(
			this,
			&ThisClass::HandleEnemyDataChanged);
	}
}

UNSEnemyData* ANSEnemyPawnBase::GetEnemyData() const
{
	return CoreComponent ? CoreComponent->GetEnemyData() : nullptr;
}

void ANSEnemyPawnBase::SetCurrentAttackRow(const FNSEnemyAttackRow& InAttackRow)
{
	if (CombatComponent)
	{
		CombatComponent->SetAttackRow(InAttackRow);
	}
}

const FNSEnemyAttackRow* ANSEnemyPawnBase::GetCurrentAttackRow() const
{
	return CombatComponent ? CombatComponent->GetAttackRow() : nullptr;
}

void ANSEnemyPawnBase::ClearCurrentAttackRow()
{
	if (CombatComponent)
	{
		CombatComponent->ClearAttackRow();
	}
}

bool ANSEnemyPawnBase::IsHitReacting() const
{
	return StateComponent && StateComponent->IsHitReacting();
}

FVector ANSEnemyPawnBase::GetAimLocation() const
{
	if (EnemyMesh)
	{
		return EnemyMesh->Bounds.Origin;
	}

	return GetActorLocation();
}

void ANSEnemyPawnBase::SetEnemyData(UNSEnemyData* InEnemyData)
{
	if (!HasAuthority() || !InEnemyData || !CoreComponent)
	{
		return;
	}

	CoreComponent->SetEnemyData(InEnemyData);
}

void ANSEnemyPawnBase::SetDifficultyScale(const FNSDifficultyScale& InScale)
{
	if (CoreComponent)
	{
		CoreComponent->SetDifficultyScale(InScale);
	}
}

void ANSEnemyPawnBase::Die()
{
	if (StateComponent)
	{
		StateComponent->Die();
	}
}

bool ANSEnemyPawnBase::IsDead() const
{
	return StateComponent && StateComponent->IsDead();
}

void ANSEnemyPawnBase::InitializeFromData(bool bFullInit)
{
	UNSEnemyData* EnemyData = GetEnemyData();
	if (!EnemyData)
	{
		return;
	}

	if (CoreComponent)
	{
		CoreComponent->InitializeFromData(
			bFullInit,
			AttributeSet);
	}

	if (bFullInit)
	{
		ApplyVisualData();
	}
}

void ANSEnemyPawnBase::HandleEnemyDataChanged(UNSEnemyData* NewEnemyData)
{
	ApplyVisualData();
}

void ANSEnemyPawnBase::ApplyVisualData()
{
	UNSEnemyData* EnemyData = GetEnemyData();
	if (!EnemyData || !EnemyMesh)
	{
		return;
	}

	if (EnemyData->SkeletalMesh)
	{
		EnemyMesh->SetSkeletalMeshAsset(EnemyData->SkeletalMesh);
	}

	if (EnemyData->AnimClass)
	{
		EnemyMesh->SetAnimInstanceClass(EnemyData->AnimClass);
	}

	SetActorScale3D(EnemyData->DrawScale);
}

void ANSEnemyPawnBase::ApplyDeadState()
{
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	ClearCurrentAttackRow();
}

void ANSEnemyPawnBase::ApplyAliveState()
{
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionProfileName(NSCollisionProfiles::EnemyCharacter);
	}
}

void ANSEnemyPawnBase::HandleDeadStateChanged(bool bDead)
{
	if (bDead)
	{
		ApplyDeadState();
	}
	else
	{
		ApplyAliveState();
	}
}
