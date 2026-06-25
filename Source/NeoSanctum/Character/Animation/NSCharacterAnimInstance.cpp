// Copyright 2026 One Team. All rights reserved.

#include "NSCharacterAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "CharacterTrajectoryComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Combat/Weapon/NSWeaponBase.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

void UNSCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// 초기화 시점의 소유 캐릭터/주요 컴포넌트 캐싱
	RefreshCachedReferences();
}

void UNSCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerCharacter)
	{
		RefreshCachedReferences();
	}

	if (!OwnerCharacter)
	{
		ResetRuntimeData();
		return;
	}

	PreviousMovementState = MovementState;
	PreviousGait = Gait;
	PreviousSpeed2D = Speed2D;

	// 이전 프레임 값 기준 이동/전투/조준 상태 순차 갱신
	UpdateMovementData();
	UpdateMovementMode();
	UpdateMovementState();
	UpdateGait();
	UpdateStartStopData(DeltaSeconds);
	UpdateLandingData(DeltaSeconds);
	UpdatePivotData(DeltaSeconds);
	UpdateSpinTransitionData();
	UpdateAimData();
	UpdateTurnInPlaceData();
	UpdateTimeToLand();
	UpdateHandIKData(DeltaSeconds);

	PreviousVerticalVelocity = VerticalVelocity;
	bWasFalling = MovementMode == ENSAnimMovementMode::InAir;
}

void UNSCharacterAnimInstance::RefreshCachedReferences()
{
	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
	CharacterMovement = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;
	CharacterTrajectoryComponent = OwnerCharacter ? OwnerCharacter->FindComponentByClass<UCharacterTrajectoryComponent>() : nullptr;
}

void UNSCharacterAnimInstance::ResetRuntimeData()
{
	// Preview나 Pawn이 없는 경우에 이전 프레임 값 제거해야 안 터짐
	CharacterMovement = nullptr;
	CharacterTrajectoryComponent = nullptr;

	Velocity = FVector::ZeroVector;
	LocalVelocity = FVector::ZeroVector;
	Acceleration = FVector::ZeroVector;
	LocalAcceleration = FVector::ZeroVector;
	Speed2D = 0.f;
	GroundSpeed = 0.f;
	PreviousSpeed2D = 0.f;
	StopSpeed2D = 0.f;
	VerticalVelocity = 0.f;
	LocomotionAngle = 0.f;
	AccelerationAngle = 0.f;
	bHasAcceleration = false;
	bShouldMove = false;

	MovementMode = ENSAnimMovementMode::OnGround;
	MovementState = ENSAnimMovementState::Idle;
	PreviousMovementState = ENSAnimMovementState::Idle;
	Gait = ENSAnimGait::Walk;
	PreviousGait = ENSAnimGait::Walk;
	StopGait = ENSAnimGait::Walk;

	bIsStarting = false;
	bIsPivoting = false;
	bJustLandedLight = false;
	bJustLandedHeavy = false;
	bShouldTurnInPlace = false;
	bShouldSpinTransition = false;
	TimeToLand = 0.f;
	bActivateLeftHandIK = false;
	LeftHandIKAlpha = 0.f;
	LeftHandIKTransform = FTransform::Identity;

	AimYaw = 0.f;
	AimPitch = 0.f;

	TurnInPlaceDirection = ENSTurnInPlaceDirection::None;
	TurnInPlaceYawDelta = 0.f;

	bWasFalling = false;
	PreviousVerticalVelocity = 0.f;
	StartStateRemainingTime = 0.f;
	LandStateRemainingTime = 0.f;
	PivotStateRemainingTime = 0.f;
}

