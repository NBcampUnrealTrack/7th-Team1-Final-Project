// Copyright 2026 One Team. All rights reserved.

#include "NSBaseDroneAI.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "AbilitySystemComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "AIController.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Data/AI/NSCompanionAbilitySet.h"
#include "NeoSanctum/Data/AI/NSBaseDroneDefinition.h"
#include "Net/UnrealNetwork.h"

ANSBaseDroneAI::ANSBaseDroneAI()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// AIController 자동빙의
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	SphereComponent = CreateDefaultSubobject<USphereComponent>("Collision");
	SetRootComponent(SphereComponent);
	SphereComponent->SetSimulatePhysics(false);
	
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMesh");
	SkeletalMeshComponent->SetupAttachment(SphereComponent);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	FloatingPawnMovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>("FloatingPawnMovement");
	FloatingPawnMovementComponent->MaxSpeed = 1000.f;
	FloatingPawnMovementComponent->Acceleration = 4000.f;
	FloatingPawnMovementComponent->Deceleration = 4000.f;
	FloatingPawnMovementComponent->TurningBoost = 8.f;
	
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	
	bReplicates = true;
	SetReplicateMovement(true);
}

UAbilitySystemComponent* ANSBaseDroneAI::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ANSBaseDroneAI::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (HasAuthority())
	{
		DroneAIRotate(DeltaSeconds);

		GroundSampleAccumulator += DeltaSeconds;
		if (GroundSampleAccumulator >= GroundSampleInterval)  // 예: 0.1초
		{
			MaintainAltitude(GroundSampleAccumulator);
			GroundSampleAccumulator = 0.f;
		}
	}
}

void ANSBaseDroneAI::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ANSBaseDroneAI, CurrentDefinition);
}

void ANSBaseDroneAI::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	if (!HasAuthority() || !IsValid(NewController)) return;
	
	AAIController* DroneAIController = Cast<AAIController>(NewController);
	if (!IsValid(DroneAIController)) return;
	CachedAIController = DroneAIController;
	
	InitializeFromData();
}

void ANSBaseDroneAI::BeginPlay()
{
	Super::BeginPlay();
	
	InitSteeringDirections();
	
	InitAbilityActorInfo();
}

void ANSBaseDroneAI::MoveTowards(const FVector& TargetLocation)
{
	FVector TargetPosition = TargetLocation - GetActorLocation();
	TargetPosition.Z = 0.f;
	
	if (TargetPosition.SizeSquared() < FMath::Square(ArrivalRadius)) return;
	
	FVector TargetDirection = TargetPosition.GetSafeNormal();  
	BuildInterestMap(TargetDirection);  
	BuildDangerMap();  
	FVector SteeringDirection = ChooseSteeringDirection();  
  
	AddMovementInput(SteeringDirection, 1.0f);
}

void ANSBaseDroneAI::SetPendingDefinition(const UNSBaseDroneDefinition* InDefinition)
{
	if (!InDefinition) return;
	
	CurrentDefinition = InDefinition;
}

void ANSBaseDroneAI::InitAbilityActorInfo()
{
	checkf(AbilitySystemComponent, TEXT("Can't Found ASC %s"), *GetName());
	
	AbilitySystemComponent->InitAbilityActorInfo(this,this);
}

void ANSBaseDroneAI::InitializeDefaultStats()
{
	if (!HasAuthority()) return;
	
	if (!AbilitySystemComponent || !DefaultStatsEffect)	return;
	
	FGameplayEffectContextHandle ContextHandle =
		AbilitySystemComponent->MakeEffectContext();
	
	FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(
		DefaultStatsEffect,
		1.f,
		ContextHandle
		);
	
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void ANSBaseDroneAI::GiveDefaultAbilities()
{
	if (!HasAuthority() || bDefaultAbilitiesGranted) return;
	
	if (!AbilitySystemComponent) return;
	
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (!AbilityClass) continue;
		
		FGameplayAbilitySpec AbilitySpec(
		AbilityClass,
		1,
		INDEX_NONE,
		this
		);
		
		AbilitySystemComponent->GiveAbility(AbilitySpec);
	}
	
	bDefaultAbilitiesGranted = true;
}

