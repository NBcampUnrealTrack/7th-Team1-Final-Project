// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/Core/Waypoint/NSWaypointMarkerComponent.h"

#include "Net/UnrealNetwork.h"
#include "NeoSanctum/Core/Waypoint/NSWaypointSubsystem.h"

UNSWaypointMarkerComponent::UNSWaypointMarkerComponent()
{
	// 화면 갱신은 컨테이너 위젯 틱이 담당하므로 컴포넌트 틱 불필요
	PrimaryComponentTick.bCanEverTick = false;

	// bMarkerActive 리플리케이션을 위해 컴포넌트 리플리케이션 활성화
	SetIsReplicatedByDefault(true);
}

void UNSWaypointMarkerComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNSWaypointMarkerComponent, bMarkerActive);
}

void UNSWaypointMarkerComponent::BeginPlay()
{
	Super::BeginPlay();

	// 스폰 시점의 초기 상태를 레지스트리에 반영
	UpdateRegistration();
}

void UNSWaypointMarkerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 액터 파괴/레벨 전환 시 레지스트리에 스테일 항목이 남지 않도록 확실히 해제
	if (UWorld* World = GetWorld())
	{
		if (UNSWaypointSubsystem* Subsystem = World->GetSubsystem<UNSWaypointSubsystem>())
		{
			Subsystem->UnregisterMarker(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UNSWaypointMarkerComponent::SetMarkerActive(bool bNewActive)
{
	// 리플리케이션 원본은 서버이므로 서버 권한에서만 상태 변경 허용
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (bMarkerActive == bNewActive)
	{
		return;
	}

	bMarkerActive = bNewActive;

	// 리슨서버 호스트는 OnRep이 호출되지 않으므로 직접 갱신
	UpdateRegistration();
}

void UNSWaypointMarkerComponent::SetMarkerActiveLocal(bool bNewActive)
{
	// 로컬 전용이므로 권한 확인 불필요 —> 호출한 머신에서만 반영
	if (bMarkerActiveLocal == bNewActive)
	{
		return;
	}

	bMarkerActiveLocal = bNewActive;
	UpdateRegistration();
}

void UNSWaypointMarkerComponent::OnRep_MarkerActive()
{
	// 접속자 클라에서 서버가 바꾼 상태를 로컬 레지스트리에 반영
	UpdateRegistration();
}

void UNSWaypointMarkerComponent::UpdateRegistration()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UNSWaypointSubsystem* Subsystem = World->GetSubsystem<UNSWaypointSubsystem>();
	if (!Subsystem)
	{
		return;
	}

	// 복제 플래그(서버 구동)와 로컬 플래그(클라 구동) 중 하나라도 켜져 있으면 표시
	if (bMarkerActive || bMarkerActiveLocal)
	{
		Subsystem->RegisterMarker(this);
	}
	else
	{
		Subsystem->UnregisterMarker(this);
	}
}

FVector UNSWaypointMarkerComponent::GetMarkerWorldLocation() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ZeroVector;
	}

	return Owner->GetActorLocation() + WorldOffset;
}
