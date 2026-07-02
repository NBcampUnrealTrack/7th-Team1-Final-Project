// Copyright 2026 One Team. All rights reserved.


#include "NSFlyingLocomotionComponent.h"

#include "GameFramework/FloatingPawnMovement.h"


UNSFlyingLocomotionComponent::UNSFlyingLocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UNSFlyingLocomotionComponent::BeginPlay()
{
	Super::BeginPlay();
	InitSteeringDirections();
}

void UNSFlyingLocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// 서버 권한 확인, 오너/무브먼트 유효성 확인
	// 회전 갱신
	// 지형 샘플 주기 누적, 주기 도달 시 고도 유지 호출 후 누적값 리셋
	
	if (!HasAuthorityChecked() || !MovementComponent.IsValid() || !OwnerPawn.IsValid()) return;
	
	UpdateRotation(DeltaTime);
	
	GroundSampleAccumulator += DeltaTime;
	if (GroundSampleAccumulator >= GroundSampleInterval)  // 예: 0.1초
	{
		MaintainAltitude(GroundSampleAccumulator);
		GroundSampleAccumulator = 0.f;
	}
	
}

bool UNSFlyingLocomotionComponent::HasAuthorityChecked() const
{
	if (!OwnerPawn.IsValid()) return false;
	return OwnerPawn->HasAuthority();
}

#pragma region Rotation
void UNSFlyingLocomotionComponent::UpdateRotation(float DeltaSeconds)
{
	// 유효성 방어 (Tick에서 이미 체크했지만 이중 방어)
	if (!OwnerPawn.IsValid() || !MovementComponent.IsValid()) return;
    
	// 회전 타겟 유무에 따라 desired yaw 결정
	const bool bHasTarget = RotationTarget.IsValid();
    
	FRotator Desired;
    
	if (bHasTarget)
	{
		// 타겟 방향 벡터 (XY 평면)
		FVector ToTarget = RotationTarget->GetActorLocation() - OwnerPawn->GetActorLocation();
		ToTarget.Z = 0.f;
        
		// 타겟이 너무 가까우면 회전 스킵 (기존 버그: CombatYawInterpSpeed와 비교하던 것 → 전용 거리 변수)
		if (ToTarget.SizeSquared() < FMath::Square(MinCombatDistanceToRotate)) return;
        
		Desired = FRotator(0.f, ToTarget.Rotation().Yaw, 0.f);
	}
	else
	{
		// velocity 방향 (XY 평면)
		FVector Vel = MovementComponent->Velocity;
		Vel.Z = 0.f;
        
		// 속도가 임계 이하면 회전 스킵 (제자리 회전 방지)
		if (Vel.SizeSquared() < FMath::Square(MinSpeedToRotate)) return;
        
		Desired = FRotator(0.f, Vel.Rotation().Yaw, 0.f);
	}
    
	// InterpSpeed 선택 후 회전 보간 적용
	const float InterpSpeed = bHasTarget ? CombatYawInterpSpeed : YawInterpSpeed;
	const FRotator NewRot = FMath::RInterpTo(OwnerPawn->GetActorRotation(), Desired, DeltaSeconds, InterpSpeed);
	OwnerPawn->SetActorRotation(NewRot);
}
#pragma endregion

#pragma region PublicAPI
void UNSFlyingLocomotionComponent::InitializeLocomotion(APawn* InOwnerPawn, UFloatingPawnMovement* InMovementComponent)
{
	// 입력 유효성 확인
	// 오너 폰과 무브먼트 컴포넌트 WeakPtr 캐싱
	if (!ensureMsgf(InOwnerPawn && InMovementComponent,
	TEXT("[%s] InitializeLocomotion: OwnerPawn=%s, MovementComponent=%s"),
	*GetName(),
	InOwnerPawn ? TEXT("Valid") : TEXT("NULL"),
	InMovementComponent ? TEXT("Valid") : TEXT("NULL")))
	{
		return;
	}
	
	OwnerPawn = InOwnerPawn;
	MovementComponent = InMovementComponent;
}

void UNSFlyingLocomotionComponent::RequestMoveTowards(const FVector& TargetLocation)
{
	if (!HasAuthorityChecked()) return;
	
	FVector TargetPosition = TargetLocation - OwnerPawn->GetActorLocation();
	TargetPosition.Z = 0.f;
	
	if (TargetPosition.SizeSquared() < FMath::Square(ArrivalRadius)) return;
	
	FVector TargetDirection = TargetPosition.GetSafeNormal();  
	BuildInterestMap(TargetDirection);  
	BuildDangerMap();  
	FVector SteeringDirection = ChooseSteeringDirection();  
  
	OwnerPawn->AddMovementInput(SteeringDirection, 1.0f);
}

