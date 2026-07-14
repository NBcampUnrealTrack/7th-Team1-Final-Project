// Copyright 2026 One Team. All rights reserved.


#include "NSFlyingLocomotionComponent.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "GenericTeamAgentInterface.h"


UNSFlyingLocomotionComponent::UNSFlyingLocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UNSFlyingLocomotionComponent::BeginPlay()
{
	Super::BeginPlay();
	// 스티어링 방향 벡터 배열 사전 계산
	InitSteeringDirections();
}

void UNSFlyingLocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	// 부모 FloatingPawnMovement에서 pending input 소비, velocity 적분, SafeMoveUpdatedComponent 처리
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 서버 권한 및 오너 유효성 방어
	if (!HasAuthorityChecked()) return;
	
	if (bDeathFalling)
	{
		APawn* OwnerPawn = GetPawnOwner();
		if (!IsValid(OwnerPawn)) return;

		// 현재 위치 아래로 지면 샘플 (GroundTraceDistance 이내에 지면이 있을 때만 true)
		float GroundZ = 0.f;
		const bool bHasGround = TraceGroundAt(OwnerPawn->GetActorLocation(), GroundZ);
		const float RestZ = GroundZ;

		// 지면 도달 → 위치 스냅 + 완전 정지 (Super가 먼저 이동시켜 오버슈트되므로 스냅으로 보정)
		if (bHasGround && OwnerPawn->GetActorLocation().Z <= RestZ)
		{
			FVector Loc = OwnerPawn->GetActorLocation();
			Loc.Z = RestZ;
			OwnerPawn->SetActorLocation(Loc, false, nullptr, ETeleportType::TeleportPhysics);
			Velocity = FVector::ZeroVector;
			return;
		}

		// 아직 공중: 수평 감쇠 + 중력 램프
		Velocity.X = FMath::FInterpTo(Velocity.X, 0.f, DeltaTime, DeathFallHorizontalDamp);
		Velocity.Y = FMath::FInterpTo(Velocity.Y, 0.f, DeltaTime, DeathFallHorizontalDamp);
		Velocity.Z = FMath::Max(Velocity.Z - DeathFallGravity * DeltaTime, -DeathFallMaxSpeed);
		return;
	}
	
	if (bScriptedMove) { ApplyScriptedMoveInput(DeltaTime); }
	
	// 회전 갱신 (타겟 or velocity 방향)
	UpdateRotation(DeltaTime);

	if (bScriptedMove)
	{
		// 스크립트 이동 중엔 매 프레임 갱신, 목표는 ScriptedTargetZ 고정값 (SampleHighestGround 미사용)
		DriveScriptedAltitude(DeltaTime);
	}
	else
	{
		// 고도 샘플링은 매 프레임이 아니라 주기적으로만 수행 (트레이스 비용 절감)
		GroundSampleAccumulator += DeltaTime;
		if (GroundSampleAccumulator >= GroundSampleInterval)
		{
			MaintainAltitude(GroundSampleAccumulator);
			GroundSampleAccumulator = 0.f;
		}
	}
}

void UNSFlyingLocomotionComponent::ResetLocomotionState()
{
	// 자기 자신이 MovementComponent이므로 자기 Velocity를 직접 초기화
	Velocity = FVector::ZeroVector;

	bHasValidGround = false;
	SmoothedTargetHeight = 0.f;
	GroundSampleAccumulator = 0.f;
	bIsRetreating = false;
	RetreatEnteredTime = 0.f;
	
	if (bDeathFalling)
	{
		Deceleration = CachedDeceleration;
		bDeathFalling = false;
	}
}

void UNSFlyingLocomotionComponent::StartDeathFall()
{
	if (bDeathFalling) return;
	bDeathFalling = true;
	bScriptedMove = false;
	RotationTarget = nullptr;
	CachedDeceleration = Deceleration;
	Deceleration = 0.f;
}

bool UNSFlyingLocomotionComponent::HasAuthorityChecked() const
{
	// PawnMovementComponent가 제공하는 오너 접근자 사용
	const APawn* OwnerPawn = GetPawnOwner();
	return IsValid(OwnerPawn) && OwnerPawn->HasAuthority();
}

