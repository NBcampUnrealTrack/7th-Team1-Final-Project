// Copyright 2026 One Team. All rights reserved.

#include "NSTitanWalkerAnimInstance.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"

void UNSTitanWalkerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CacheTitanWalker();
}

void UNSTitanWalkerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!IsValid(OwnerPawn) || !IsValid(ThreatComponent))
	{
		CacheTitanWalker();
	}

	UpdateLocomotion();
	UpdateTurn(DeltaSeconds);
	UpdateUpperAim(DeltaSeconds);
}

void UNSTitanWalkerAnimInstance::CacheTitanWalker()
{
	if (!IsValid(OwnerPawn) || !EnemyAgent.GetObject())
	{
		CacheOwner();
	}

	ThreatComponent = IsValid(OwnerPawn)
		                  ? OwnerPawn->FindComponentByClass<UNSEnemyThreatComponent>()
		                  : nullptr;

	if (IsValid(OwnerPawn) && !bHasLastActorYaw)
	{
		LastActorYaw = OwnerPawn->GetActorRotation().Yaw;
		bHasLastActorYaw = true;
	}
}

void UNSTitanWalkerAnimInstance::UpdateLocomotion()
{
	if (!IsValid(OwnerPawn) || bIsDead)
	{
		MoveSpeed = 0.0f;
		bIsMoving = false;
		return;
	}

	MoveSpeed = OwnerPawn->GetVelocity().Size2D();
	bIsMoving = MoveSpeed > MovingSpeedThreshold;
}

void UNSTitanWalkerAnimInstance::UpdateTurn(float DeltaSeconds)
{
	if (!IsValid(OwnerPawn) || bIsDead || DeltaSeconds <= UE_KINDA_SMALL_NUMBER)
	{
		TurnYaw = FMath::FInterpTo(TurnYaw, 0.0f, DeltaSeconds, TurnInterpSpeed);
		return;
	}

	const float CurrentYaw = OwnerPawn->GetActorRotation().Yaw;

	if (!bHasLastActorYaw)
	{
		LastActorYaw = CurrentYaw;
		bHasLastActorYaw = true;
		return;
	}

	const float DeltaYaw = FRotator::NormalizeAxis(CurrentYaw - LastActorYaw);
	const float TargetTurnYaw = DeltaYaw / DeltaSeconds;

	TurnYaw = FMath::FInterpTo(
		TurnYaw,
		TargetTurnYaw,
		DeltaSeconds,
		TurnInterpSpeed);

	LastActorYaw = CurrentYaw;
}

void UNSTitanWalkerAnimInstance::UpdateUpperAim(float DeltaSeconds)
{
	float TargetYaw = 0.0f;
	float TargetPitch = 0.0f;

	const INSEnemyAgent* EnemyAgentInterface = EnemyAgent.GetInterface();
	const FNSEnemyAttackRow* CurrentAttackRow = EnemyAgentInterface
		                                            ? EnemyAgentInterface->GetCurrentAttackRow()
		                                            : nullptr;

	const bool bCanTurnUpper =
		!bIsDead &&
		!bIsHitReacting &&
		CurrentAttackRow &&
		CurrentAttackRow->bTurnUpper;

	if (bCanTurnUpper && IsValid(OwnerPawn) && IsValid(ThreatComponent))
	{
		const AActor* TargetActor = ThreatComponent->GetCurrentTarget();

		if (IsValid(TargetActor))
		{
			const FVector AimOrigin = EnemyAgentInterface
				                          ? EnemyAgentInterface->GetAimLocation()
				                          : OwnerPawn->GetActorLocation();

			const FVector TargetLocation = GetTargetAimLocation(TargetActor);
			const FVector ToTarget = TargetLocation - AimOrigin;

			if (!ToTarget.IsNearlyZero())
			{
				const FRotator WorldAimRotation = ToTarget.Rotation();
				const FRotator ReferenceRotation(
					0.0f,
					OwnerPawn->GetActorRotation().Yaw,
					0.0f);

				const FRotator LocalAimRotation =
					UKismetMathLibrary::NormalizedDeltaRotator(
						WorldAimRotation,
						ReferenceRotation);

				const float YawLimit = CurrentAttackRow->YawLimit > 0.0f
					                       ? CurrentAttackRow->YawLimit
					                       : DefaultUpperYawLimit;

				const float PitchLimit = CurrentAttackRow->PitchLimit > 0.0f
					                         ? CurrentAttackRow->PitchLimit
					                         : DefaultUpperPitchLimit;

				TargetYaw = FMath::Clamp(
					LocalAimRotation.Yaw,
					-YawLimit,
					YawLimit);

				TargetPitch = FMath::Clamp(
					LocalAimRotation.Pitch,
					-PitchLimit,
					PitchLimit);
			}
		}
	}

	UpperAimYaw = FMath::FInterpTo(
		UpperAimYaw,
		TargetYaw,
		DeltaSeconds,
		UpperAimInterpSpeed);

	UpperAimPitch = FMath::FInterpTo(
		UpperAimPitch,
		TargetPitch,
		DeltaSeconds,
		UpperAimInterpSpeed);
}

FVector UNSTitanWalkerAnimInstance::GetTargetAimLocation(const AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return FVector::ZeroVector;
	}

	FVector Origin = TargetActor->GetActorLocation();
	FVector Extent = FVector::ZeroVector;

	if (const UPrimitiveComponent* PrimitiveComponent =
		Cast<UPrimitiveComponent>(TargetActor->GetRootComponent()))
	{
		Origin = PrimitiveComponent->Bounds.Origin;
		Extent = PrimitiveComponent->Bounds.BoxExtent;
	}

	return Origin + FVector::UpVector * (Extent.Z * AimZRatio);
}
