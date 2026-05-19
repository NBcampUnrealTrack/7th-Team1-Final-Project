// Copyright 2026 One Team. All rights reserved.


#include "NSDroneAI.h"
#include "AIController.h"
#include "EngineUtils.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"

ANSDroneAI::ANSDroneAI()
{
	PrimaryActorTick.bCanEverTick = false;
	bIsMoving = false;
	TargetRange = 50.f;
}

void ANSDroneAI::BeginPlay()
{
	Super::BeginPlay();
	
	AIController = Cast<AAIController>(GetController());
	
	if (AIController)
	{
		AIController->ReceiveMoveCompleted.RemoveDynamic(this, &ANSDroneAI::OnMoveCompleted);
		AIController->ReceiveMoveCompleted.AddDynamic(this, &ANSDroneAI::OnMoveCompleted);
		
		FindTargetOwninigCharacter(); 
		StartMoving();
	}
}

void ANSDroneAI::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	bIsMoving = false;
	
	if (Result == EPathFollowingResult::Success)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &ANSDroneAI::MoveToTargetOwningCharacter, 0.5f, false);
	}
	else
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &ANSDroneAI::MoveToTargetOwningCharacter, 1.0f, false);
	}
}

void ANSDroneAI::FindTargetOwninigCharacter()
{
	if (!TargetActor)
	{
		for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
		{
			ACharacter* PlayerCharacter = *It;
			if (PlayerCharacter->ActorHasTag(TEXT("Player")))
			{
				TargetOwningCharacter = PlayerCharacter;
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Target Actor Found"));
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Target Actor Not Found"));
			}
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Already Have TargetActor"));
	}
}

void ANSDroneAI::StartMoving()
{
	FindTargetOwninigCharacter();
	MoveToTargetOwningCharacter();
}

void ANSDroneAI::MoveToTargetOwningCharacter()
{
	if (!AIController || bIsMoving)
	{
		return;
	}
	
	if (TargetOwningCharacter)
	{
		bIsMoving = true;
		
		EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(
			TargetOwningCharacter,
			TargetRange,
			true,
			true,
			false,
			ACharacter::StaticClass());
		
		if (MoveResult == EPathFollowingRequestResult::Failed)
		{
			bIsMoving = false;
		}
	}
}