void ANSBaseDroneAI::ApplyDroneDefinition(const UNSBaseDroneDefinition* NewDefinition)
{
	if (!HasAuthority() || !NewDefinition) return;
	
	//@민재 TODO : 예외처리 생각하기
	/*if (CurrentDefinition == NewDefinition) return;
	UE_LOG(LogTemp, Warning, TEXT("CurrentDefinition == NewDefinition"));*/
	
	if (!IsValid(NewDefinition->AbilitySet)) return;
	
	CurrentAbilityHandles.TakeFromAbilitySystem(AbilitySystemComponent);
	
	NewDefinition->AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, &CurrentAbilityHandles, this);
	
	FGameplayEffectContextHandle ContextHandle =
	AbilitySystemComponent->MakeEffectContext();
	
	FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(
		NewDefinition->TypeStatsEffect,
		1.f,
		ContextHandle
		);
	
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
	
	ApplyDroneVisual(NewDefinition);
	
	CurrentDefinition = NewDefinition;
}

void ANSBaseDroneAI::ApplyDroneVisual(const UNSBaseDroneDefinition* NewDefinition)
{
	if (!NewDefinition) return;
	
	if (NewDefinition->Mesh.Get() != nullptr)
	{
		SkeletalMeshComponent->SetSkeletalMesh(NewDefinition->Mesh.Get());
	}
	else
	{
		const TSoftObjectPtr<USkeletalMesh> MeshToLoad = NewDefinition->Mesh;
		
		FStreamableManager& StreamableManager = UAssetManager::Get().GetStreamableManager();
		StreamableManager.RequestAsyncLoad(
			MeshToLoad.ToSoftObjectPath(),
			FStreamableDelegate::CreateWeakLambda(this, [this, MeshToLoad, NewDefinition]()
			{
				if (NewDefinition != CurrentDefinition) return;
				
				if (USkeletalMesh* Loaded = MeshToLoad.Get())
				{
					SkeletalMeshComponent->SetSkeletalMesh(Loaded);
				}
			})
		);
	}
}

void ANSBaseDroneAI::OnRep_CurrentDefinition()
{
	ApplyDroneVisual(CurrentDefinition);
}

void ANSBaseDroneAI::MaintainAltitude(float DeltaSeconds)
{
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
		const float DesiredMoveDis = SmoothedTargetHeight - GetActorLocation().Z;
		if (FMath::Abs(DesiredMoveDis) > AltitudeDeadZone)
		{
			// Clamp를 통해 입력강도 값 -1~1 사이 값으로 변환 후 적용
			const float InputZ = FMath::Clamp(DesiredMoveDis / AltitudeCorrectionRange, -1.f, 1.f);
			AddMovementInput(FVector::UpVector, InputZ);
		}
	}
	
	/*
	const FVector End = Start - FVector(0.f,0.f,GroundTraceDistance);
	
	FHitResult Hit;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_Visibility, CollisionParams))
	{
		const float DesiredHeight = Hit.ImpactPoint.Z + Altitude;
		const float DesiredMoveDistance = DesiredHeight - Start.Z;
		
		if (FMath::Abs(DesiredMoveDistance) > AltitudeDeadZone)
		{
			const float InputZ = FMath::Clamp(DesiredMoveDistance / AltitudeCorrectionRange, -1.f, 1.f);
			AddMovementInput(FVector::UpVector, InputZ);
		}
	}
	*/
}

bool ANSBaseDroneAI::TraceGroundAt(const FVector& WorldXY, float& OutZ) const
{
	// 시작위치 종료위치 설정
	const FVector StartWorldLocation = WorldXY;
	const FVector EndWorldLocation = WorldXY - FVector(0.f, 0.f, GroundTraceDistance);
	
	// 트레이스 정보 입력
	FHitResult Hit;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	
	if (GetWorld()->LineTraceSingleByChannel(Hit, StartWorldLocation, EndWorldLocation, ECC_Visibility, CollisionParams))
	{
		OutZ = Hit.ImpactPoint.Z;
		return true;
	}
	return false;
}

