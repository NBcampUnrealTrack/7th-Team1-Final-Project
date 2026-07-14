// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"
#include "NeoSanctum/System/Component/NSDissolveComponent.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NeoSanctum/AI/Enemy/Controller/NSEnemyAIController.h"
#include "NeoSanctum/Collision/NSCollisionProfiles.h"
#include "NeoSanctum/Combat/Component/NSEnemyAttackComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyMeleeComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyMoveComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyTargetComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"
#include "NeoSanctum/Combat/HitReaction/NSHitReactionComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/System/Component/NSDamageFlashComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPartComponent.h"
#include "NeoSanctum/System/Minimap/NSMinimapIconComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NeoSanctum/Type/NSBBTypes.h"

ANSEnemyCharacterBase::ANSEnemyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCapsuleComponent()->SetCollisionProfileName(NSCollisionProfiles::EnemyCharacter);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->bUseControllerDesiredRotation = false;
	Movement->RotationRate = FRotator(0.0f, 360.0f, 0.0f);

	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UNSMonsterAttributeSet>(TEXT("AttributeSet"));

	DissolveComponent = CreateDefaultSubobject<UNSDissolveComponent>(TEXT("DissolveComponent"));
	DamageFlashComponent = CreateDefaultSubobject<UNSDamageFlashComponent>(TEXT("DamageFlashComponent"));
	HitReactionComponent = CreateDefaultSubobject<UNSHitReactionComponent>(TEXT("HitReactionComponent"));
	PhaseComponent = CreateDefaultSubobject<UNSEnemyPhaseComponent>(TEXT("PhaseComponent"));
	CoreComponent = CreateDefaultSubobject<UNSEnemyCoreComponent>(TEXT("CoreComponent"));
	AttackComponent = CreateDefaultSubobject<UNSEnemyAttackComponent>(TEXT("AttackComponent"));
	TargetComponent = CreateDefaultSubobject<UNSEnemyTargetComponent>(TEXT("TargetComponent"));
	ThreatComponent = CreateDefaultSubobject<UNSEnemyThreatComponent>(TEXT("ThreatComponent"));
	MeleeComponent = CreateDefaultSubobject<UNSEnemyMeleeComponent>(TEXT("MeleeComponent"));
	MoveComponent = CreateDefaultSubobject<UNSEnemyMoveComponent>(TEXT("MoveComponent"));
	CombatComponent = CreateDefaultSubobject<UNSEnemyCombatComponent>(TEXT("CombatComponent"));
	StateComponent = CreateDefaultSubobject<UNSEnemyStateComponent>(TEXT("StateComponent"));
	PartComponent = CreateDefaultSubobject<UNSEnemyPartComponent>(TEXT("PartComponent"));
	MinimapIconComponent = CreateDefaultSubobject<UNSMinimapIconComponent>(TEXT("MinimapIconComponent"));
	MinimapIconComponent->SetHideWhenOwnerHealthZero(true);

	HitReactionComponent->SetTargetType(ENSHitFeedbackTargetType::Enemy);

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 200.0f;
	GetCharacterMovement()->AvoidanceWeight = 0.5f;
}

void ANSEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (!ASC)
	{
		return;
	}

	ASC->InitAbilityActorInfo(this, this);

	if (StateComponent)
	{
		StateComponent->InitState(AttributeSet);
		StateComponent->OnDeathStarted.AddUObject(this, &ThisClass::HandleDeathStarted);
		StateComponent->OnDeadStateChanged.AddUObject(this, &ThisClass::HandleDeadStateChanged);
		StateComponent->OnInactiveStateChanged.AddUObject(this, &ThisClass::HandleInactiveStateChanged);
		StateComponent->OnHitReactionStateChanged.AddUObject(this, &ThisClass::HandleHitReactionStateChanged);
	}

	InitializeFromData(true);

	if (CoreComponent)
	{
		CoreComponent->OnEnemyDataChanged.AddUObject(
			this,
			&ANSEnemyCharacterBase::HandleEnemyDataChanged);
	}

	if (DissolveComponent && HasAuthority())
	{
		DissolveComponent->OnDissolveComplete.BindUObject(this, &ANSEnemyCharacterBase::OnDissolveFinished);
	}
}

void ANSEnemyCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (!HasAuthority() || !bIsTraversingNavLink)
	{
		return;
	}

	UpdateNavLinkTraversal(DeltaSeconds);
}

void ANSEnemyCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANSEnemyCharacterBase, bHasCombatAimTarget);
	DOREPLIFETIME(ANSEnemyCharacterBase, CombatAimTargetLocation);
	DOREPLIFETIME(ANSEnemyCharacterBase, bIsRetreating);
	DOREPLIFETIME(ANSEnemyCharacterBase, bIsTraversingNavLink);
	DOREPLIFETIME(ANSEnemyCharacterBase, NavLinkDestination);
	DOREPLIFETIME(ANSEnemyCharacterBase, NavLinkTraversalPhase);
}

void ANSEnemyCharacterBase::SetCurrentAttackRow(const FNSEnemyAttackRow& InAttackRow)
{
	if (CombatComponent)
	{
		CombatComponent->SetAttackRow(InAttackRow);
	}
}

const FNSEnemyAttackRow* ANSEnemyCharacterBase::GetCurrentAttackRow() const
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

void ANSEnemyCharacterBase::ClearCurrentAttackRow()
{
	if (CombatComponent)
	{
		CombatComponent->ClearAttackRow();
	}
}

FVector ANSEnemyCharacterBase::GetAimLocation() const
{
	const USkeletalMeshComponent* MeshComponent = GetMesh();
	if (MeshComponent)
	{
		return MeshComponent->Bounds.Origin;
	}

	return GetActorLocation();
}

void ANSEnemyCharacterBase::Die()
{
	if (StateComponent)
	{
		StateComponent->Die();
	}
}

bool ANSEnemyCharacterBase::IsDead() const
{
	return StateComponent && StateComponent->IsDead();
}

bool ANSEnemyCharacterBase::IsInPool() const
{
	return StateComponent && StateComponent->IsInactive();
}

bool ANSEnemyCharacterBase::IsHitReacting() const
{
	return StateComponent && StateComponent->IsHitReacting();
}

void ANSEnemyCharacterBase::HandleEnemyDataChanged(UNSEnemyData* NewEnemyData)
{
	ApplyVisualData();
}

void ANSEnemyCharacterBase::ApplyVisualData()
{
	UNSEnemyData* EnemyData = GetEnemyData();
	if (!EnemyData || !GetMesh())
	{
		return;
	}

	if (EnemyData->SkeletalMesh)
	{
		GetMesh()->SetSkeletalMeshAsset(EnemyData->SkeletalMesh);
		InitializeRuntimeMaterials();
	}

	if (EnemyData->AnimClass)
	{
		GetMesh()->SetAnimInstanceClass(EnemyData->AnimClass);
	}

	SetActorScale3D(EnemyData->DrawScale);
}

void ANSEnemyCharacterBase::InitializeFromData(bool bFullInit)
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

void ANSEnemyCharacterBase::ApplyAliveVisual()
{
	SetActorHiddenInGame(false);

	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionProfileName(NSCollisionProfiles::EnemyCharacter);
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetComponentTickEnabled(true);
		MeshComp->bBlendPhysics = false;
		MeshComp->bPauseAnims = false;
		MeshComp->SetAllBodiesSimulatePhysics(false);
		MeshComp->SetSimulatePhysics(false);
		MeshComp->AttachToComponent(
			GetCapsuleComponent(),
			FAttachmentTransformRules::SnapToTargetIncludingScale);
		MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		MeshComp->SetCollisionProfileName(TEXT("CharacterMesh"));
		MeshComp->bPauseAnims = false;
	}

	if (DissolveComponent)
	{
		DissolveComponent->ResetDissolve();
	}
}

