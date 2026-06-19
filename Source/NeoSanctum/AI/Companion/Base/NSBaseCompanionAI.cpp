// Copyright 2026 One Team. All rights reserved.

#include "NSBaseCompanionAI.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "NeoSanctum/GAS/AttributeSet/NSCompanionAttributeSet.h"
#include "NeoSanctum/Data/AI/NSCompanionAbilitySet.h"
#include "NeoSanctum/Data/AI/NSCompanionDefinition.h"

ANSBaseCompanionAI::ANSBaseCompanionAI()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// AIController 자동빙의
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ANSDroneAIController::StaticClass();
	
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
	CompanionAttributeSet = CreateDefaultSubobject<UNSCompanionAttributeSet>("AttributeSet");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	
	bReplicates = true;
	SetReplicateMovement(true);
}

UAbilitySystemComponent* ANSBaseCompanionAI::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ANSBaseCompanionAI::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		MaintainAltitude(DeltaSeconds);
		DroneAIRotate(DeltaSeconds);
	}
}

void ANSBaseCompanionAI::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ANSBaseCompanionAI::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	if (!HasAuthority() || !IsValid(NewController)) return;
	
	ANSDroneAIController* DroneAIController = Cast<ANSDroneAIController>(NewController);
	if (!IsValid(DroneAIController)) return;
	
	CachedAIController = DroneAIController;
	
	GetWorldTimerManager().SetTimer(
		CheckDistanceToOwnerTimer,
		this,
		&ANSBaseCompanionAI::CheckDistanceToOwner,
		0.25f,
		true);
	
	
	InitAbilityActorInfo();
	InitializeDefaultStats();
	GiveDefaultAbilities();
}

void ANSBaseCompanionAI::BeginPlay()
{
	Super::BeginPlay();
	
	InitSteeringDirections();
	
	InitAbilityActorInfo();
}

void ANSBaseCompanionAI::MoveTowards(const FVector& TargetLocation)
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

void ANSBaseCompanionAI::CheckDistanceToOwner()
{
	if (!OwnerPlayer) return;
	
	// 오너와 거리 계산
	if (FVector::DistSquared(GetActorLocation(), OwnerPlayer->GetActorLocation()) > FMath::Square(MaxDistance))
	{
		// 오너 쪽 순간이동
		// @TODO 민재 : 재화 탐색으로 인한 거리 멀어질시 텔포x
		TeleportToOwner();
	}
}

void ANSBaseCompanionAI::TeleportToOwner()
{
	// 서버 및 오너 존재 체크
	if (!HasAuthority() || !OwnerPlayer) return;
	
	// Owner도착 지점 값 가져오기
	const FVector Target = OwnerPlayer->GetActorLocation();
	
	// 텔레포트전 이동속도 0 세팅
	if (FloatingPawnMovementComponent)
	{
		FloatingPawnMovementComponent->Velocity = FVector::ZeroVector;
	}
	
	// 텔레포트 적용
	SetActorLocation(Target, false, nullptr, ETeleportType::TeleportPhysics);
	
	bHasValidGround = false;
	
}

void ANSBaseCompanionAI::SetOwnerPlayer(AActor* Actor)
{
	if (!IsValid(Actor)) return;
	
	OwnerPlayer = Actor;
}

void ANSBaseCompanionAI::InitAbilityActorInfo()
{
	checkf(AbilitySystemComponent, TEXT("Can't Found ASC %s"), *GetName());
	
	
	AbilitySystemComponent->InitAbilityActorInfo(this,this);
	
}

void ANSBaseCompanionAI::InitializeDefaultStats()
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

void ANSBaseCompanionAI::GiveDefaultAbilities()
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

void ANSBaseCompanionAI::ApplyDroneDefinition(UNSCompanionDefinition* NewDefinition)
{
	if (!HasAuthority() || !NewDefinition) return;
	if (CurrentDefinition == NewDefinition) return;
	if (!IsValid(NewDefinition->AbilitySet)) return;
	
	CurrentAbilityHandles.TakeFromAbilitySystem(AbilitySystemComponent);
	
	NewDefinition->AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, &CurrentAbilityHandles, this);
	
	FGameplayEffectContextHandle ContextHandle =
	AbilitySystemComponent->MakeEffectContext();
	
	FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(
		NewDefinition->DefaultStatsEffect,
		1.f,
		ContextHandle
		);
	
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
	
	if (NewDefinition->CompanionMesh.Get() != nullptr)
	{
		SkeletalMeshComponent->SetSkeletalMesh(NewDefinition->CompanionMesh.Get());
	}
	else
	{
		const TSoftObjectPtr<USkeletalMesh> MeshToLoad = NewDefinition->CompanionMesh;
		
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
	
	CurrentDefinition = NewDefinition;
}

