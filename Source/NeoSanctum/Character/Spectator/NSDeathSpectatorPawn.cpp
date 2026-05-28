// Copyright 2026 One Team. All rights reserved.

#include "NSDeathSpectatorPawn.h"

#include "Camera/CameraComponent.h"
#include "NeoSanctum/Character/Component/NSInputBinderComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Input.h"

ANSDeathSpectatorPawn::ANSDeathSpectatorPawn()
{
	PrimaryActorTick.bCanEverTick = false;
	
	SceneRootComp = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRootComp"));
	SetRootComponent(SceneRootComp);
	
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SceneRootComp);
	
	InputBinderComp = CreateDefaultSubobject<UNSInputBinderComponent>(TEXT("InputBinderComp"));
	
	FGameplayTagContainer SpectatorInputModeTags;
	SpectatorInputModeTags.AddTag(NSGameplayTags::InputMode_DeathSpectator);
	SpectatorInputModeTags.AddTag(NSGameplayTags::InputMode_UI);
	InputBinderComp->SetActiveInputModeTags(SpectatorInputModeTags);
}

void ANSDeathSpectatorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if (InputBinderComp)
	{
		InputBinderComp->InitializePlayerInput(PlayerInputComponent);
	}
}