void UNSCharacterAnimInstance::UpdateMovementData()
{
	if (!CharacterMovement)
	{
		CharacterMovement = OwnerCharacter->GetCharacterMovement();
	}

	Velocity = OwnerCharacter->GetVelocity();
	LocalVelocity = OwnerCharacter->GetActorTransform().InverseTransformVectorNoScale(Velocity);
	Speed2D = FVector(Velocity.X, Velocity.Y, 0.f).Size();
	GroundSpeed = Speed2D;
	VerticalVelocity = Velocity.Z;
	// Chooser/BlendSpace 입력용 캐릭터 로컬 기준 이동 각도
	LocomotionAngle = Speed2D > MoveSpeedThreshold
		? FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X))
		: 0.f;

	if (CharacterMovement)
	{
		Acceleration = CharacterMovement->GetCurrentAcceleration();
		LocalAcceleration = OwnerCharacter->GetActorTransform().InverseTransformVectorNoScale(Acceleration);
		bHasAcceleration = Acceleration.SizeSquared2D() > UE_KINDA_SMALL_NUMBER;
		// 시작/피벗 계열 전이 선택용 가속 방향
		AccelerationAngle = bHasAcceleration
			? FMath::RadiansToDegrees(FMath::Atan2(LocalAcceleration.Y, LocalAcceleration.X))
			: 0.f;
	}
	else
	{
		Acceleration = FVector::ZeroVector;
		LocalAcceleration = FVector::ZeroVector;
		bHasAcceleration = false;
		AccelerationAngle = 0.f;
	}

	bShouldMove = Speed2D > MoveSpeedThreshold && bHasAcceleration;
}

void UNSCharacterAnimInstance::UpdateMovementMode()
{
	// CharacterMovement Falling 여부를 애니메이션용 단순 상태로 변환
	const bool bIsFalling = CharacterMovement && CharacterMovement->IsFalling();
	MovementMode = bIsFalling
		? ENSAnimMovementMode::InAir
		: ENSAnimMovementMode::OnGround;
}

void UNSCharacterAnimInstance::UpdateMovementState()
{
	// 작은 미끄러짐은 Idle로 취급해 불필요한 이동 전이 감소
	MovementState = Speed2D > MoveSpeedThreshold
		? ENSAnimMovementState::Moving
		: ENSAnimMovementState::Idle;
}

void UNSCharacterAnimInstance::UpdateGait()
{
	if (MovementState == ENSAnimMovementState::Idle)
	{
		return;
	}

	// 속도 임계값 기반 현재 보행 단계 구분
	if (Speed2D < WalkSpeedThreshold)
	{
		Gait = ENSAnimGait::Walk;
	}
	else if (Speed2D < RunSpeedThreshold)
	{
		Gait = ENSAnimGait::Run;
	}
	else
	{
		Gait = ENSAnimGait::Sprint;
	}
}

void UNSCharacterAnimInstance::UpdateStartStopData(float DeltaSeconds)
{
	const bool bStartedMoving =
		PreviousMovementState == ENSAnimMovementState::Idle &&
		MovementState == ENSAnimMovementState::Moving;

	const bool bStoppedMoving =
		PreviousMovementState == ENSAnimMovementState::Moving &&
		MovementState == ENSAnimMovementState::Idle;

	if (bStartedMoving)
	{
		// 시작 애니메이션 선택을 위한 짧은 유지 시간
		StartStateRemainingTime = StartStateHoldTime;
	}
	else
	{
		StartStateRemainingTime = FMath::Max(0.f, StartStateRemainingTime - DeltaSeconds);
	}

	if (bStoppedMoving)
	{
		// 정지 애니메이션 선택용 정지 직전 속도와 Gait 보관
		StopGait = PreviousGait;
		StopSpeed2D = PreviousSpeed2D;
	}

	bIsStarting = StartStateRemainingTime > 0.f;
}

void UNSCharacterAnimInstance::UpdateLandingData(float DeltaSeconds)
{
	const bool bIsInAir = MovementMode == ENSAnimMovementMode::InAir;
	const bool bJustLanded = bWasFalling && !bIsInAir;

	if (bJustLanded)
	{
		const float LandSpeed = FMath::Abs(PreviousVerticalVelocity);
		// 착지 직전 낙하 속도 기반 Light/Heavy 착지 구분
		bJustLandedHeavy = LandSpeed >= HeavyLandSpeedThreshold;
		bJustLandedLight = !bJustLandedHeavy;
		LandStateRemainingTime = LandStateHoldTime;
	}
	else if (bIsInAir)
	{
		bJustLandedLight = false;
		bJustLandedHeavy = false;
		LandStateRemainingTime = 0.f;
	}
	else
	{
		LandStateRemainingTime = FMath::Max(0.f, LandStateRemainingTime - DeltaSeconds);
		if (LandStateRemainingTime <= 0.f)
		{
			bJustLandedLight = false;
			bJustLandedHeavy = false;
		}
	}
}