void ANSEnemyCharacterBase::ApplyDeadVisual()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	ClearCurrentAttackRow();
	ClearCombatAimTarget();

	if (!StartDeathRagdoll())
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			MeshComp->bPauseAnims = true;
			MeshComp->SetComponentTickEnabled(false);
		}
	}

	if (DissolveComponent)
	{
		DissolveComponent->StartDissolve(false);
	}
}

bool ANSEnemyCharacterBase::StartDeathRagdoll()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || !MeshComp->GetPhysicsAsset())
	{
		return false;
	}

	if (MeshComp->IsAnySimulatingPhysics())
	{
		return true;
	}

	MeshComp->bPauseAnims = false;
	MeshComp->SetComponentTickEnabled(true);
	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComp->SetAllBodiesSimulatePhysics(true);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->WakeAllRigidBodies();
	MeshComp->bBlendPhysics = true;

	return MeshComp->IsAnySimulatingPhysics();
}

void ANSEnemyCharacterBase::OnDissolveFinished()
{
	if (!HasAuthority())
	{
		return;
	}

	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (GameMode && GameMode->Implements<UNSRunGameModeInterface>())
	{
		INSRunGameModeInterface::Execute_ReturnMonsterToPool(GameMode, this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("풀 매니저 없음, 풀 반환 실패"));
	}
}

void ANSEnemyCharacterBase::UpdateCombatAimTarget(AActor* TargetActor)
{
	if (!HasAuthority() || !IsValid(TargetActor))
	{
		ClearCombatAimTarget();
		return;
	}

	FVector AimBoundsOrigin = TargetActor->GetActorLocation();
	FVector AimBoundsExtent = FVector::ZeroVector;

	if (const UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(TargetActor->GetRootComponent()))
	{
		AimBoundsOrigin = RootPrimitive->Bounds.Origin;
		AimBoundsExtent = RootPrimitive->Bounds.BoxExtent;
	}

	CombatAimTargetLocation = AimBoundsOrigin + FVector::UpVector * (AimBoundsExtent.Z * AimTargetZOffsetRatio);

	bHasCombatAimTarget = true;
}

void ANSEnemyCharacterBase::ClearCombatAimTarget()
{
	if (!HasAuthority())
	{
		return;
	}

	bHasCombatAimTarget = false;
	CombatAimTargetLocation = FVector::ZeroVector;
}

void ANSEnemyCharacterBase::SetEnemyData(UNSEnemyData* InEnemyData)
{
	if (!HasAuthority() || !InEnemyData || !CoreComponent)
	{
		return;
	}

	CoreComponent->SetEnemyData(InEnemyData);
}

