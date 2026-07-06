// Copyright 2026 One Team. All rights reserved.

#include "NSTitanWalkerAnimInstance.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "NeoSanctum/Combat/Component/NSEnemyPartComponent.h"
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
	UpdateAim(DeltaSeconds);
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

	PartComponent = IsValid(OwnerPawn)
		                ? OwnerPawn->FindComponentByClass<UNSEnemyPartComponent>()
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

void UNSTitanWalkerAnimInstance::UpdateAim(float DeltaSeconds)
{
	const auto ResetAimValues =
		[this, DeltaSeconds]()
		{
			bUseUpperAim = false;
			bUseWeaponAim = false;

			UpperAimYaw = FMath::FInterpTo(
				UpperAimYaw,
				0.0f,
				DeltaSeconds,
				UpperAimInterpSpeed);

			UpperAimPitch = FMath::FInterpTo(
				UpperAimPitch,
				0.0f,
				DeltaSeconds,
				UpperAimInterpSpeed);

			WeaponAimYaw = FMath::FInterpTo(
				WeaponAimYaw,
				0.0f,
				DeltaSeconds,
				WeaponAimInterpSpeed);

			WeaponAimPitch = FMath::FInterpTo(
				WeaponAimPitch,
				0.0f,
				DeltaSeconds,
				WeaponAimInterpSpeed);
		};

	CurrentAttackId = NAME_None;

	const INSEnemyAgent* EnemyAgentInterface = EnemyAgent.GetInterface();
	const FNSEnemyAttackRow* CurrentAttackRow = EnemyAgentInterface
		                                            ? EnemyAgentInterface->GetCurrentAttackRow()
		                                            : nullptr;

	if (CurrentAttackRow)
	{
		CurrentAttackId = CurrentAttackRow->AttackId;
	}

	if (!IsValid(OwnerPawn) ||
		bIsDead ||
		bIsHitReacting ||
		!CurrentAttackRow ||
		!IsValid(ThreatComponent))
	{
		ResetAimValues();
		return;
	}

	const AActor* TargetActor = ThreatComponent->GetCurrentTarget();
	if (!IsValid(TargetActor))
	{
		ResetAimValues();
		return;
	}

	const FVector AimOrigin = EnemyAgentInterface
		                          ? EnemyAgentInterface->GetAimLocation()
		                          : OwnerPawn->GetActorLocation();

	const FVector TargetLocation = GetTargetAimLocation(TargetActor);
	const FVector ToTarget = TargetLocation - AimOrigin;

	if (ToTarget.IsNearlyZero())
	{
		ResetAimValues();
		return;
	}

	const FRotator WorldAimRotation = ToTarget.Rotation();
	const FRotator ReferenceRotation(
		0.0f,
		OwnerPawn->GetActorRotation().Yaw,
		0.0f);

	const FRotator LocalAimRotation =
		UKismetMathLibrary::NormalizedDeltaRotator(
			WorldAimRotation,
			ReferenceRotation);

	float TargetUpperYaw = 0.0f;
	float TargetUpperPitch = 0.0f;
	float UpperAimSpeed = UpperAimInterpSpeed;

	bUseUpperAim = false;

	if (CurrentAttackRow->bTurnUpper)
	{
		float UpperYawLimit = DefaultUpperYawLimit;
		float UpperPitchLimit = DefaultUpperPitchLimit;

		if (IsValid(PartComponent))
		{
			float PartYawLimit = 0.0f;
			float PartPitchLimit = 0.0f;
			float PartAimSpeed = 0.0f;

			const bool bHasPartAimLimit =
				PartComponent->TryGetAimLimitsByAttackId(
					CurrentAttackRow->AttackId,
					ENSEnemyPartAimRole::UpperBody,
					PartYawLimit,
					PartPitchLimit,
					PartAimSpeed);

			if (bHasPartAimLimit)
			{
				if (PartYawLimit > 0.0f)
				{
					UpperYawLimit = PartYawLimit;
				}

				if (PartPitchLimit > 0.0f)
				{
					UpperPitchLimit = PartPitchLimit;
				}

				if (PartAimSpeed > 0.0f)
				{
					UpperAimSpeed = PartAimSpeed;
				}
			}
		}

		TargetUpperYaw = FMath::Clamp(
			LocalAimRotation.Yaw,
			-UpperYawLimit,
			UpperYawLimit);

		TargetUpperPitch = FMath::Clamp(
			LocalAimRotation.Pitch,
			-UpperPitchLimit,
			UpperPitchLimit);

		bUseUpperAim = true;
	}

	UpperAimYaw = FMath::FInterpTo(
		UpperAimYaw,
		TargetUpperYaw,
		DeltaSeconds,
		UpperAimSpeed);

	UpperAimPitch = FMath::FInterpTo(
		UpperAimPitch,
		TargetUpperPitch,
		DeltaSeconds,
		UpperAimSpeed);

	float TargetWeaponYaw = 0.0f;
	float TargetWeaponPitch = 0.0f;
	float WeaponAimSpeed = WeaponAimInterpSpeed;

	bUseWeaponAim = false;

	if (CurrentAttackRow->bTurnWeapon && IsValid(PartComponent))
	{
		float WeaponYawLimit = 0.0f;
		float WeaponPitchLimit = 0.0f;
		float PartAimSpeed = 0.0f;

		const bool bHasWeaponAim =
			PartComponent->TryGetAimLimitsByAttackId(
				CurrentAttackRow->AttackId,
				ENSEnemyPartAimRole::Weapon,
				WeaponYawLimit,
				WeaponPitchLimit,
				PartAimSpeed);

		if (bHasWeaponAim)
		{
			const float ResidualYaw =
				FRotator::NormalizeAxis(LocalAimRotation.Yaw - UpperAimYaw);

			const float ResidualPitch =
				FRotator::NormalizeAxis(LocalAimRotation.Pitch - UpperAimPitch);

			if (WeaponYawLimit > 0.0f)
			{
				TargetWeaponYaw = FMath::Clamp(
					ResidualYaw,
					-WeaponYawLimit,
					WeaponYawLimit);
			}

			if (WeaponPitchLimit > 0.0f)
			{
				TargetWeaponPitch = FMath::Clamp(
					ResidualPitch,
					-WeaponPitchLimit,
					WeaponPitchLimit);
			}

			if (PartAimSpeed > 0.0f)
			{
				WeaponAimSpeed = PartAimSpeed;
			}

			bUseWeaponAim = true;
		}
	}

	WeaponAimYaw = FMath::FInterpTo(
		WeaponAimYaw,
		TargetWeaponYaw,
		DeltaSeconds,
		WeaponAimSpeed);

	WeaponAimPitch = FMath::FInterpTo(
		WeaponAimPitch,
		TargetWeaponPitch,
		DeltaSeconds,
		WeaponAimSpeed);
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