void UNSFlyingLocomotionComponent::SetRotationTarget(AActor* InTarget)
{
	RotationTarget = InTarget;
}

bool UNSFlyingLocomotionComponent::HasReachedLocation(const FVector& TargetLocation) const
{
	if (!OwnerPawn.IsValid()) return false;
    
	const float DistSqXY = FVector::DistSquaredXY(OwnerPawn->GetActorLocation(), TargetLocation);
	return DistSqXY < FMath::Square(ArrivalRadius);
}

#pragma endregion

#pragma region Altitude
void UNSFlyingLocomotionComponent::MaintainAltitude(float DeltaSeconds)
{
	if (!OwnerPawn.IsValid() || !HasAuthorityChecked()) return;
	
	// 고도 가장 높은 값
	float OutZ;
	
	// 지형 추종 샘플을 순회하며 가장 높은 지형 위치를 반환
	if(SampleHighestGround(OutZ))
	{
		float RawTarget = OutZ + Altitude;
		// 이미 유효한 지형 정보를 가지고 있는지 확인 첫감지
		if (!bHasValidGround)
		{
			SmoothedTargetHeight = RawTarget;
			bHasValidGround = true;
		}
		else
		{
			// Clamp를 통해 변화량 제한 급격한 이동 방지
			float DesiredPoint = RawTarget - SmoothedTargetHeight;
			SmoothedTargetHeight += FMath::Clamp(DesiredPoint, -MaxDescendSpeed * DeltaSeconds, MaxClimbSpeed * DeltaSeconds);
		}
		
		// 떨림 방지 데드존 적은값 이동을 위해 드론이 흔들리는것을 방지
		const float DesiredMoveDis = SmoothedTargetHeight - OwnerPawn->GetActorLocation().Z;
		if (FMath::Abs(DesiredMoveDis) > AltitudeDeadZone)
		{
			// Clamp를 통해 입력강도 값 -1~1 사이 값으로 변환 후 적용
			const float InputZ = FMath::Clamp(DesiredMoveDis / AltitudeCorrectionRange, -1.f, 1.f);
			OwnerPawn->AddMovementInput(FVector::UpVector, InputZ);
		}
	}
}

bool UNSFlyingLocomotionComponent::TraceGroundAt(const FVector& WorldXY, float& OutZ) const
{
	if (!OwnerPawn.IsValid()) return false;
	// 시작위치 종료위치 설정
	const FVector StartWorldLocation = WorldXY;
	const FVector EndWorldLocation = WorldXY - FVector(0.f, 0.f, GroundTraceDistance);
	
	// 트레이스 정보 입력
	FHitResult Hit;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(OwnerPawn.Get());
	
	if (GetWorld()->LineTraceSingleByChannel(Hit, StartWorldLocation, EndWorldLocation, ECC_Visibility, CollisionParams))
	{
		OutZ = Hit.ImpactPoint.Z;
		return true;
	}
	return false;
}