#pragma region Rotation
void UNSFlyingLocomotionComponent::UpdateRotation(float DeltaSeconds)
{
	// 오너 폰 확보
	APawn* OwnerPawn = GetPawnOwner();
	if (!IsValid(OwnerPawn)) return;

	// 회전 타겟 유무에 따라 desired yaw 결정
	const bool bHasTarget = RotationTarget.IsValid();

	FRotator Desired;

	if (bHasTarget)
	{
		// 타겟 방향 벡터 (XY 평면)
		FVector ToTarget = RotationTarget->GetActorLocation() - OwnerPawn->GetActorLocation();
		ToTarget.Z = 0.f;

		// 타겟이 너무 가까우면 회전 스킵
		if (ToTarget.SizeSquared() < FMath::Square(MinCombatDistanceToRotate)) return;

		Desired = FRotator(0.f, ToTarget.Rotation().Yaw, 0.f);
	}
	else
	{
		// velocity 방향 (XY 평면) - 자기 Velocity 사용
		FVector Vel = Velocity;
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
void UNSFlyingLocomotionComponent::RequestMoveTowards(const FVector& TargetLocation)
{
	// 서버 권한 확인
	if (!HasAuthorityChecked()) return;
	if (bDeathFalling) return;
	if (bScriptedMove) return;
	if (bSteeringInputLocked) return;
	
	APawn* OwnerPawn = GetPawnOwner();
	if (!IsValid(OwnerPawn)) return;

	// 목표까지의 XY 벡터
	FVector TargetPosition = TargetLocation - OwnerPawn->GetActorLocation();
	TargetPosition.Z = 0.f;

	// 도착 반경 안이면 이동 입력 없음
	if (TargetPosition.SizeSquared() < FMath::Square(ArrivalRadius)) return;

	// 컨텍스트 스티어링 계산
	const FVector TargetDirection = TargetPosition.GetSafeNormal();
	BuildInterestMap(TargetDirection);
	BuildDangerMap();

	FVector SteeringDirection = ChooseSteeringDirection();
	float InputScale = 1.0f;

	// 후퇴 락 유지 시간 판정
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	// 후퇴중이며 && 현재 시간에서 후퇴시간을 뺀만큼이 최소 후퇴시간보다 작다면 true
	const bool bStillLocked = bIsRetreating && (CurrentTime - RetreatEnteredTime) < RetreatMinDuration;

	// 갈 방향이 없거나 후퇴 락 상태면 후퇴 방향으로 대체
	if (SteeringDirection.IsNearlyZero() || bStillLocked)
	{
		if (bEnableRetreat)
		{
			const FVector RetreatDir = ChooseRetreatDirection();
			if (!RetreatDir.IsNearlyZero())
			{
				SteeringDirection = RetreatDir;
				InputScale = RetreatInputScale;

				if (!bIsRetreating)
				{
					bIsRetreating = true;
					RetreatEnteredTime = CurrentTime;
				}
			}
		}
	}
	else
	{
		bIsRetreating = false;
	}

	// 유효한 방향이 있으면 경사 스케일 적용 후 이동 입력
	if (!SteeringDirection.IsNearlyZero())
	{
		const float SlopeScale = ComputeSlopeSpeedScale();
		OwnerPawn->AddMovementInput(SteeringDirection, InputScale * SlopeScale);
	}
}

void UNSFlyingLocomotionComponent::SetRotationTarget(AActor* InTarget)
{
	RotationTarget = InTarget;
}

bool UNSFlyingLocomotionComponent::HasReachedLocation(const FVector& TargetLocation) const
{
	const APawn* OwnerPawn = GetPawnOwner();
	if (!IsValid(OwnerPawn)) return false;

	// XY 평면 거리로만 도착 판정
	const float DistSqXY = FVector::DistSquaredXY(OwnerPawn->GetActorLocation(), TargetLocation);
	return DistSqXY < FMath::Square(ArrivalRadius);
}
#pragma endregion

#pragma region Altitude
bool UNSFlyingLocomotionComponent::TraceGroundAtWorldLocation(const UWorld* World, const FVector& TraceStart,
	float TraceDistance, ECollisionChannel Channel, float MaxWalkableSlopeAngleDeg, const AActor* IgnoreActor,
	FHitResult& OutHit)
{
	if (!World) return false;

	const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, TraceDistance);

	FCollisionQueryParams CollisionParams;
	if (IgnoreActor)
	{
		CollisionParams.AddIgnoredActor(IgnoreActor);
	}

	if (!World->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, Channel, CollisionParams))
	{
		return false;
	}

	// 지오메트리에 파고든 채 시작한 히트는 법선이 부정확 → 바닥으로 인정하지 않음
	if (OutHit.bStartPenetrating)
	{
		return false;
	}

	// 아래로 쐈는데 걷기 불가능한 가파른 면(=벽/장애물 옆면)에 맞으면 바닥이 아님
	const float WalkableCosThreshold = FMath::Cos(FMath::DegreesToRadians(MaxWalkableSlopeAngleDeg));
	if (OutHit.ImpactNormal.Z < WalkableCosThreshold)
	{
		return false;
	}

	return true;
}

