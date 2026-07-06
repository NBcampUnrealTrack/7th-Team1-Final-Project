// Copyright 2026 One Team. All rights reserved.

#include "NSTitanWalkerMoveComponent.h"

#include "GameFramework/Pawn.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"

UNSTitanWalkerMoveComponent::UNSTitanWalkerMoveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UNSTitanWalkerMoveComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeUpdatedComponent();

	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->SetReplicateMovement(true);
	}
}

void UNSTitanWalkerMoveComponent::InitializeUpdatedComponent()
{
	if (UpdatedComponent)
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	SetUpdatedComponent(OwnerPawn->GetRootComponent());
}

void UNSTitanWalkerMoveComponent::SetMoveTarget(AActor* InTargetActor)
{
	MoveTargetActor = InTargetActor;
}

void UNSTitanWalkerMoveComponent::StopMove()
{
	MoveTargetActor.Reset();
	ClearMoveVelocity();
}

void UNSTitanWalkerMoveComponent::TickMove(float DeltaSeconds)
{
	if (DeltaSeconds <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}

	InitializeUpdatedComponent();

	AActor* TargetActor = MoveTargetActor.Get();
	if (!UpdatedComponent || !IsValid(TargetActor) || !CanUpdateMovement())
	{
		ClearMoveVelocity();
		return;
	}

	FVector ToTarget = TargetActor->GetActorLocation() - UpdatedComponent->GetComponentLocation();
	ToTarget.Z = 0.0f;

	const float DistanceToTarget = ToTarget.Size();
	if (DistanceToTarget <= UE_KINDA_SMALL_NUMBER)
	{
		ClearMoveVelocity();
		return;
	}

	const FVector DirectionToTarget = ToTarget / DistanceToTarget;

	if (!CanMoveBody())
	{
		ClearMoveVelocity();
		return;
	}

	Velocity = CalculateMoveVelocity(DirectionToTarget, DistanceToTarget);

	if (Velocity.IsNearlyZero())
	{
		ClearMoveVelocity();
		return;
	}

	FHitResult Hit;
	const FVector DeltaMove = Velocity * DeltaSeconds;

	SafeMoveUpdatedComponent(
		DeltaMove,
		UpdatedComponent->GetComponentQuat(),
		true,
		Hit);

	if (Hit.IsValidBlockingHit())
	{
		SlideAlongSurface(
			DeltaMove,
			1.0f - Hit.Time,
			Hit.Normal,
			Hit);
	}
}

bool UNSTitanWalkerMoveComponent::CanUpdateMovement() const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	const UNSEnemyStateComponent* StateComponent =
		OwnerActor->FindComponentByClass<UNSEnemyStateComponent>();

	if (StateComponent &&
		(StateComponent->IsDead() ||
			StateComponent->IsInactive() ||
			StateComponent->IsHitReacting()))
	{
		return false;
	}

	const UNSEnemyPhaseComponent* PhaseComponent =
		OwnerActor->FindComponentByClass<UNSEnemyPhaseComponent>();

	if (PhaseComponent && PhaseComponent->IsPatternLocked())
	{
		return false;
	}

	return true;
}

bool UNSTitanWalkerMoveComponent::CanMoveBody() const
{
	const FNSEnemyAttackRow* AttackRow = GetCurrentAttackRow();
	return !AttackRow || AttackRow->bMove;
}

bool UNSTitanWalkerMoveComponent::CanTurnActorForMove(
	float DistanceToTarget,
	const FVector& MoveVelocity) const
{
	if (MoveVelocity.IsNearlyZero())
	{
		return false;
	}

	// 공격 실행 중에는 타깃 조준을 Control Rig의 Body Bone이 담당하므로 Actor Root를 회전하지 않음
	if (GetCurrentAttackRow())
	{
		return false;
	}

	const float MaxDistance = DesiredDistance + DistanceTolerance;

	// 타깃에게 접근하는 이동일 때만 Actor Root 회전을 허용함
	if (DistanceToTarget <= MaxDistance)
	{
		return false;
	}

	return TurnSpeed > 0.0f;
}

const FNSEnemyAttackRow* UNSTitanWalkerMoveComponent::GetCurrentAttackRow() const
{
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(GetOwner());
	return EnemyAgent ? EnemyAgent->GetCurrentAttackRow() : nullptr;
}

void UNSTitanWalkerMoveComponent::UpdateActorRotation(
	const FVector& MoveDirection,
	float DeltaSeconds)
{
	if (!UpdatedComponent || MoveDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRotation = UpdatedComponent->GetComponentRotation();
	const FRotator TargetRotation(0.0f, MoveDirection.Rotation().Yaw, 0.0f);

	const FRotator NewRotation = TurnSpeed > 0.0f
		                             ? FMath::RInterpConstantTo(
			                             CurrentRotation,
			                             TargetRotation,
			                             DeltaSeconds,
			                             TurnSpeed)
		                             : TargetRotation;

	MoveUpdatedComponent(
		FVector::ZeroVector,
		NewRotation,
		false);
}

FVector UNSTitanWalkerMoveComponent::CalculateMoveVelocity(
	const FVector& DirectionToTarget,
	float DistanceToTarget) const
{
	const float MinDistance = FMath::Max(0.0f, DesiredDistance - DistanceTolerance);
	const float MaxDistance = DesiredDistance + DistanceTolerance;

	if (DistanceToTarget > MaxDistance)
	{
		return DirectionToTarget * MoveSpeed;
	}

	if (bAllowBackwardMove && DistanceToTarget < MinDistance)
	{
		return -DirectionToTarget * MoveSpeed;
	}

	return FVector::ZeroVector;
}

void UNSTitanWalkerMoveComponent::ClearMoveVelocity()
{
	Velocity = FVector::ZeroVector;
}
