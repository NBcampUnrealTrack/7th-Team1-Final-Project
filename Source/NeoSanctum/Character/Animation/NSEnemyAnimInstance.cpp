// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Combat/Component/NSEnemyPartComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"

void UNSEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	EnemyCharacter = Cast<ANSEnemyCharacterBase>(TryGetPawnOwner());
	if (EnemyCharacter)
	{
		MovementComponent = EnemyCharacter->GetCharacterMovement();
		PartComponent = EnemyCharacter->FindComponentByClass<UNSEnemyPartComponent>();
	}
}

void UNSEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 초기화 실패 시 혹은 런타임 예외 발생 시 다시 캐싱
	if (!EnemyCharacter || !MovementComponent || !PartComponent)
	{
		EnemyCharacter = Cast<ANSEnemyCharacterBase>(TryGetPawnOwner());
		if (EnemyCharacter)
		{
			MovementComponent = EnemyCharacter->GetCharacterMovement();
			PartComponent = EnemyCharacter->FindComponentByClass<UNSEnemyPartComponent>();
		}
	}

	if (MovementComponent && EnemyCharacter)
	{
		const FVector Velocity = EnemyCharacter->GetVelocity();

		// 수평 속도만 사용하여 Idle과 Walk/Run 구분
		GroundSpeed = Velocity.Size2D();
		bIsMoving = GroundSpeed > MovingSpeedThreshold;

		// 양수면 상승, 음수면 낙하 중
		VerticalVelocity = Velocity.Z;

		// LaunchCharacter가 실행되면 MovementMode가 Falling으로 변경됨
		bIsInAir = MovementComponent->IsFalling();

		const FVector LocalVelocity = EnemyCharacter->GetActorTransform().InverseTransformVectorNoScale(Velocity);

		LocalForwardSpeed = LocalVelocity.X - 300.0f;

		const bool bActuallyMovingBackward = LocalForwardSpeed < -MovingSpeedThreshold;

		bIsRetreating = EnemyCharacter->IsRetreating() && bActuallyMovingBackward && !bIsInAir;
	}
	else
	{
		GroundSpeed = 0.0f;
		VerticalVelocity = 0.0f;
		LocalForwardSpeed = 0.0f;

		bIsMoving = false;
		bIsInAir = false;
		bIsRetreating = false;
	}

	UpdateAimRotation(DeltaSeconds);
	UpdateLeftHandIK(DeltaSeconds);
}