void UNSFlyingLocomotionComponent::MaintainAltitude(float DeltaSeconds)
{
	// 서버 권한 및 오너 확보
	if (!HasAuthorityChecked()) return;

	APawn* OwnerPawn = GetPawnOwner();
	if (!IsValid(OwnerPawn)) return;

	// 지형 샘플링 실패 시 조기 종료
	float OutZ;
	if (!SampleHighestGround(OutZ))
	{
		return;
	}
	
	// 원하는 목표 높이 (지형 + 유지 고도)
	const float RawTarget = OutZ + Altitude;

	// 첫 감지면 즉시 세팅, 이후엔 상승/하강 속도 클램프
	if (!bHasValidGround)
	{
		SmoothedTargetHeight = RawTarget;
		bHasValidGround = true;
	}
	else
	{
		// 프레임간 변화량 제한으로 급격한 이동 방지
		const float DesiredPoint = RawTarget - SmoothedTargetHeight;
		SmoothedTargetHeight += FMath::Clamp(DesiredPoint,
			-MaxDescendSpeed * DeltaSeconds,
			MaxClimbSpeed * DeltaSeconds);
	}

	// 데드존 안이면 미세 조정 스킵 (떨림 방지)
	const float DesiredMoveDis = SmoothedTargetHeight - OwnerPawn->GetActorLocation().Z;
	if (FMath::Abs(DesiredMoveDis) > AltitudeDeadZone)
	{
		// 입력강도를 -1 ~ 1로 정규화 후 Z 방향 입력
		const float InputZ = FMath::Clamp(DesiredMoveDis / AltitudeCorrectionRange, -1.f, 1.f);
		OwnerPawn->AddMovementInput(FVector::UpVector, InputZ);
	}
}

bool UNSFlyingLocomotionComponent::TraceGroundAt(const FVector& WorldXY, float& OutZ) const
{
	const APawn* OwnerPawn = GetPawnOwner();
	if (!IsValid(OwnerPawn)) return false;

	FHitResult Hit;
	const bool bHit = TraceGroundAtWorldLocation(
		GetWorld(), WorldXY, GroundTraceDistance, GroundChannel, MaxWalkableSlopeAngle, OwnerPawn, Hit);

	if (!bHit) return false;

	OutZ = Hit.ImpactPoint.Z;
	return true;
}

bool UNSFlyingLocomotionComponent::SampleHighestGround(float& OutGroundZ) const
{
	const APawn* OwnerPawn = GetPawnOwner();
	if (!IsValid(OwnerPawn)) return false;

	// 샘플 포인트 배열 준비 (현재 위치 + 이동 방향 예측 + 원형 샘플)
	TArray<FVector> SamplePoints;
	// 지형 샘플 갯수 + 드론 현재 위치 1 + 이동 방향 예측 위치 1 만큼 공간 확보
	SamplePoints.Reset(GroundSampleCount + 2);

	const FVector DroneLocation = OwnerPawn->GetActorLocation();
	SamplePoints.Add(DroneLocation);

	// 이동 예측 지점 - 자기 Velocity 방향, 정지 상태면 폰 forward 사용
	FVector ForwardXY = Velocity.GetSafeNormal2D();
	if (ForwardXY.IsNearlyZero())
	{
		ForwardXY = OwnerPawn->GetActorForwardVector().GetSafeNormal2D();
	}

	const FVector LookAheadPoint = DroneLocation + ForwardXY * GroundLookAheadDistance;
	SamplePoints.Add(LookAheadPoint);

	// 드론 주변 원형 샘플 배치 (360도 균등 분할)
	const float AngleStep = 360.f / GroundSampleCount;
	for (int32 i = 0; i < GroundSampleCount; ++i)
	{
		const float Angle = i * AngleStep;
		const FVector Dir = FRotator(0.f, Angle, 0.f).Vector();
		const FVector SamplePoint = DroneLocation + Dir * GroundSampleRadius;
		SamplePoints.Add(SamplePoint);
	}

	// 처음 들어오는 값을 무조건 갱신하기 위해 가장 낮은 값으로 초기화
	float HighestZ = TNumericLimits<float>::Lowest();
	bool bFound = false;

	// 각 샘플에서 아래로 트레이스 후 가장 높은 지형 Z 갱신
	for (const FVector& Point : SamplePoints)
	{
		float HitZ;
		if (TraceGroundAt(Point, HitZ))
		{
			bFound = true;
			HighestZ = FMath::Max(HighestZ, HitZ);
		}
	}

	OutGroundZ = HighestZ;
	return bFound;
}
#pragma endregion