void ANSBaseCompanionAI::ApplyStatUpgrade(FGameplayTag NodeTag, int32 NewLevel)
{
	// 서버 체크
	if (!HasAuthority() || !CurrentDefinition) return;
	
	// 현재 업그레이드 정보 받아오기 순회
	for (const FNSCompanionUpgradeNode& CurrentUpgradeNode : CurrentDefinition->UpgradeNodes)
	{
		// 업그레이드가 존재하는 노드 태그 찾기
		if (CurrentUpgradeNode.NodeTag != NodeTag) continue;
		
		// 들어온 업그레이드 레벨 값 정상화 방어 코드
		NewLevel = FMath::Clamp(NewLevel, 0, CurrentUpgradeNode.MaxLevel);
		// 실제 적용 수치 
		float ApplyStat = NewLevel * CurrentUpgradeNode.MagnitudePerLevel;
		
		// 현재 적용중인 업그레이드 수치가 있는지 확인
		if (FActiveGameplayEffectHandle* BeforeEffectHandle = StatUpgradeHandles.Find(CurrentUpgradeNode.NodeTag))
		{
			// 있다면 모두 제거
			AbilitySystemComponent->RemoveActiveGameplayEffect(*BeforeEffectHandle);
		}
		
		if (NewLevel == 0)
		{
			StatUpgradeHandles.Remove(CurrentUpgradeNode.NodeTag);
			break;
		}
		
		// 새로운 contextHandle
		FGameplayEffectContextHandle ContextHandle =
				AbilitySystemComponent->MakeEffectContext();
			
		// 새로운 SpecHandle
		FGameplayEffectSpecHandle UpgradeSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		CurrentUpgradeNode.UpgradeEffect,
		NewLevel,
		ContextHandle);
			
		// SpecHandle을 만드는데 성공했다면
		if (UpgradeSpecHandle.IsValid())
		{
			// setsetbycaller로 값 저장 후 현재 업그레이드 Map에 추가
			UpgradeSpecHandle.Data->SetSetByCallerMagnitude(CurrentUpgradeNode.SetByCallerTag, ApplyStat);
			
			StatUpgradeHandles.Add(
			NodeTag,
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*UpgradeSpecHandle.Data.Get())
			);
		}
		
		break;
	}
}

void ANSBaseCompanionAI::MaintainAltitude(float DeltaSeconds)
{
	float OutZ;
	
	if(SampleHighestGround(OutZ))
	{
		float RawTarget = OutZ + Altitude;
		if (!bHasValidGround)
		{
			SmoothedTargetHeight = RawTarget;
			bHasValidGround = true;
		}
		else
		{
			float DesiredPoint = RawTarget - SmoothedTargetHeight;
			SmoothedTargetHeight += FMath::Clamp(DesiredPoint, -MaxDescendSpeed * DeltaSeconds, MaxClimbSpeed * DeltaSeconds);
		}
		
		const float DesiredMoveDis = SmoothedTargetHeight - GetActorLocation().Z;
		if (FMath::Abs(DesiredMoveDis) > AltitudeDeadZone)
		{
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

bool ANSBaseCompanionAI::TraceGroundAt(const FVector& WorldXY, float& OutZ) const
{
	const FVector StartWorldLocation = WorldXY;
	const FVector EndWorldLocation = WorldXY - FVector(0.f, 0.f, GroundTraceDistance);
	
	FHitResult Hit;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	CollisionParams.AddIgnoredActor(OwnerPlayer);
	
	if (GetWorld()->LineTraceSingleByChannel(Hit, StartWorldLocation, EndWorldLocation, ECC_Visibility, CollisionParams))
	{
		OutZ = Hit.ImpactPoint.Z;
		return true;
	}
	return false;
}

bool ANSBaseCompanionAI::SampleHighestGround(float& OutGroundZ) const
{
	if (!FloatingPawnMovementComponent) return false;
	
	TArray<FVector> SamplePoints;
	SamplePoints.Reset(GroundSampleCount + 2);
        
	const FVector DroneLocation = GetActorLocation();
	SamplePoints.Add(DroneLocation);
	
	FVector ForwardXY = FloatingPawnMovementComponent->Velocity.GetSafeNormal2D();
	
	if (ForwardXY.IsNearlyZero())
	{
		ForwardXY = GetActorForwardVector().GetSafeNormal2D();
	}
	
	const FVector LookAheadPoint = DroneLocation + ForwardXY * GroundLookAheadDistance;
	SamplePoints.Add(LookAheadPoint);
	
	const float AngleStep = 360.f / GroundSampleCount;
	
	for (int32 i = 0; i < GroundSampleCount; ++i)
	{
		const float Angle = i * AngleStep;
        const FVector Dir = FRotator(0.f, Angle, 0.f).Vector();
		const FVector SamplePoint = DroneLocation + Dir * GroundSampleRadius;
		SamplePoints.Add(SamplePoint);
	}
	
	float HighestZ = TNumericLimits<float>::Lowest();
	bool bFound = false;
	
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

void ANSBaseCompanionAI::DroneAIRotate(float DeltaSeconds)
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

void ANSBaseCompanionAI::InitSteeringDirections()
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

void ANSBaseCompanionAI::BuildInterestMap(const FVector& DesiredDirection)
{
	InterestMap.Reset(SteeringDirections.Num());
	
	for (const FVector& Direction : SteeringDirections)
	{
		float Interest = FVector::DotProduct(Direction,DesiredDirection);
		InterestMap.Add(Interest);
	}
	
}

void ANSBaseCompanionAI::BuildDangerMap()
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

bool ANSBaseCompanionAI::IsWalkableSurface(const FVector& SurfaceNormal) const
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

FVector ANSBaseCompanionAI::ChooseSteeringDirection() const
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
