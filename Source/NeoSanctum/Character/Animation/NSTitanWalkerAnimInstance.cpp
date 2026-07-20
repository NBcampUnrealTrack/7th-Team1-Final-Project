// Copyright 2026 One Team. All rights reserved.

#include "NSTitanWalkerAnimInstance.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/Combat/Component/NSEnemyPartComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyCombatComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"

namespace
{
	float InterpAngleDegrees(
		float Current,
		float Target,
		float DeltaSeconds,
		float InterpSpeed)
	{
		if (InterpSpeed <= 0.0f || DeltaSeconds <= UE_KINDA_SMALL_NUMBER)
		{
			return FRotator::NormalizeAxis(Target);
		}

		const float DeltaAngle = FMath::FindDeltaAngleDegrees(Current, Target);
		const float Alpha = FMath::Clamp(DeltaSeconds * InterpSpeed, 0.0f, 1.0f);

		return FRotator::NormalizeAxis(Current + DeltaAngle * Alpha);
	}
}

void UNSTitanWalkerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CacheTitanWalker();
}

void UNSTitanWalkerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!IsValid(OwnerPawn) || !IsValid(ThreatComponent) || !IsValid(CombatComponent))
	{
		CacheTitanWalker();
	}

	UpdateLocomotion();
	UpdateTurn(DeltaSeconds);
	UpdateAim(DeltaSeconds);
	UpdateControlRigBlend(DeltaSeconds);
	UpdateCombatPoseHold();
}

bool UNSTitanWalkerAnimInstance::IsAimAligned(float ToleranceDegrees) const
{
	const float SafeTolerance = FMath::Max(ToleranceDegrees, 0.0f);

	float MaxError = 0.0f;

	if (bUseUpperAim)
	{
		MaxError = FMath::Max(
			MaxError,
			FMath::Abs(FMath::FindDeltaAngleDegrees(UpperAimYaw, TargetUpperYaw)));

		MaxError = FMath::Max(
			MaxError,
			FMath::Abs(UpperAimPitch - TargetUpperPitch));
	}

	if (bUseWeaponAim)
	{
		MaxError = FMath::Max(
			MaxError,
			FMath::Abs(FMath::FindDeltaAngleDegrees(WeaponAimYaw, TargetWeaponYaw)));

		MaxError = FMath::Max(
			MaxError,
			FMath::Abs(WeaponAimPitch - TargetWeaponPitch));
	}

	return MaxError <= SafeTolerance;
}