void ANSEnemyCharacterBase::PrepareForReuse(
	const FVector& SpawnLocation, 
	const FRotator& SpawnRotation)
{
	if (!HasAuthority())
	{
		return;
	}

	FinishNavLinkTraversal(false);

	if (StateComponent)
	{
		StateComponent->ResetForReuse();
	}

	ClearCurrentAttackRow();
	ClearCombatAimTarget();

	SetRetreating(false);
	SetActorLocationAndRotation(
		SpawnLocation,
		SpawnRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	SetActorTickEnabled(false);
	SetActorEnableCollision(true);

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
	}

	ApplyAliveVisual();

	InitializeFromData(true);

	SpawnDefaultController();
}
void ANSEnemyCharacterBase::DeactivateForPool()
{
	if (!HasAuthority())
	{
		return;
	}

	FinishNavLinkTraversal(false);

	if (StateComponent)
	{
		StateComponent->SetInactive(true);
		StateComponent->FinishHitReaction();
		StateComponent->ResetHitGauge();
	}

	SetRetreating(false);
	ClearCurrentAttackRow();
	ClearCombatAimTarget();

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* Anim = MeshComp->GetAnimInstance())
		{
			Anim->StopAllMontages(0.0f);
		}
	}

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
		AICon->UnPossess();
		AICon->Destroy();
	}

	if (ASC)
	{
		ASC->CancelAllAbilities();
		ASC->RemoveActiveEffects(FGameplayEffectQuery());
		ASC->ClearAllAbilities();
	}

	if (PartComponent)
	{
		PartComponent->UnEquipParts();
	}

	if (DamageFlashComponent)
	{
		DamageFlashComponent->CancelFlash();
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void ANSEnemyCharacterBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (!bNavLinkJumping)
	{
		return;
	}

	bNavLinkJumping = false;

	if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		const FVector Here = GetActorLocation();
		FNavLocation NavLoc;

		if (NavSys->ProjectPointToNavigation(Here, NavLoc, FVector(100.0f, 100.0f, 200.0f)))
		{
			const UCapsuleComponent* Capsule = GetCapsuleComponent();
			const float Radius = Capsule ? Capsule->GetScaledCapsuleRadius() : 50.0f;
			const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 88.0f;

			if (FVector::DistSquared2D(Here, NavLoc.Location) > FMath::Square(Radius))
			{
				SetActorLocation(
					NavLoc.Location + FVector(0.0f, 0.0f, HalfHeight),
					false);
			}
		}
	}

	if (bIsTraversingNavLink)
	{
		FinishNavLinkTraversal(true);
	}
}

void ANSEnemyCharacterBase::StartNavLinkJump(const FVector& DestPoint)
{
	if (!HasAuthority())
	{
		return;
	}

	StartNavLinkTraversal(
		DestPoint,
		FNSNavLinkTraversalFinishedDelegate());
}

bool ANSEnemyCharacterBase::StartNavLinkTraversal(
	const FVector& DestPoint,
	FNSNavLinkTraversalFinishedDelegate OnTraversalFinished)
{
	if (!HasAuthority() || IsDead() || IsInPool() || IsHitReacting())
	{
		return false;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return false;
	}

	if (bIsTraversingNavLink)
	{
		FinishNavLinkTraversal(false);
	}

	bIsTraversingNavLink = true;
	bNavLinkJumping = false;
	NavLinkDestination = DestPoint;
	NavLinkTraversalPhase = ENSNavLinkTraversalPhase::Rotating;
	NavLinkRotationElapsed = 0.0f;
	NavLinkTraversalFinishedDelegate = OnTraversalFinished;

	bCachedNavLinkOrientRotationToMovement = Movement->bOrientRotationToMovement;
	bCachedNavLinkUseControllerDesiredRotation = Movement->bUseControllerDesiredRotation;

	Movement->StopMovementImmediately();
	Movement->bOrientRotationToMovement = false;
	Movement->bUseControllerDesiredRotation = false;

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	ClearCombatAimTarget();

	FVector Direction = NavLinkDestination - GetActorLocation();
	Direction.Z = 0.0f;

	if (Direction.IsNearlyZero())
	{
		NavLinkTargetRotation = GetActorRotation();
		StartNavLinkJumpAfterRotation();
		return true;
	}

	NavLinkTargetRotation = Direction.Rotation();
	NavLinkTargetRotation.Pitch = 0.0f;
	NavLinkTargetRotation.Roll = 0.0f;

	RefreshNavLinkTraversalBlackboard();

	const float CurrentYaw = GetActorRotation().Yaw;
	const float TargetYaw = NavLinkTargetRotation.Yaw;
	const float YawDifference = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw));

	if (YawDifference <= NavLinkRotationAcceptableYaw)
	{
		SetActorRotation(NavLinkTargetRotation);
		StartNavLinkJumpAfterRotation();
		return true;
	}

	SetActorTickEnabled(true);
	return true;
}

