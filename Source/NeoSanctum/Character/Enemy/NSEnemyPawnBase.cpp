// Copyright 2026 One Team. All rights reserved.

#include "NSEnemyPawnBase.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Collision/NSCollisionProfiles.h"
#include "NeoSanctum/Combat/Component/NSEnemyAttackComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyCombatComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyCoreComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPartComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyTargetComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"
#include "NeoSanctum/System/Component/NSDissolveComponent.h"
#include "NeoSanctum/System/Minimap/NSMinimapIconComponent.h"
#include "NeoSanctum/Combat/Cosmetic/NSEnemyVisualMaterialApplier.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"

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
	PartComponent = CreateDefaultSubobject<UNSEnemyPartComponent>(TEXT("PartComponent"));
	DissolveComponent = CreateDefaultSubobject<UNSDissolveComponent>(TEXT("DissolveComponent"));
	MinimapIconComponent = CreateDefaultSubobject<UNSMinimapIconComponent>(TEXT("MinimapIconComponent"));
	MinimapIconComponent->SetHideWhenOwnerHealthZero(true);
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
		StateComponent->OnDeathStarted.AddUObject(
			this,
			&ThisClass::HandleDeathStarted);
		StateComponent->OnHitReactionStateChanged.AddUObject(
			this,
			&ThisClass::HandleHitReactionStateChanged);
	}

	InitializeFromData(true);
	ConfigureHurtCollision();

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
	if (!CombatComponent)
	{
		return nullptr;
	}

	if (const FNSEnemyAttackRow* AttackRow = CombatComponent->GetAttackRow())
	{
		return AttackRow;
	}

	const FName AttackId = CombatComponent->GetCurrentAttackId();
	if (AttackId.IsNone())
	{
		return nullptr;
	}

	const UNSEnemyData* EnemyData = GetEnemyData();
	if (!EnemyData)
	{
		return nullptr;
	}

	for (const FNSEnemyAttackRow* AttackRow : EnemyData->GetAttackRows())
	{
		if (AttackRow && AttackRow->AttackId == AttackId)
		{
			return AttackRow;
		}
	}

	return nullptr;
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

bool ANSEnemyPawnBase::IsDead() const
{
	return StateComponent && StateComponent->IsDead();
}

float ANSEnemyPawnBase::ResolvePhysicsAssetDamageMultiplier(const FHitResult& HitResult) const
{
	if (!bUsePhysicsAssetHurtCollision || HitResult.GetComponent() != EnemyMesh)
	{
		return 1.0f;
	}

	if (const float* ExactMultiplier = ExactBoneDamageMultipliers.Find(HitResult.BoneName))
	{
		return FMath::Max(*ExactMultiplier, 0.0f);
	}

	const FString HitBoneName = HitResult.BoneName.ToString();
	for (const TPair<FName, float>& PrefixRule : BonePrefixDamageMultipliers)
	{
		if (HitBoneName.StartsWith(PrefixRule.Key.ToString()))
		{
			return FMath::Max(PrefixRule.Value, 0.0f);
		}
	}

	return FMath::Max(DefaultPhysicsAssetDamageMultiplier, 0.0f);
}

void ANSEnemyPawnBase::ConfigureHurtCollision()
{
	const ECollisionResponse RootHitResponse =
		bUsePhysicsAssetHurtCollision ? ECR_Ignore : ECR_Block;

	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionResponseToChannel(
			NSCollisionChannels::PlayerWeaponTrace,
			RootHitResponse);
	}

	if (!EnemyMesh)
	{
		return;
	}

	if (!bUsePhysicsAssetHurtCollision)
	{
		EnemyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		EnemyMesh->SetGenerateOverlapEvents(false);
		return;
	}

	EnemyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EnemyMesh->SetCollisionObjectType(NSCollisionChannels::Enemy);
	EnemyMesh->SetGenerateOverlapEvents(false);
	EnemyMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	EnemyMesh->SetCollisionResponseToChannel(NSCollisionChannels::PlayerWeaponTrace, ECR_Block);
}

FName ANSEnemyPawnBase::GetAliveCollisionProfileName() const
{
	return NSCollisionProfiles::EnemyCharacter;
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

		if (PartComponent)
		{
			PartComponent->EquipParts();
		}
	}
}

void ANSEnemyPawnBase::HandleEnemyDataChanged(UNSEnemyData* NewEnemyData)
{
	ApplyVisualData();
}

void ANSEnemyPawnBase::HandleHitReactionStateChanged(bool bHitReacting)
{
}

void ANSEnemyPawnBase::ApplyVisualData()
{
	UNSEnemyData* EnemyData = GetEnemyData();
	if (!EnemyData || !EnemyMesh)
	{
		return;
	}

	EnemyMesh->SetRelativeLocation(EnemyMeshRelativeLocation);
	EnemyMesh->SetRelativeRotation(EnemyMeshRelativeRotation);

	if (EnemyData->SkeletalMesh)
	{
		EnemyMesh->SetSkeletalMeshAsset(EnemyData->SkeletalMesh);
	}

	InitializeRuntimeMaterials();

	if (EnemyData->AnimClass)
	{
		EnemyMesh->SetAnimInstanceClass(EnemyData->AnimClass);
	}

	SetActorScale3D(EnemyData->DrawScale);
	ConfigureHurtCollision();
}

void ANSEnemyPawnBase::ApplyDeadState()
{
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	if (EnemyMesh)
	{
		EnemyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	ClearCurrentAttackRow();
}

void ANSEnemyPawnBase::ApplyAliveState()
{
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionProfileName(GetAliveCollisionProfileName());
	}
	
	ConfigureHurtCollision();

	if (DissolveComponent)
	{
		DissolveComponent->ResetDissolve();
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

void ANSEnemyPawnBase::HandleDeathStarted()
{
	if (!HasAuthority())
	{
		return;
	}

	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (!GameMode || !GameMode->Implements<UNSRunGameModeInterface>())
	{
		NS_ACTOR_LOG(this, LogNS, Warning,
			"GameMode가 NSRunGameModeInterface를 구현하지 않아 사망 알림을 보내지 못했습니다. GameMode={GameMode}",
			("GameMode", GetNameSafe(GameMode))
		);
		return;
	}

	NS_ACTOR_LOG(this, LogNS, Log, "GameMode에 Pawn 사망을 알립니다.");
	AController* Killer = StateComponent ? StateComponent->GetLastKiller() : nullptr;
	INSRunGameModeInterface::Execute_NotifyEnemyKilled(GameMode, this, Killer);
}

void ANSEnemyPawnBase::InitializeRuntimeMaterials()
{
	UNSEnemyData* EnemyData = GetEnemyData();
	if (!EnemyData || !EnemyMesh)
	{
		return;
	}

	// Pawn 기반 몬스터는 현재 피격 플래시 대상 등록을 사용하지 않으므로 빈 배열로 받는 변수
	TArray<UMaterialInstanceDynamic*> FlashTargets;

	FNSEnemyVisualMaterialApplier::ApplyEnemyVisualMaterials(
		this,
		EnemyMesh,
		EnemyData,
		RuntimeVisualMaterials,
		FlashTargets);
}