bool ANSBaseDroneAI::SampleHighestGround(float& OutGroundZ) const
{
	if (!FloatingPawnMovementComponent) return false;
	
	// 샘플 포인트 여러개 벡터로 배열
	TArray<FVector> SamplePoints;
	// 지형 정보를 받아올 멤버변수 지형 샘플 갯수 + 드론현재위치 1 + 이동 방향 예측위치 1 만큼 공간확보
	SamplePoints.Reset(GroundSampleCount + 2);
        
	const FVector DroneLocation = GetActorLocation();
	SamplePoints.Add(DroneLocation);
	
	// 드론의 이동 방향 예측
	FVector ForwardXY = FloatingPawnMovementComponent->Velocity.GetSafeNormal2D();
	
	if (ForwardXY.IsNearlyZero())
	{
		ForwardXY = GetActorForwardVector().GetSafeNormal2D();
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

void ANSBaseDroneAI::InitializeFromData()
{
	
}

void ANSBaseDroneAI::DroneAIRotate(float DeltaSeconds)
{
	if (!FloatingPawnMovementComponent) return;
	
	if (!HasAuthority()) return;
	
	const bool bHasEnemy = (GetCurrentEnemy() != nullptr);
	
	FRotator Desired;
	
	if (bHasEnemy)
	{
		FVector ToTarget = CurrentEnemy->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.f;
		if (ToTarget.SizeSquared() < FMath::Square(CombatYawInterpSpeed)) return;
		Desired = FRotator(0.f,ToTarget.Rotation().Yaw,0.f);
	}
	else
	{
		FVector Vel = FloatingPawnMovementComponent->Velocity;
		Vel.Z = 0.f;
		if (Vel.SizeSquared() < FMath::Square(MinSpeedToRotate)) return;
		Desired = FRotator(0.f,Vel.Rotation().Yaw,0.f);
	}
	
	const float InterpSpeed = GetCurrentEnemy() ? CombatYawInterpSpeed : YawInterpSpeed;
	const FRotator NewRot = FMath::RInterpTo(GetActorRotation(), Desired,DeltaSeconds,InterpSpeed);
	SetActorRotation(NewRot);
}

#pragma region 회피기능

void ANSBaseDroneAI::InitSteeringDirections()
{
	float AngleStep = 360.0f / NumSteeringDirections;
	
	for (int32 i = 0; i < NumSteeringDirections; ++i)
	{
		float Angle = i * AngleStep;
		
		FRotator Rotation = FRotator(0.f,Angle,0.f);
		FVector Direction = Rotation.Vector();
		
		SteeringDirections.Add(Direction);
	}
}

void ANSBaseDroneAI::BuildInterestMap(const FVector& DesiredDirection)
{
	InterestMap.Reset(SteeringDirections.Num());
	
	for (const FVector& Direction : SteeringDirections)
	{
		float Interest = FVector::DotProduct(Direction,DesiredDirection);
		InterestMap.Add(Interest);
	}
	
}

void ANSBaseDroneAI::BuildDangerMap()
{
	DangerMap.Reset(SteeringDirections.Num());
	
	for (const FVector& Direction : SteeringDirections)
	{
		const FVector Start = GetActorLocation();
		const FVector End = Start + Direction * AvoidanceTraceDistance;
		
		FHitResult Hit;
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(this);
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

bool ANSBaseDroneAI::IsWalkableSurface(const FVector& SurfaceNormal) const
{
	float SurfaceAndUpVectorDot = FVector::DotProduct(SurfaceNormal,FVector::UpVector);
	float MaxSlopeRadians = FMath::DegreesToRadians(MaxWalkableSlopeAngle);
	float MaxSlopeCos = FMath::Cos(MaxSlopeRadians);
	
	if (SurfaceAndUpVectorDot >= MaxSlopeCos)
	{
		return true;
	}
	
	return false;
}

FVector ANSBaseDroneAI::ChooseSteeringDirection() const
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

#pragma endregion