void ANSEnemyCharacterBase::UpdateNavLinkTraversal(float DeltaSeconds)
{
	if (NavLinkTraversalPhase != ENSNavLinkTraversalPhase::Rotating)
	{
		return;
	}

	NavLinkRotationElapsed += DeltaSeconds;

	const FRotator CurrentRotation = GetActorRotation();
	const FRotator NewRotation = FMath::RInterpConstantTo(
		CurrentRotation,
		NavLinkTargetRotation,
		DeltaSeconds,
		NavLinkRotationSpeed);

	SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));

	const float YawDifference = FMath::Abs(
		FMath::FindDeltaAngleDegrees(
			NewRotation.Yaw,
			NavLinkTargetRotation.Yaw));

	if (YawDifference <= NavLinkRotationAcceptableYaw || NavLinkRotationElapsed >= NavLinkRotationTimeout)
	{
		SetActorRotation(NavLinkTargetRotation);
		StartNavLinkJumpAfterRotation();
	}
}

void ANSEnemyCharacterBase::StartNavLinkJumpAfterRotation()
{
	if (!bIsTraversingNavLink)
	{
		return;
	}

	SetActorTickEnabled(false);

	NavLinkTraversalPhase = ENSNavLinkTraversalPhase::Jumping;
	RefreshNavLinkTraversalBlackboard();

	if (!ExecuteNavLinkJump(NavLinkDestination))
	{
		FinishNavLinkTraversal(true);
	}
}

bool ANSEnemyCharacterBase::ExecuteNavLinkJump(const FVector& DestPoint)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return false;
	}

	FVector LaunchVelocity = FVector::ZeroVector;
	const bool bFoundVelocity =
		UGameplayStatics::SuggestProjectileVelocity_CustomArc(
			this,
			LaunchVelocity,
			GetActorLocation(),
			DestPoint,
			Movement->GetGravityZ(),
			0.5f);

	if (!bFoundVelocity)
	{
		return false;
	}

	bNavLinkJumping = true;
	LaunchCharacter(LaunchVelocity, true, true);

	return true;
}

void ANSEnemyCharacterBase::FinishNavLinkTraversal(bool bNotifyPathFollowing)
{
	const bool bHadTraversalState =
		bIsTraversingNavLink ||
		NavLinkTraversalPhase != ENSNavLinkTraversalPhase::None ||
		NavLinkTraversalFinishedDelegate.IsBound();

	if (!bHadTraversalState)
	{
		return;
	}

	SetActorTickEnabled(false);

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = bCachedNavLinkOrientRotationToMovement;
		Movement->bUseControllerDesiredRotation = bCachedNavLinkUseControllerDesiredRotation;
	}

	bIsTraversingNavLink = false;
	bNavLinkJumping = false;
	NavLinkDestination = FVector::ZeroVector;
	NavLinkTraversalPhase = ENSNavLinkTraversalPhase::None;
	NavLinkRotationElapsed = 0.0f;
	NavLinkTargetRotation = FRotator::ZeroRotator;

	RefreshNavLinkTraversalBlackboard();

	FNSNavLinkTraversalFinishedDelegate FinishedDelegate = NavLinkTraversalFinishedDelegate;
	NavLinkTraversalFinishedDelegate.Unbind();

	if (bNotifyPathFollowing && FinishedDelegate.IsBound())
	{
		FinishedDelegate.Execute();
	}
}

void ANSEnemyCharacterBase::RefreshNavLinkTraversalBlackboard()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
	{
		return;
	}

	UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
	if (!Blackboard)
	{
		return;
	}

	Blackboard->SetValueAsBool(
		NSBB::Movement::IsTraversingNavLink,
		bIsTraversingNavLink);

	Blackboard->SetValueAsEnum(
		NSBB::Movement::NavLinkTraversalPhase,
		static_cast<uint8>(NavLinkTraversalPhase));

	if (bIsTraversingNavLink)
	{
		Blackboard->SetValueAsVector(
			NSBB::Movement::NavLinkDestination,
			NavLinkDestination);
	}
	else
	{
		Blackboard->ClearValue(NSBB::Movement::NavLinkDestination);
	}
}