void UNSTitanWalkerAnimInstance::ResetCombatAimImmediate()
{
	ResetAimImmediate();
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

	CombatComponent = IsValid(OwnerPawn)
		                  ? OwnerPawn->FindComponentByClass<UNSEnemyCombatComponent>()
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

		RawAimYaw = 0.0f;
		RawAimPitch = 0.0f;
		TargetUpperYaw = 0.0f;
		TargetUpperPitch = 0.0f;
		TargetWeaponYaw = 0.0f;
		TargetWeaponPitch = 0.0f;

		UpperAimYaw = InterpAngleDegrees(UpperAimYaw, 0.0f, DeltaSeconds, UpperAimInterpSpeed);
		UpperAimPitch = FMath::FInterpTo(UpperAimPitch, 0.0f, DeltaSeconds, UpperAimInterpSpeed);

		WeaponAimYaw = InterpAngleDegrees(WeaponAimYaw, 0.0f, DeltaSeconds, WeaponAimInterpSpeed);
		WeaponAimPitch = FMath::FInterpTo(WeaponAimPitch, 0.0f, DeltaSeconds, WeaponAimInterpSpeed);
	};

	const INSEnemyAgent* EnemyAgentInterface = EnemyAgent.GetInterface();
	const FNSEnemyAttackRow* CurrentAttackRow = EnemyAgentInterface
		                                            ? EnemyAgentInterface->GetCurrentAttackRow()
		                                            : nullptr;

	bHasCurrentAttackRow = CurrentAttackRow != nullptr;

	if (CurrentAttackRow)
	{
		CurrentAttackId = CurrentAttackRow->AttackId;
	}

	if (!IsValid(OwnerPawn) || bIsDead || !IsValid(ThreatComponent))
	{
		ResetAimValues();
		return;
	}

	if (bIsHitReacting)
	{
		ResetAimImmediate();
		return;
	}

	FVector TargetLocation = FVector::ZeroVector;
	bool bHasAimTargetLocation = false;

	if (IsValid(CombatComponent))
	{
		bHasAimTargetLocation = CombatComponent->TryGetReplicatedAimTargetLocation(TargetLocation);
	}

	if (!bHasAimTargetLocation)
	{
		const AActor* TargetActor = ThreatComponent->GetCurrentTarget();
		if (!IsValid(TargetActor))
		{
			ResetAimValues();
			return;
		}

		TargetLocation = GetTargetAimLocation(TargetActor);
		bHasAimTargetLocation = true;
	}

	if (!bHasAimTargetLocation)
	{
		ResetAimValues();
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		LastTargetSeenTime = World->GetTimeSeconds();
	}

	const FVector AimOrigin = EnemyAgentInterface
		                          ? EnemyAgentInterface->GetAimLocation()
		                          : OwnerPawn->GetActorLocation();

	const FVector ToTarget = TargetLocation - AimOrigin;

	if (ToTarget.IsNearlyZero())
	{
		ResetAimValues();
		return;
	}

	FVector ToTargetFlat = ToTarget;
	ToTargetFlat.Z = 0.0f;

	if (!ToTargetFlat.Normalize())
	{
		ResetAimValues();
		return;
	}

	const float ReferenceYaw = OwnerPawn->GetActorRotation().Yaw;

	const FVector ReferenceForward =
		FRotationMatrix(FRotator(0.0f, ReferenceYaw, 0.0f))
		.GetUnitAxis(EAxis::X)
		.GetSafeNormal2D();

	const float Dot = FVector::DotProduct(ReferenceForward, ToTargetFlat);
	const float CrossZ = FVector::CrossProduct(ReferenceForward, ToTargetFlat).Z;

	const float SignedYaw = FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot));
	const float HorizontalDistance = FVector::Dist2D(AimOrigin, TargetLocation);
	const float SignedPitch = FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Z, HorizontalDistance));

	const FRotator LocalAimRotation(SignedPitch, SignedYaw, 0.0f);

	RawAimYaw = LocalAimRotation.Yaw;
	RawAimPitch = LocalAimRotation.Pitch;

	const bool bAllowUpperAim = bHasCurrentAttackRow
		                            ? CurrentAttackRow->bTurnUpper
		                            : bTrackUpperToTarget;

	const bool bAllowWeaponAim =
		bHasCurrentAttackRow &&
		CurrentAttackRow->bTurnWeapon;

	float DesiredUpperYaw = 0.0f;
	float DesiredUpperPitch = 0.0f;
	float UpperAimSpeed = bHasCurrentAttackRow ? UpperAimInterpSpeed : TrackAimSpeed;

	bUseUpperAim = false;

	if (bAllowUpperAim)
	{
		float UpperYawLimit = bHasCurrentAttackRow ? DefaultUpperYawLimit : TrackYawLimit;
		float UpperPitchLimit = bHasCurrentAttackRow ? DefaultUpperPitchLimit : TrackPitchLimit;

		if (bHasCurrentAttackRow && IsValid(PartComponent))
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

		DesiredUpperYaw = FMath::Clamp(LocalAimRotation.Yaw, -UpperYawLimit, UpperYawLimit);
		DesiredUpperPitch = FMath::Clamp(LocalAimRotation.Pitch, -UpperPitchLimit, UpperPitchLimit);

		bUseUpperAim = true;
	}

	TargetUpperYaw = DesiredUpperYaw;
	TargetUpperPitch = DesiredUpperPitch;

	UpperAimYaw = InterpAngleDegrees(UpperAimYaw, DesiredUpperYaw, DeltaSeconds, UpperAimSpeed);
	UpperAimPitch = FMath::FInterpTo(UpperAimPitch, DesiredUpperPitch, DeltaSeconds, UpperAimSpeed);

	float DesiredWeaponYaw = 0.0f;
	float DesiredWeaponPitch = 0.0f;
	float WeaponAimSpeed = WeaponAimInterpSpeed;

	bUseWeaponAim = false;

	if (bAllowWeaponAim)
	{
		float WeaponYawLimit = DefaultWeaponYawLimit;
		float WeaponPitchLimit = DefaultWeaponPitchLimit;

		if (IsValid(PartComponent))
		{
			float PartYawLimit = 0.0f;
			float PartPitchLimit = 0.0f;
			float PartAimSpeed = 0.0f;

			const bool bHasWeaponAimData =
				PartComponent->TryGetAimLimitsByAttackId(
					CurrentAttackRow->AttackId,
					ENSEnemyPartAimRole::Weapon,
					PartYawLimit,
					PartPitchLimit,
					PartAimSpeed);

			if (bHasWeaponAimData)
			{
				if (PartYawLimit > 0.0f)
				{
					WeaponYawLimit = PartYawLimit;
				}

				if (PartPitchLimit > 0.0f)
				{
					WeaponPitchLimit = PartPitchLimit;
				}

				if (PartAimSpeed > 0.0f)
				{
					WeaponAimSpeed = PartAimSpeed;
				}
			}
		}

		const float ResidualYaw = FRotator::NormalizeAxis(LocalAimRotation.Yaw - UpperAimYaw);
		const float ResidualPitch = FRotator::NormalizeAxis(LocalAimRotation.Pitch - UpperAimPitch);

		if (WeaponYawLimit > 0.0f)
		{
			DesiredWeaponYaw = FMath::Clamp(ResidualYaw, -WeaponYawLimit, WeaponYawLimit);
		}

		if (WeaponPitchLimit > 0.0f)
		{
			DesiredWeaponPitch = FMath::Clamp(ResidualPitch, -WeaponPitchLimit, WeaponPitchLimit);
		}

		bUseWeaponAim = true;
	}

	TargetWeaponYaw = DesiredWeaponYaw;
	TargetWeaponPitch = DesiredWeaponPitch;

	WeaponAimYaw = InterpAngleDegrees(WeaponAimYaw, DesiredWeaponYaw, DeltaSeconds, WeaponAimSpeed);
	WeaponAimPitch = FMath::FInterpTo(WeaponAimPitch, DesiredWeaponPitch, DeltaSeconds, WeaponAimSpeed);
}