bool UNSFlyingLocomotionComponent::SampleHighestGround(float& OutGroundZ) const
{
	if (!MovementComponent.IsValid() || !OwnerPawn.IsValid()) return false;
	
	// 샘플 포인트 여러개 벡터로 배열
	TArray<FVector> SamplePoints;
	// 지형 정보를 받아올 멤버변수 지형 샘플 갯수 + 드론현재위치 1 + 이동 방향 예측위치 1 만큼 공간확보
	SamplePoints.Reset(GroundSampleCount + 2);
        
	const FVector DroneLocation = OwnerPawn->GetActorLocation();
	SamplePoints.Add(DroneLocation);
	
	// 드론의 이동 방향 예측
	FVector ForwardXY = MovementComponent->Velocity.GetSafeNormal2D();
	
	if (ForwardXY.IsNearlyZero())
	{
		ForwardXY = OwnerPawn->GetActorForwardVector().GetSafeNormal2D();
	}
	
	const FVector LookAheadPoint = DroneLocation + ForwardXY * GroundLookAheadDistance;
	SamplePoints.Add(LookAheadPoint);
	
	// 360도 원 기준으로 지형 샘플 만큼 나누기
	const float AngleStep = 360.f / GroundSampleCount;
	
	// 나온 각 각도에 지형 샘플 포인트 배치
	for (int32 i = 0; i < GroundSampleCount; ++i)
	{
		const float Angle = i * AngleStep;
		const FVector Dir = FRotator(0.f, Angle, 0.f).Vector();
		const FVector SamplePoint = DroneLocation + Dir * GroundSampleRadius;
		SamplePoints.Add(SamplePoint);
	}
	
	// 처음 들어오는 값을 무조건 갱신하기위해 가장 낮은 값 대입
	float HighestZ = TNumericLimits<float>::Lowest();
	bool bFound = false;
	
	// 존재하는 지형 샘플 마다 순회하며 아래쪽으로 트레이스 발사 최대 값 갱신
	for (const FVector& Point : SamplePoints)
	{
		float HitZ;
		if (TraceGroundAt(Point, HitZ))
		{
			bFound = true;
			HighestZ = FMath::Max(HighestZ, HitZ);
		}
	}
	
	// 순회 후 가장 높은 지형을 반환 및 bool 값 반환
	OutGroundZ = HighestZ;
	return bFound;
}
#pragma endregion

#pragma region Steering
void UNSFlyingLocomotionComponent::InitSteeringDirections()
{
	SteeringDirections.Reset(NumSteeringDirections);
	float AngleStep = 360.0f / NumSteeringDirections;
	
	for (int32 i = 0; i < NumSteeringDirections; ++i)
	{
		float Angle = i * AngleStep;
		
		FRotator Rotation = FRotator(0.f,Angle,0.f);
		FVector Direction = Rotation.Vector();
		
		SteeringDirections.Add(Direction);
	}
}

void UNSFlyingLocomotionComponent::BuildInterestMap(const FVector& DesiredDirection)
{
	InterestMap.Reset(SteeringDirections.Num());
	
	for (const FVector& Direction : SteeringDirections)
	{
		float Interest = FVector::DotProduct(Direction,DesiredDirection);
		InterestMap.Add(Interest);
	}
}

void UNSFlyingLocomotionComponent::BuildDangerMap()
{
	if (!OwnerPawn.IsValid()) return;
	DangerMap.Reset(SteeringDirections.Num());
	
	for (const FVector& Direction : SteeringDirections)
	{
		const FVector Start = OwnerPawn->GetActorLocation();
		const FVector End = Start + Direction * AvoidanceTraceDistance;
		
		FHitResult Hit;
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(OwnerPawn.Get());
		CollisionParams.bTraceComplex = false;
		
		float Danger = 0.f;
	
		if (GetWorld()->SweepSingleByChannel(
			Hit,
			Start,
			End,
			FQuat::Identity,
			ECollisionChannel::ECC_Visibility,
			FCollisionShape::MakeSphere(AvoidanceTraceRadius),
			CollisionParams))
		{
			if (!IsWalkableSurface(Hit.Normal))
			{
				Danger = 1.f - FMath::Clamp(Hit.Distance / AvoidanceTraceDistance, 0.f, 1.f);
			}
			else
			{
				Danger = 0.f;
			}
		}
		
		DangerMap.Add(Danger);
	}
}

FVector UNSFlyingLocomotionComponent::ChooseSteeringDirection() const
{
	float BestInterest = TNumericLimits<float>::Lowest();
	int32 BestIndex = INDEX_NONE;
	
	for (int32 i = 0; i < NumSteeringDirections; ++i)
	{
		if (DangerMap[i] > DangerThreshold)
		{
			continue;
		}
		
		if (InterestMap[i] > BestInterest)
		{
			BestInterest = InterestMap[i];
			BestIndex = i;
		}
	}
	
	if (BestIndex == INDEX_NONE)
	{
		return FVector::ZeroVector;
	}
	
	return SteeringDirections[BestIndex];
}

bool UNSFlyingLocomotionComponent::IsWalkableSurface(const FVector& SurfaceNormal) const
{
	float SurfaceAndUpVectorDot = FVector::DotProduct(SurfaceNormal,FVector::UpVector);
	float MaxSlopeRadians = FMath::DegreesToRadians(MaxWalkableSlopeAngle);
	float MaxSlopeCos = FMath::Cos(MaxSlopeRadians);
	
	return SurfaceAndUpVectorDot >= MaxSlopeCos;
}
#pragma endregion