#pragma region Steering
void UNSFlyingLocomotionComponent::InitSteeringDirections()
{
	// XY 평면을 균등 분할한 방향 벡터 배열 사전 계산
	SteeringDirections.Reset(NumSteeringDirections);
	const float AngleStep = 360.0f / NumSteeringDirections;

	for (int32 i = 0; i < NumSteeringDirections; ++i)
	{
		const float Angle = i * AngleStep;
		const FRotator Rotation = FRotator(0.f, Angle, 0.f);
		SteeringDirections.Add(Rotation.Vector());
	}
}

void UNSFlyingLocomotionComponent::BuildInterestMap(const FVector& DesiredDirection)
{
	// 각 방향과 목표 방향의 내적으로 관심도 산출
	InterestMap.Reset(SteeringDirections.Num());

	for (const FVector& Direction : SteeringDirections)
	{
		const float Interest = FVector::DotProduct(Direction, DesiredDirection);
		InterestMap.Add(Interest);
	}
}

void UNSFlyingLocomotionComponent::BuildDangerMap()
{
	const APawn* OwnerPawn = GetPawnOwner();
	if (!IsValid(OwnerPawn)) return;

	// 스무딩 맵 크기 동기화 (스티어링 방향 갯수와 맞춤)
	if (SmoothedDangerMap.Num() != SteeringDirections.Num())
	{
		SmoothedDangerMap.SetNumZeroed(SteeringDirections.Num());
	}

	DangerMap.Reset(SteeringDirections.Num());

	const float DeltaTime = GetWorld()->GetDeltaSeconds();

	TArray<AActor*> IgnoredAllies;
	CollectAvoidanceIgnoredActors(OwnerPawn, IgnoredAllies);

	// 방향별 스윕 → raw danger → 시간 스무딩
	for (int32 i = 0; i < SteeringDirections.Num(); ++i)
	{
		const FVector& Direction = SteeringDirections[i];
		const FVector Start = OwnerPawn->GetActorLocation();
		const FVector End = Start + Direction * AvoidanceTraceDistance;

		FHitResult Hit;
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(OwnerPawn);
		CollisionParams.AddIgnoredActors(IgnoredAllies);
		CollisionParams.bTraceComplex = false;

		// 이번 프레임 raw Danger 계산
		// 회피 채널은 지형 미응답이므로 walkable 판정 불필요
		// 히트하면 곧 진짜 장애물 - 거리 기반 위험도만 계산
		float RawDanger = 0.f;
		if (GetWorld()->SweepSingleByChannel(
			Hit, Start, End, FQuat::Identity,
			AvoidanceChannel,
			FCollisionShape::MakeSphere(AvoidanceTraceRadius),
			CollisionParams))
		{
			// 완만한 경사면(=지형)은 회피 대상이 아님. 가파른 면(=벽/장애물)만 위험도 부여.
			// 파고든 상태 시작 히트는 법선 신뢰 불가 → 보수적으로 장애물 취급
			const float WalkableCosThreshold = FMath::Cos(FMath::DegreesToRadians(MaxWalkableSlopeAngle));
			const bool bWalkableTerrain = !Hit.bStartPenetrating && (Hit.ImpactNormal.Z >= WalkableCosThreshold);

			if (!bWalkableTerrain)
			{
				RawDanger = 1.f - FMath::Clamp(Hit.Distance / AvoidanceTraceDistance, 0.f, 1.f);
			}
		}

		DangerMap.Add(RawDanger);

		// 프레임간 튐 억제를 위한 지수 스무딩 (프레임레이트 독립)
		// Alpha = Clamp(Rate * dt) 형태로 프레임레이트 영향 완화
		const float Alpha = FMath::Clamp(DangerResponseRate * DeltaTime, 0.f, 1.f);
		SmoothedDangerMap[i] = FMath::Lerp(SmoothedDangerMap[i], RawDanger, Alpha);
	}
}