void ANSEnemyCharacterBase::SetRetreating(bool bInRetreating)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsRetreating = bInRetreating;
}

float ANSEnemyCharacterBase::GetHitGauge() const
{
	return StateComponent ? StateComponent->GetHitGauge() : 0.0f;
}

float ANSEnemyCharacterBase::GetMaxHitGauge() const
{
	return StateComponent ? StateComponent->GetMaxHitGauge() : 0.0f;
}

void ANSEnemyCharacterBase::ResetHitGauge()
{
	if (StateComponent)
	{
		StateComponent->ResetHitGauge();
	}
}

void ANSEnemyCharacterBase::FinishHitReaction()
{
	if (StateComponent)
	{
		StateComponent->FinishHitReaction();
	}
}

void ANSEnemyCharacterBase::HandleDeathStarted()
{
	if (!HasAuthority())
	{
		return;
	}

	FinishNavLinkTraversal(false);

	SetRetreating(false);
	ClearCombatAimTarget();

	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (GameMode && GameMode->Implements<UNSRunGameModeInterface>())
	{
		INSRunGameModeInterface::Execute_NotifyEnemyKilled(GameMode, this);
	}

	OnEnemyDead.Broadcast();

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->UnPossess();
		AIController->Destroy();
	}
}

void ANSEnemyCharacterBase::HandleDeadStateChanged(bool bDead)
{
	if (bDead)
	{
		ApplyDeadVisual();
		return;
	}

	ApplyAliveVisual();
}

void ANSEnemyCharacterBase::HandleInactiveStateChanged(bool bInactive)
{
	SetActorHiddenInGame(bInactive);
	SetActorEnableCollision(!bInactive);
}

void ANSEnemyCharacterBase::HandleHitReactionStateChanged(bool bHitReacting)
{
	if (!HasAuthority())
	{
		return;
	}

	ANSEnemyAIController* EnemyController = Cast<ANSEnemyAIController>(GetController());
	if (!EnemyController)
	{
		return;
	}

	if (bHitReacting)
	{
		EnemyController->HandleHitReactionStarted();
	}
	else
	{
		EnemyController->HandleHitReactionFinished();
	}
}

void ANSEnemyCharacterBase::InitializeRuntimeMaterials()
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	UNSEnemyData* EnemyData = GetEnemyData();

	if (!MeshComponent || !EnemyData)
	{
		return;
	}

	if (DamageFlashComponent)
	{
		DamageFlashComponent->ClearMaterialFlashTargets();
	}

	RuntimeVisualMaterials.Reset();

	TArray<UMaterialInstanceDynamic*> FlashTargets;

	for (const FNSEnemyMaterialDefinition& Definition : EnemyData->MaterialDefinitions)
	{
		const int32 MaterialIndex = MeshComponent->GetMaterialIndex(Definition.MaterialSlotName);

		if (MaterialIndex == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy material slot not found: %s"),
			       *Definition.MaterialSlotName.ToString());

			continue;
		}

		UMaterialInterface* InitialMaterial = Definition.InitialMaterial
			                                      ? Definition.InitialMaterial.Get()
			                                      : MeshComponent->GetMaterial(MaterialIndex);

		if (!InitialMaterial)
		{
			continue;
		}

		MeshComponent->SetMaterial(MaterialIndex, InitialMaterial);

		UMaterialInstanceDynamic* MID =
			MeshComponent->CreateDynamicMaterialInstance(MaterialIndex, InitialMaterial);

		if (!MID)
		{
			continue;
		}

		MID->SetVectorParameterValue(TEXT("MonsterTint"), Definition.MonsterTint);
		MID->SetScalarParameterValue(TEXT("HitFlashAmount"), 0.0f);

		RuntimeVisualMaterials.Add(MID);
		FlashTargets.Add(MID);
	}

	if (DamageFlashComponent)
	{
		DamageFlashComponent->SetMaterialFlashTargets(FlashTargets);
	}
}