void UNSCharacterAnimInstance::UpdatePivotData(float DeltaSeconds)
{
	const FVector Velocity2D = FVector(Velocity.X, Velocity.Y, 0.f);
	const FVector Acceleration2D = FVector(Acceleration.X, Acceleration.Y, 0.f);
	// 현재 이동 방향과 입력 가속 방향이 크게 반대인 Pivot 후보
	const bool bWantsPivot =
		MovementState == ENSAnimMovementState::Moving &&
		bHasAcceleration &&
		Velocity2D.SizeSquared() > FMath::Square(MoveSpeedThreshold) &&
		FVector::DotProduct(Velocity2D.GetSafeNormal(), Acceleration2D.GetSafeNormal()) <= PivotAccelerationDotThreshold;

	if (bWantsPivot)
	{
		PivotStateRemainingTime = PivotStateHoldTime;
	}
	else
	{
		PivotStateRemainingTime = FMath::Max(0.f, PivotStateRemainingTime - DeltaSeconds);
	}

	bIsPivoting = PivotStateRemainingTime > 0.f;
}

void UNSCharacterAnimInstance::UpdateSpinTransitionData()
{
	// 뒤쪽에 가까운 이동 방향의 큰 회전 전이 선택
	bShouldSpinTransition =
		MovementMode == ENSAnimMovementMode::OnGround &&
		MovementState == ENSAnimMovementState::Moving &&
		Gait != ENSAnimGait::Sprint &&
		FMath::Abs(LocomotionAngle) >= SpinTransitionAngle;
}

void UNSCharacterAnimInstance::UpdateAimData()
{
	const FRotator ActorRotation = OwnerCharacter->GetActorRotation();
	const FRotator AimRotation = OwnerCharacter->GetBaseAimRotation();
	const FRotator AimDelta = (AimRotation - ActorRotation).GetNormalized();

	// AimOffset 입력 튐 방지를 위한 허용 각도 제한
	AimYaw = FMath::Clamp(FRotator::NormalizeAxis(AimDelta.Yaw), -AimYawLimit, AimYawLimit);
	AimPitch = FMath::Clamp(FRotator::NormalizeAxis(AimDelta.Pitch), -AimPitchLimit, AimPitchLimit);
}

void UNSCharacterAnimInstance::UpdateTurnInPlaceData()
{
	if (MovementMode != ENSAnimMovementMode::OnGround || MovementState != ENSAnimMovementState::Idle)
	{
		bShouldTurnInPlace = false;
		TurnInPlaceDirection = ENSTurnInPlaceDirection::None;
		TurnInPlaceYawDelta = 0.f;
		return;
	}

	const FRotator ActorRotation = OwnerCharacter->GetActorRotation();
	const FRotator AimRotation = OwnerCharacter->GetBaseAimRotation();
	TurnInPlaceYawDelta = FRotator::NormalizeAxis(AimRotation.Yaw - ActorRotation.Yaw);

	// 경계값 근처 상태 떨림 방지용 시작/종료 각도 분리
	const float AbsYawDelta = FMath::Abs(TurnInPlaceYawDelta);
	bShouldTurnInPlace = bShouldTurnInPlace
		? AbsYawDelta > TurnInPlaceStopAngle
		: AbsYawDelta >= TurnInPlaceStartAngle;

	if (!bShouldTurnInPlace)
	{
		TurnInPlaceDirection = ENSTurnInPlaceDirection::None;
		return;
	}

	const bool bIsLeftTurn = TurnInPlaceYawDelta < 0.f;
	const bool bIs180Turn = AbsYawDelta >= TurnInPlace180Angle;

	// 방향과 각도 크기 기반 90/180도 회전 애니메이션 선택
	if (bIsLeftTurn)
	{
		TurnInPlaceDirection = bIs180Turn ? ENSTurnInPlaceDirection::Left180 : ENSTurnInPlaceDirection::Left90;
	}
	else
	{
		TurnInPlaceDirection = bIs180Turn ? ENSTurnInPlaceDirection::Right180 : ENSTurnInPlaceDirection::Right90;
	}
}

