// Copyright 2026 One Team. All rights reserved.

#include "NSDeathSpectatorPawn.h"

#include "ProceduralDungeonSettings.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Character/Component/NSInputBinderComponent.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Tag/NSGameplayTags_Input.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"

ANSDeathSpectatorPawn::ANSDeathSpectatorPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	
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

	if (!FollowTarget)
	{
		return;
	}

	ApplySpectatorTargetView();

	// 관전 대상 근처로 이동해 Room streaming 기준점 유지
	const FVector NewLocation = FMath::VInterpTo(
		GetActorLocation(),
		FollowTarget->GetActorLocation() + FollowOffset,
		DeltaSeconds,
		FollowInterpSpeed
	);
	SetActorLocation(NewLocation);
}

void ANSDeathSpectatorPawn::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	if (!IsLocallyControlled() || !SpectatorTarget || !CameraComp)
	{
		Super::CalcCamera(DeltaTime, OutResult);
		return;
	}

	// 관전 대상 카메라 POV를 보간해 최종 View로 반환
	FMinimalViewInfo TargetPOV;
	SpectatorTarget->CalcCamera(DeltaTime, TargetPOV);

	if (!bHasSmoothedCameraPOV)
	{
		SmoothedCameraLocation = TargetPOV.Location;
		SmoothedCameraRotation = TargetPOV.Rotation;
		SmoothedCameraFOV = TargetPOV.FOV;
		bHasSmoothedCameraPOV = true;
	}
	else
	{
		SmoothedCameraLocation = FMath::VInterpTo(
			SmoothedCameraLocation,
			TargetPOV.Location,
			DeltaTime,
			CameraLocationInterpSpeed);
		SmoothedCameraRotation = FMath::RInterpTo(
			SmoothedCameraRotation,
			TargetPOV.Rotation,
			DeltaTime,
			CameraRotationInterpSpeed);
		SmoothedCameraFOV = FMath::FInterpTo(
			SmoothedCameraFOV,
			TargetPOV.FOV,
			DeltaTime,
			CameraFOVInterpSpeed);
	}

	OutResult.Location = SmoothedCameraLocation;
	OutResult.Rotation = SmoothedCameraRotation;
	OutResult.FOV = SmoothedCameraFOV;
}

void ANSDeathSpectatorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if (InputBinderComp)
	{
		InputBinderComp->InitializePlayerInput(PlayerInputComponent);
	}
}

void ANSDeathSpectatorPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANSDeathSpectatorPawn, SpectatorTarget);
	DOREPLIFETIME(ANSDeathSpectatorPawn, SpectatorTargetRevision);
}

void ANSDeathSpectatorPawn::SetFollowTarget(AActor* NewFollowTarget)
{
	FollowTarget = NewFollowTarget;
}

void ANSDeathSpectatorPawn::SetSpectatorTarget(ANSPlayerCharacterBase* NewSpectatorTarget)
{
	if (SpectatorTarget != NewSpectatorTarget)
	{
		LastAppliedSpectatorTarget = nullptr;
		bHasSmoothedCameraPOV = false;
	}

	SpectatorTarget = NewSpectatorTarget;
	if (HasAuthority())
	{
		// 같은 대상 재확정 시 클라이언트 View 재적용 보장용 갱신 번호 증가
		++SpectatorTargetRevision;
	}
	SetFollowTarget(NewSpectatorTarget);

	// 서버에서 관전자 Pawn을 대상 주변으로 먼저 이동시켜 스트리밍 기준점 보정
	// 대상 전환 직후 대상 근처 이동으로 복제와 스트리밍 유도
	if (HasAuthority() && NewSpectatorTarget)
	{
		SetActorLocation(NewSpectatorTarget->GetActorLocation() + FollowOffset);
		NewSpectatorTarget->ForceNetUpdate();
		ForceNetUpdate();
	}

	ApplySpectatorTargetView();
}

void ANSDeathSpectatorPawn::OnRep_SpectatorTarget()
{
	RefreshSpectatorTargetView();
}

void ANSDeathSpectatorPawn::OnRep_SpectatorTargetRevision()
{
	RefreshSpectatorTargetView();
}

void ANSDeathSpectatorPawn::RefreshSpectatorTargetView()
{
	if (SpectatorTarget && FollowTarget != SpectatorTarget)
	{
		// 복제 순서 차이로 추적 대상이 비어있을 때 복제된 관전 대상으로 복구
		SetFollowTarget(SpectatorTarget);
	}

	ApplySpectatorTargetView();
}

void ANSDeathSpectatorPawn::ApplySpectatorTargetView()
{
	if (!IsLocallyControlled() || !SpectatorTarget)
	{
		return;
	}

	if (ANSPlayerController* NSPlayerController = Cast<ANSPlayerController>(GetController()))
	{
		if (LastAppliedSpectatorTarget == SpectatorTarget && NSPlayerController->GetViewTarget() == this)
		{
			return;
		}

		LastAppliedSpectatorTarget = SpectatorTarget;
		// 로컬 PlayerController에 최종 관전 ViewTarget 적용 위임
		NSPlayerController->ApplyConfirmedSpectatorTarget(SpectatorTarget);
	}
}
