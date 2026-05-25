// Copyright 2026 One Team. All rights reserved.


#include "NSDroneAI.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "NeoSanctum/AI/Controller/DroneAI/NSDroneAIController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"

ANSDroneAI::ANSDroneAI()
{
	PrimaryActorTick.bCanEverTick = true;
	
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCharacterMovement()->GravityScale = 0.f;
	GetCharacterMovement()->MaxFlySpeed = 600.f;
	
	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 200.f;
	GetCharacterMovement()->AvoidanceWeight = 0.2f;
	
	GetMesh()->SetEnableGravity(true);
	GetCapsuleComponent()->SetCollisionObjectType(ECC_GameTraceChannel1);
}

void ANSDroneAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	DrawSightDebug();
}

void ANSDroneAI::BeginPlay()
{
	Super::BeginPlay();
	
	DroneAIController = Cast<ANSDroneAIController>(GetController());
	
	if (DroneAIController)
	{
		DroneAIBBComponent = DroneAIController->GetBlackboardComponent();
	}
}


void ANSDroneAI::DrawSightDebug()
{
	ANSDroneAIController* DroneCon = Cast<ANSDroneAIController>(GetController());
	if (!IsValid(DroneCon)) return;

	// 컨트롤러의 멤버변수 직접 접근
	UAISenseConfig_Sight* SightConfig = DroneCon->DroneAISightConfig;
	if (!IsValid(SightConfig)) return;

	FVector  Location     = GetActorLocation();
	FVector  Forward      = GetActorForwardVector();
	float    SightRadius  = SightConfig->SightRadius;
	float    LoseRadius   = SightConfig->LoseSightRadius;
	float    HalfAngleRad = FMath::DegreesToRadians(
								SightConfig->PeripheralVisionAngleDegrees
							);

	// 감지 원뿔 (초록)
	DrawDebugCone(
		GetWorld(),
		Location,
		Forward,
		SightRadius,
		HalfAngleRad,
		HalfAngleRad,
		16,
		FColor::Green,
		false,
		-1.f,
		0,
		1.f
	);

	// 이탈 범위 구체 (노랑 점선)
	DrawDebugSphere(
		GetWorld(),
		Location,
		LoseRadius,
		16,
		FColor::Yellow,
		false,
		-1.f,
		0,
		1.f
	);
}