void UNSCharacterAnimInstance::UpdateTimeToLand()
{
	TimeToLand = 0.f;

	if (MovementMode != ENSAnimMovementMode::InAir || VerticalVelocity >= 0.f || !OwnerCharacter)
	{
		return;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
	{
		return;
	}

	const FVector TraceStart = OwnerCharacter->GetActorLocation();
	const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, TimeToLandTraceDistance);

	// 아래 방향 지면 탐색으로 낙하 애니메이션 전이 시간 추정
	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NSAnimInstanceTimeToLand), false, OwnerCharacter);
	if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return;
	}

	const float CollisionHalfHeight = OwnerCharacter->GetSimpleCollisionHalfHeight();
	const float DistanceToGround = FMath::Max(0.f, HitResult.Distance - CollisionHalfHeight);
	// 현재 하강 속도 기준 지면 도달 시간 계산
	TimeToLand = DistanceToGround / FMath::Max(FMath::Abs(VerticalVelocity), 1.f);
}

void UNSCharacterAnimInstance::UpdateHandIKData(float DeltaSeconds)
{
	bActivateLeftHandIK = false;
	float TargetAlpha = 0.f;
	
	const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(OwnerCharacter);
	if (!PlayerCharacter)
	{
		UpdateHandIKAlpha(TargetAlpha, DeltaSeconds);
		return;
	}
	
	// 필요한 경우 State.Deactivate.HandIK 태그를 통해 임시로 HandIK를 비활성화 할 수 있음
	if (const UAbilitySystemComponent* ASC = PlayerCharacter->GetAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(NSGameplayTags::State_Deactivate_HandIK))
		{
			UpdateHandIKAlpha(TargetAlpha, DeltaSeconds);
			return;
		}
	}
	
	const ANSWeaponBase* CurrentWeapon = PlayerCharacter->GetCurrentWeapon();
	if (!IsValid(CurrentWeapon))
	{
		UpdateHandIKAlpha(TargetAlpha, DeltaSeconds);
		return;
	}
	
	// 무기에서 HandIK용 Socket Transform을 받아옴
	FTransform LeftHandIKWorldTransform;
	if (!CurrentWeapon->TryGetLeftHandIKTransform(LeftHandIKWorldTransform))
	{
		UpdateHandIKAlpha(TargetAlpha, DeltaSeconds);
		return;
	}
	
	USkeletalMeshComponent* OwningComponent = GetOwningComponent();
	if (!IsValid(OwningComponent))
	{
		UpdateHandIKAlpha(TargetAlpha, DeltaSeconds);
		return;
	}
	
	FVector BoneSpaceLocation = FVector::ZeroVector;
	FRotator BoneSpaceRotation = FRotator::ZeroRotator;
	OwningComponent->TransformToBoneSpace(
		TEXT("hand_r"),
		LeftHandIKWorldTransform.GetLocation(),
		LeftHandIKWorldTransform.Rotator(),
		BoneSpaceLocation,
		BoneSpaceRotation);
	
	LeftHandIKTransform = FTransform(BoneSpaceRotation, BoneSpaceLocation, FVector::OneVector);
	TargetAlpha = 1.f;
	UpdateHandIKAlpha(TargetAlpha, DeltaSeconds);
}

void UNSCharacterAnimInstance::UpdateHandIKAlpha(float TargetAlpha, float DeltaSeconds)
{
	LeftHandIKAlpha = FMath::FInterpTo(LeftHandIKAlpha, TargetAlpha, DeltaSeconds, LeftHandIKInterpSpeed);
	// UE_KINDA_SMALL_NUMBER : 엔진에서 사실상 0으로 보는 가장 작은 값. (0.f 같은 float보다 안전하다)
	bActivateLeftHandIK = LeftHandIKAlpha > UE_KINDA_SMALL_NUMBER;
}
