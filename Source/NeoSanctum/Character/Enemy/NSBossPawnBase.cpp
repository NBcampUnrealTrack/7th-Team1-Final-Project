// Copyright 2026 One Team. All rights reserved.


#include "NSBossPawnBase.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NeoSanctum/Collision/NSCollisionProfiles.h"
#include "NeoSanctum/Combat/Component/NSBossModeComponent.h"
#include "NeoSanctum/Combat/Component/NSBossTargetComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyAttackComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyCombatComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyCoreComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyTargetComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"

ANSBossPawnBase::ANSBossPawnBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	CollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->SetCollisionProfileName(NSCollisionProfiles::EnemyCharacter);

	BossMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BossMesh"));
	BossMesh->SetupAttachment(CollisionComponent);
	BossMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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
	BossModeComponent = CreateDefaultSubobject<UNSBossModeComponent>(TEXT("BossModeComponent"));
	BossTargetComponent = CreateDefaultSubobject<UNSBossTargetComponent>(TEXT("BossTargetComponent"));
}

void ANSBossPawnBase::BeginPlay()
{
	Super::BeginPlay();

	if (ASC)
	{
		ASC->InitAbilityActorInfo(this, this);
	}

	if (StateComponent)
	{
		StateComponent->InitState(AttributeSet);
		StateComponent->OnDeadStateChanged.AddUObject(this, &ThisClass::HandleDeadStateChanged);
	}

	InitializeFromData(true);

	if (CoreComponent)
	{
		CoreComponent->OnEnemyDataChanged.AddUObject(
			this,
			&ThisClass::HandleEnemyDataChanged);
	}
	
	if (BossModeComponent)
	{
		BossModeComponent->InitializeMode();
	}
}

UNSEnemyData* ANSBossPawnBase::GetEnemyData() const
{
	return CoreComponent ? CoreComponent->GetEnemyData() : nullptr;
}

void ANSBossPawnBase::SetCurrentAttackRow(const FNSEnemyAttackRow& InAttackRow)
{
	if (CombatComponent)
	{
		CombatComponent->SetAttackRow(InAttackRow);
	}
}

const FNSEnemyAttackRow* ANSBossPawnBase::GetCurrentAttackRow() const
{
	return CombatComponent ? CombatComponent->GetAttackRow() : nullptr;
}

void ANSBossPawnBase::ClearCurrentAttackRow()
{
	if (CombatComponent)
	{
		CombatComponent->ClearAttackRow();
	}
}

FVector ANSBossPawnBase::GetAimLocation() const
{
	if (BossMesh)
	{
		return BossMesh->Bounds.Origin;
	}

	return GetActorLocation();
}

void ANSBossPawnBase::SetEnemyData(UNSEnemyData* InEnemyData)
{
	if (!HasAuthority() || !InEnemyData || !CoreComponent)
	{
		return;
	}

	CoreComponent->SetEnemyData(InEnemyData);
}

void ANSBossPawnBase::SetDifficultyScale(const FNSDifficultyScale& InScale)
{
	if (CoreComponent)
	{
		CoreComponent->SetDifficultyScale(InScale);
	}
}

void ANSBossPawnBase::Die()
{
	if (StateComponent)
	{
		StateComponent->Die();
	}
}

void ANSBossPawnBase::InitializeFromData(bool bFullInit)
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

void ANSBossPawnBase::HandleEnemyDataChanged(UNSEnemyData* NewEnemyData)
{
	ApplyVisualData();
}

void ANSBossPawnBase::ApplyVisualData()
{
	UNSEnemyData* EnemyData = GetEnemyData();
	if (!EnemyData || !BossMesh)
	{
		return;
	}

	if (EnemyData->SkeletalMesh)
	{
		BossMesh->SetSkeletalMeshAsset(EnemyData->SkeletalMesh);
	}

	if (EnemyData->AnimClass)
	{
		BossMesh->SetAnimInstanceClass(EnemyData->AnimClass);
	}

	SetActorScale3D(EnemyData->DrawScale);
}

void ANSBossPawnBase::ApplyDeadState()
{
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	if (BossTargetComponent)
	{
		BossTargetComponent->ResetTargets();
	}

	ClearCurrentAttackRow();
}

void ANSBossPawnBase::ApplyAliveState()
{
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionProfileName(NSCollisionProfiles::EnemyCharacter);
	}
}

void ANSBossPawnBase::HandleDeadStateChanged(bool bDead)
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
