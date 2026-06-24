// Copyright 2026 One Team. All rights reserved.

#include "NSDeathSpectatorPawn.h"

#include "ProceduralDungeonSettings.h"
#include "Camera/CameraComponent.h"
#include "NeoSanctum/Character/Component/NSInputBinderComponent.h"
#include "NeoSanctum/Character/Component/NSSpectatorViewComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Input.h"
#include "Components/SphereComponent.h"

ANSDeathSpectatorPawn::ANSDeathSpectatorPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	SceneRootComp = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRootComp"));
	SetRootComponent(SceneRootComp);
	
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SceneRootComp);
	
	InputBinderComp = CreateDefaultSubobject<UNSInputBinderComponent>(TEXT("InputBinderComp"));
	
	RoomBoundsComp = CreateDefaultSubobject<USphereComponent>(TEXT("RoomBoundsComp"));
	RoomBoundsComp->SetupAttachment(SceneRootComp);
	RoomBoundsComp->InitSphereRadius(34.0f);
	// 물리 콜리전은 필요없으므로 Query로 킴
	RoomBoundsComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);  
	// 기본값은 전부 무시
	RoomBoundsComp->SetCollisionResponseToAllChannels(ECR_Ignore);     
	// 룸 채널만 Overlap
	if (const UProceduralDungeonSettings* Settings = GetDefault<UProceduralDungeonSettings>())
	{
		RoomBoundsComp->SetCollisionResponseToChannel(Settings->RoomObjectType, ECR_Overlap);
	}
	RoomBoundsComp->SetGenerateOverlapEvents(false);  
	
	FGameplayTagContainer SpectatorInputModeTags;
	SpectatorInputModeTags.AddTag(NSGameplayTags::InputMode_DeathSpectator);
	SpectatorInputModeTags.AddTag(NSGameplayTags::InputMode_UI);
	InputBinderComp->SetActiveInputModeTags(SpectatorInputModeTags);
}

void ANSDeathSpectatorPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!TargetSpectatorView)
	{
		return;
	}

	// 컴포넌트의 POV 구조체를 받아와서 지금 Pawn 카메라에 정보를 덮어씀
	const FNSReplicatedSpectatorPOV& TargetPOV = TargetSpectatorView->GetReplicatedPOV();
	if (bSnapToTargetPOV)
	{
		SetActorLocationAndRotation(TargetPOV.Location, TargetPOV.Rotation);
		if (CameraComp)
		{
			CameraComp->SetFieldOfView(TargetPOV.FOV);
		}

		bSnapToTargetPOV = false;
		return;
	}

	const FVector NewLocation = FMath::VInterpTo(
		GetActorLocation(),
		TargetPOV.Location,
		DeltaSeconds,
		LocationInterpSpeed
	);
	const FRotator NewRotation = FMath::RInterpTo(
		GetActorRotation(),
		TargetPOV.Rotation,
		DeltaSeconds,
		RotationInterpSpeed
	);

	SetActorLocationAndRotation(NewLocation, NewRotation);
	if (CameraComp)
	{
		const float NewFOV = FMath::FInterpTo(
			CameraComp->FieldOfView,
			TargetPOV.FOV,
			DeltaSeconds,
			FOVInterpSpeed
		);
		CameraComp->SetFieldOfView(NewFOV);
	}
}

void ANSDeathSpectatorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if (InputBinderComp)
	{
		InputBinderComp->InitializePlayerInput(PlayerInputComponent);
	}
}

void ANSDeathSpectatorPawn::SetSpectatorView(UNSSpectatorViewComponent* NewSpectatorView)
{
	TargetSpectatorView = NewSpectatorView;
	bSnapToTargetPOV = TargetSpectatorView != nullptr;
}