FVector UNSFlyingLocomotionComponent::ChooseSteeringDirection() const
{
	// 위험도 임계 이하 방향 중 관심도가 최고인 것 선택
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

float UNSFlyingLocomotionComponent::ComputeSlopeSpeedScale() const
{
	// 앞쪽 지형 예측이 없으면 감속 없음
	if (!bHasValidGround) return 1.f;

	const APawn* OwnerPawn = GetPawnOwner();
	if (!IsValid(OwnerPawn)) return 1.f;

	// 이동 방향 - 자기 Velocity 우선, 정지 상태면 폰 forward
	FVector ForwardXY = Velocity.GetSafeNormal2D();
	if (ForwardXY.IsNearlyZero())
	{
		ForwardXY = OwnerPawn->GetActorForwardVector().GetSafeNormal2D();
	}

	const FVector LookAheadPoint = OwnerPawn->GetActorLocation() + ForwardXY * GroundLookAheadDistance;

	float LookAheadZ;
	if (!TraceGroundAt(LookAheadPoint, LookAheadZ)) return 1.f;

	// 이상적 고도 대비 현재 고도의 갭 계산
	const float IdealHeightAtLookAhead = LookAheadZ + Altitude;
	const float CurrentZ = OwnerPawn->GetActorLocation().Z;
	const float HeightGap = IdealHeightAtLookAhead - CurrentZ;

	// 내리막이거나 이미 충분히 높으면 감속 불필요
	if (HeightGap <= 0.f) return 1.f;

	// look-ahead 거리 대비 상승 필요량으로 경사각 계산
	const float SlopeAngleRad = FMath::Atan2(HeightGap, GroundLookAheadDistance);
	const float MaxClimbAngleRad = FMath::DegreesToRadians(MaxClimbAngle);

	// 등반 한계 이하면 풀속
	if (SlopeAngleRad <= MaxClimbAngleRad) return 1.f;

	// 초과분에 비례해 감속 (한계각의 1.5배가 되면 30%까지 떨어짐)
	const float ExcessRatio = SlopeAngleRad / MaxClimbAngleRad;
	return FMath::Clamp(1.f / ExcessRatio, 0.3f, 1.f);
}

FVector UNSFlyingLocomotionComponent::ChooseRetreatDirection() const
{
	// 가장 위험도가 낮은 방향 선택 (Max 초기값으로 시작)
	float LowerDanger = TNumericLimits<float>::Max();
	int32 BestIndex = INDEX_NONE;

	// 주변 장애물 기록들 순회
	for (int32 i = 0; i < NumSteeringDirections; ++i)
	{
		// 현재 인덱스의 위험도가 지금까지 최저값보다 낮으면 교체
		if (DangerMap[i] < LowerDanger)
		{
			LowerDanger = DangerMap[i];
			BestIndex = i;
		}
	}

	// 갈 수 있는 공간이 전혀 없다면 (모든 방향이 완전 차단)
	if (BestIndex == INDEX_NONE || LowerDanger >= 1.f)
	{
		return FVector::ZeroVector;
	}

	// 이동 가능한 루트 반환
	return SteeringDirections[BestIndex];
}
#pragma endregion

#pragma region ScriptedMove

void UNSFlyingLocomotionComponent::CollectAvoidanceIgnoredActors(const APawn* OwnerPawn,
	TArray<AActor*>& OutIgnored) const
{
	const IGenericTeamAgentInterface* OwnerTeamAgent = Cast<IGenericTeamAgentInterface>(OwnerPawn);
	if (!OwnerTeamAgent) return;

	const FGenericTeamId OwnerTeamId = OwnerTeamAgent->GetGenericTeamId();
	const UClass* OwnerClass = OwnerPawn->GetClass();
	const FVector OwnerLocation = OwnerPawn->GetActorLocation();

	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* OtherPawn = *It;
		if (OtherPawn == OwnerPawn) continue;
		if (FVector::DistSquared(OwnerLocation, OtherPawn->GetActorLocation()) > FMath::Square(AvoidanceTraceDistance)) continue;

		const IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(OtherPawn);
		const bool bSameTeam = OtherTeamAgent && OtherTeamAgent->GetGenericTeamId() == OwnerTeamId;
		const bool bSameClass = OtherPawn->GetClass() == OwnerClass;

		if (bSameTeam || bSameClass)
		{
			OutIgnored.Add(OtherPawn);
		}
	}
}

