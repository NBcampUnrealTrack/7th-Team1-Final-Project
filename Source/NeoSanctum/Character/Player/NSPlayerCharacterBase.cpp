// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerCharacterBase.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

ANSPlayerCharacterBase::ANSPlayerCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(RootComponent);
	
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);
}

void ANSPlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void ANSPlayerCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANSPlayerCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

