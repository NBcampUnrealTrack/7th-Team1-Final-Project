// Copyright 2026 One Team. All rights reserved.

#include "NSSpectatorViewComponent.h"

#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"
#include "Net/UnrealNetwork.h"

UNSSpectatorViewComponent::UNSSpectatorViewComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNSSpectatorViewComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!SourceCamera)
	{
		SourceCamera = GetOwner() ? GetOwner()->FindComponentByClass<UCameraComponent>() : nullptr;
	}

	// Tick에서 하지 않고 타이머로 일정 시간마다 카메라 정보 전송
	if (AActor* OwnerActor = GetOwner())
	{
		if (OwnerActor->HasAuthority() || OwnerActor->GetLocalRole() == ROLE_AutonomousProxy)
		{
			GetWorld()->GetTimerManager().SetTimer(
				UpdateTimerHandle,
				this,
				&ThisClass::UpdateSpectatorPOV,
				UpdateInterval,
				true
			);
		}
	}
}

void UNSSpectatorViewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UNSSpectatorViewComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNSSpectatorViewComponent, ReplicatedPOV);
}

void UNSSpectatorViewComponent::SetSourceCamera(UCameraComponent* NewSourceCamera)
{
	SourceCamera = NewSourceCamera;
}

void UNSSpectatorViewComponent::UpdateSpectatorPOV()
{
	if (!SourceCamera)
	{
		return;
	}

	const FNSReplicatedSpectatorPOV NewPOV = CaptureCurrentPOV();
	if (AActor* OwnerActor = GetOwner(); OwnerActor && OwnerActor->HasAuthority())
	{
		ReplicatedPOV = NewPOV;
	}
	else
	{
		Server_UpdateSpectatorPOV(NewPOV);
	}
}

FNSReplicatedSpectatorPOV UNSSpectatorViewComponent::CaptureCurrentPOV() const
{
	// 보낼 카메라 정보 구조체
	FNSReplicatedSpectatorPOV Result;
	if (!SourceCamera)
	{
		return Result;
	}
	
	// Location, Rotation, FOV 정도는 언리얼에서 기본적으로 구조체를 제공한다
	FMinimalViewInfo ViewInfo;
	SourceCamera->GetCameraView(0.f, ViewInfo);
	
	Result.Location = ViewInfo.Location;
	Result.Rotation = ViewInfo.Rotation;
	Result.FOV = ViewInfo.FOV;
	return Result;
}

void UNSSpectatorViewComponent::Server_UpdateSpectatorPOV_Implementation(const FNSReplicatedSpectatorPOV& NewPOV)
{
	ReplicatedPOV = NewPOV;
}