void UNSEnemyAnimInstance::UpdateLeftHandIK(float DeltaSeconds)
{
	float TargetAlpha = 0.0f;

	if (!IsValid(EnemyCharacter) || EnemyCharacter->IsDead())
	{
		UpdateLeftHandIKAlpha(TargetAlpha, DeltaSeconds);
		return;
	}

	if (!IsValid(PartComponent))
	{
		UpdateLeftHandIKAlpha(TargetAlpha, DeltaSeconds);
		return;
	}

	const UAbilitySystemComponent* ASC = EnemyCharacter->GetAbilitySystemComponent();
	const bool bIsInCombat = IsValid(ASC) && ASC->HasMatchingGameplayTag(NSGameplayTags::State_Enemy_Combat);
	const bool bShouldUseIK = PartComponent->ShouldUseLeftHandIKWhileEquipped() || bIsInCombat;

	if (!bShouldUseIK)
	{
		UpdateLeftHandIKAlpha(TargetAlpha, DeltaSeconds);
		return;
	}

	FTransform SocketWorldTransform;
	if (!PartComponent->TryGetLeftHandIKTransform(SocketWorldTransform))
	{
		UpdateLeftHandIKAlpha(TargetAlpha, DeltaSeconds);
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetOwningComponent();

	if (!IsValid(MeshComponent) ||
		!MeshComponent->DoesSocketExist(TEXT("hand_r")))
	{
		UpdateLeftHandIKAlpha(TargetAlpha, DeltaSeconds);
		return;
	}

	FVector BoneSpaceLocation = FVector::ZeroVector;
	FRotator BoneSpaceRotation = FRotator::ZeroRotator;

	MeshComponent->TransformToBoneSpace(
		TEXT("hand_r"),
		SocketWorldTransform.GetLocation(),
		SocketWorldTransform.Rotator(),
		BoneSpaceLocation,
		BoneSpaceRotation);

	LeftHandIKTransform = FTransform(
		BoneSpaceRotation,
		BoneSpaceLocation,
		FVector::OneVector);

	TargetAlpha = 1.0f;
	UpdateLeftHandIKAlpha(TargetAlpha, DeltaSeconds);
}

void UNSEnemyAnimInstance::UpdateLeftHandIKAlpha(float TargetAlpha, float DeltaSeconds)
{
	LeftHandIKAlpha = FMath::FInterpTo(
		LeftHandIKAlpha,
		TargetAlpha,
		DeltaSeconds,
		LeftHandIKInterpSpeed);

	if (LeftHandIKAlpha <= UE_KINDA_SMALL_NUMBER)
	{
		LeftHandIKAlpha = 0.0f;
	}
}

void UNSEnemyAnimInstance::UpdateAimRotation(float DeltaSeconds)
{
	float TargetPitch = 0.0f;
	float TargetYaw = 0.0f;
	float TargetAlpha = 0.0f;

	if (IsValid(EnemyCharacter) &&
		!EnemyCharacter->IsDead() &&
		EnemyCharacter->HasCombatAimTarget())
	{
		FVector AimOrigin = EnemyCharacter->GetActorLocation();
		FTransform MuzzleTransform;
		const bool bHasMuzzleTransform =
			IsValid(PartComponent) &&
			PartComponent->TryGetAimMuzzleTransform(MuzzleTransform);

		if (bHasMuzzleTransform)
		{
			AimOrigin = MuzzleTransform.GetLocation();
		}

		const FVector ToTarget = EnemyCharacter->GetCombatAimTargetLocation() - AimOrigin;

		if (!ToTarget.IsNearlyZero())
		{
			const FRotator WorldAimRotation = ToTarget.Rotation();

			const FRotator ReferenceRotation(0.0f, EnemyCharacter->GetActorRotation().Yaw, 0.0f);

			const FRotator LocalAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(
				WorldAimRotation,
				ReferenceRotation);

			TargetPitch = FMath::Clamp(LocalAimRotation.Pitch, -MaxAimPitch, MaxAimPitch);

			const float RawTargetYaw = LocalAimRotation.Yaw;
			float CorrectedTargetYaw = RawTargetYaw;

			if (bHasMuzzleTransform)
			{
				const float MuzzleRelativeYaw = FRotator::NormalizeAxis(
					MuzzleTransform.Rotator().Yaw -
					ReferenceRotation.Yaw);

				// 현재 AimYaw를 제외한 애니메이션 자체의 총구 편차를 추정
				float AnimationYawOffset = FRotator::NormalizeAxis(
					MuzzleRelativeYaw - AimYaw);

				if (FMath::Abs(AnimationYawOffset) <
					MuzzleYawErrorDeadZone)
				{
					AnimationYawOffset = 0.0f;
				}

				AnimationYawOffset = FMath::Clamp(
					AnimationYawOffset,
					-MaxMuzzleYawCorrection,
					MaxMuzzleYawCorrection);

				// 총구가 오른쪽으로 벗어나면 AimYaw를 왼쪽으로 보정
				CorrectedTargetYaw -=
					AnimationYawOffset * MuzzleYawCorrectionGain;
			}

			TargetYaw = FMath::Clamp(
				CorrectedTargetYaw,
				-MaxAimYaw,
				MaxAimYaw);

			TargetAlpha = 1.0f;
		}
	}

	constexpr float AimDeadZoneDegrees = 0.35f;

	if (TargetAlpha > 0.0f)
	{
		if (FMath::Abs(TargetPitch - AimPitch) < AimDeadZoneDegrees)
		{
			TargetPitch = AimPitch;
		}

		if (FMath::Abs(TargetYaw - AimYaw) < AimDeadZoneDegrees)
		{
			TargetYaw = AimYaw;
		}
	}

	AimPitch = FMath::FInterpTo(AimPitch, TargetPitch, DeltaSeconds, AimInterpSpeed);
	AimYaw = FMath::FInterpTo(AimYaw, 0.0f, DeltaSeconds, AimInterpSpeed);
	AimAlpha = FMath::FInterpTo(AimAlpha, TargetAlpha, DeltaSeconds, AimInterpSpeed);

	auto MakeWeightedAimRotation =
		[this](float PitchWeight, float YawWeight)
	{
		return FRotator(
			0.0f,
			AimYaw * AimYawScale * YawWeight,
			AimPitch * AimPitchScale * PitchWeight);
	};

	// 각 축의 가중치 합계가 약 1.0이 되도록 분배
	Spine01AimRotation = MakeWeightedAimRotation(0.15f, 0.0f);
	Spine02AimRotation = MakeWeightedAimRotation(0.25f, 0.0f);
	Spine03AimRotation = MakeWeightedAimRotation(0.30f, 0.0f);
	ClavicleAimRotation = MakeWeightedAimRotation(0.20f, 0.0f);
	UpperArmAimRotation = MakeWeightedAimRotation(0.10f, 0.0f);
}