void UNSFlyingLocomotionComponent::BeginScriptedMove(const FVector& DestXY, float TargetAltitude, float Speed)
{
	if (!bScriptedMove)
	{
		CachedAltitude = Altitude;
		CachedMaxSpeed = MaxSpeed;
		CachedRotationTarget = RotationTarget;
		CachedAcceleration = Acceleration;
		CachedMaxClimbSpeed = MaxClimbSpeed;
	}
	
	Altitude = TargetAltitude;
	MaxSpeed = Speed;
	Acceleration = Speed * 5.f;
	MaxClimbSpeed = Speed;
	RotationTarget = nullptr;
	ScriptedDestXY = DestXY;
	bScriptedMove = true;
	
	// 진입 시점 1회 트레이스로 절대 목표 Z 확정 (이후 라이브 지면샘플 배제)
	APawn* OwnerPawn = GetPawnOwner();
	float FloorZ = 0.f;
	if (IsValid(OwnerPawn) && TraceGroundAt(OwnerPawn->GetActorLocation(), FloorZ))
	{
		ScriptedTargetZ = FloorZ + TargetAltitude;
	}
	else if (IsValid(OwnerPawn))
	{
		ScriptedTargetZ = OwnerPawn->GetActorLocation().Z;
		UE_LOG(LogTemp, Warning, TEXT("[%s] BeginScriptedMove: 지면 트레이스 실패, 현재 고도로 폴백"),
			*GetNameSafe(OwnerPawn));
	}
}

void UNSFlyingLocomotionComponent::EndScriptedMove()
{
	Altitude = CachedAltitude;
	MaxSpeed = CachedMaxSpeed;
	Acceleration = CachedAcceleration;
	MaxClimbSpeed = CachedMaxClimbSpeed;
	RotationTarget = CachedRotationTarget;
	bScriptedMove = false;
	ResetLocomotionState();
}

bool UNSFlyingLocomotionComponent::HasReachedScriptedDest() const
{
	// 오너가 없다면 false
	if (!PawnOwner) return false;

	// 이 공격패턴을 정의할때 캐싱해둔 도착XY값을 통해 실제로 도착했는지 Bool 값받기
	const bool bXYReached = HasReachedLocation(ScriptedDestXY);
	// 실행 높이와 현재 오너의 높이가 얼마나 차이가 있는지 계산
	const float AltDiff = FMath::Abs(PawnOwner->GetActorLocation().Z - ScriptedTargetZ);

	// 도착하지 못했다면 리턴
	if (!bXYReached) return false;
	
	// 고도 차이가 데드존 이내라면 목표 고도에 도달한 것으로 판단
	return AltDiff < AltitudeDeadZone;
}

void UNSFlyingLocomotionComponent::ApplyScriptedMoveInput(float DeltaSeconds)
{
	if (!PawnOwner) return;
	FVector Dist = ScriptedDestXY - PawnOwner->GetActorLocation();
	Dist.Z = 0.f;
	
	const FVector Direction2D = Dist.GetSafeNormal2D();
	
	if (Dist.SizeSquared() <= FMath::Square(ArrivalRadius)) return;
	
	PawnOwner->AddMovementInput(Direction2D, 1.f);
}

void UNSFlyingLocomotionComponent::DriveScriptedAltitude(float DeltaSeconds)
{
	// 가드 체크
	if (!HasAuthorityChecked()) return;
	
	// 오너 Pawn 캐스팅
	APawn* OwnerPawn = GetPawnOwner();
	if (!IsValid(OwnerPawn)) return;

	// 목표고도 - 현재고도 = 현재위치로부터 목표위치까지의 거리 계산
	const float DesiredMoveDis = ScriptedTargetZ - OwnerPawn->GetActorLocation().Z;
	// 고도차가 너무 낮아서 떨리는 데드존 방지 음수면 실행x 
	if (FMath::Abs(DesiredMoveDis) > AltitudeDeadZone)
	{
		// 목표 고도 를 허용고도거리로 나눈다음 내려가야하는지 올라가야하는지 클램프를통해 힘 반환
		const float InputZ = FMath::Clamp(DesiredMoveDis / AltitudeCorrectionRange, -1.f, 1.f);
		// 실제 고도로 움직임 실행
		OwnerPawn->AddMovementInput(FVector::UpVector, InputZ);
	}
}

#pragma endregion