void UNSTitanWalkerAnimInstance::UpdateControlRigBlend(float DeltaSeconds)
{
	const bool bWantsUpperAim =
		bUseUpperAim &&
		!bIsDead &&
		!bIsHitReacting;

	const bool bWantsWeaponAim =
		bUseWeaponAim &&
		!bIsDead &&
		!bIsHitReacting;

	const float TargetUpperAlpha = bWantsUpperAim ? 1.0f : 0.0f;
	const float TargetWeaponAlpha = bWantsWeaponAim ? 1.0f : 0.0f;

	if (ControlRigInterpSpeed <= 0.0f || DeltaSeconds <= UE_KINDA_SMALL_NUMBER)
	{
		UpperAimAlpha = TargetUpperAlpha;
		WeaponAimAlpha = TargetWeaponAlpha;
	}
	else
	{
		UpperAimAlpha = FMath::FInterpTo(UpperAimAlpha, TargetUpperAlpha, DeltaSeconds, ControlRigInterpSpeed);
		WeaponAimAlpha = FMath::FInterpTo(WeaponAimAlpha, TargetWeaponAlpha, DeltaSeconds, ControlRigInterpSpeed);
	}

	const float DisableThreshold = FMath::Max(ControlRigPoseDisableThreshold, 0.0f);

	if (!bWantsUpperAim && UpperAimAlpha <= DisableThreshold)
	{
		UpperAimAlpha = 0.0f;
	}

	if (!bWantsWeaponAim && WeaponAimAlpha <= DisableThreshold)
	{
		WeaponAimAlpha = 0.0f;
	}

	if (!bHasCurrentAttackRow && !bUseWeaponAim && WeaponAimAlpha <= DisableThreshold)
	{
		CurrentAttackId = NAME_None;
	}

	ControlRigAlpha = FMath::Max(UpperAimAlpha, WeaponAimAlpha);

	const bool bWantsControlRigPose =
		bWantsUpperAim ||
		bWantsWeaponAim ||
		ControlRigAlpha > DisableThreshold;

	const bool bHasAttackPoseRequest =
		!CurrentAttackId.IsNone() &&
		bWantsControlRigPose;

	bUseControlRigAttack = bHasAttackPoseRequest;

	bUseAttackBasePose =
		bUseControlRigAttack &&
		!bIsMoving;

	bShouldUseControlRigPose = bWantsControlRigPose;
}

void UNSTitanWalkerAnimInstance::UpdateCombatPoseHold()
{
	if (bIsDead)
	{
		bUseCombatBasePose = true;
		return;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;

	const bool bRecentlyHadTarget =
		World &&
		(CurrentTime - LastTargetSeenTime) <= CombatPoseHoldTime;

	const bool bWantsCombatBasePose =
		bUseAttackBasePose ||
		bRecentlyHadTarget;

	bUseCombatBasePose =
		bWantsCombatBasePose &&
		!bIsMoving &&
		!bIsHitReacting;
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

void UNSTitanWalkerAnimInstance::ResetAimImmediate()
{
	bUseUpperAim = false;
	bUseWeaponAim = false;

	UpperAimYaw = 0.0f;
	UpperAimPitch = 0.0f;
	WeaponAimYaw = 0.0f;
	WeaponAimPitch = 0.0f;

	UpperAimAlpha = 0.0f;
	WeaponAimAlpha = 0.0f;
	ControlRigAlpha = 0.0f;

	RawAimYaw = 0.0f;
	RawAimPitch = 0.0f;
	TargetUpperYaw = 0.0f;
	TargetUpperPitch = 0.0f;
	TargetWeaponYaw = 0.0f;
	TargetWeaponPitch = 0.0f;

	CurrentAttackId = NAME_None;
	bUseControlRigAttack = false;
	bUseAttackBasePose = false;
	bShouldUseControlRigPose = false;
	bHasCurrentAttackRow = false;
}
