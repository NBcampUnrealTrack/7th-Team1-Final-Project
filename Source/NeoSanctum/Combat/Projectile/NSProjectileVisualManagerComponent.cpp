// Copyright 2026 One Team. All rights reserved.


#include "NSProjectileVisualManagerComponent.h"

#include "NSProjectileVisual.h"
#include "GameFramework/GameStateBase.h"

UNSProjectileVisualManagerComponent::UNSProjectileVisualManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UNSProjectileVisualManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		SetComponentTickEnabled(false);
		return;
	}

	// 클라이언트 Proxy에서만 동작
	if (GetOwner()->HasAuthority())
	{
		const APlayerController* OwningController = Cast<APlayerController>(GetOwner()->GetOwner());

		if (!OwningController || !OwningController->IsLocalController())
		{
			SetComponentTickEnabled(false);
		}
	}
}

void UNSProjectileVisualManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TArray<int32> ProjectileIds;
	ActiveVisuals.GetKeys(ProjectileIds);

	for (const int32 ProjectileId : ProjectileIds)
	{
		RemoveVisual(ProjectileId);
	}

	Super::EndPlay(EndPlayReason);
}

void UNSProjectileVisualManagerComponent::HandleSpawnEvent(const FNSProjectileSpawnEvent& SpawnEvent)
{
	if (!VisualClass || SpawnEvent.ProjectileId == INDEX_NONE)
	{
		return;
	}

	RemoveVisual(SpawnEvent.ProjectileId);

	const float ServerTime = GetEstimatedServerTime();
	const float Age = FMath::Max(
		0.0f,
		ServerTime - SpawnEvent.ServerFireTime);

	// RPC 도착 전에 이미 수명이 끝난 투사체는 생성 X
	if (Age >= SpawnEvent.MaxLifeTime)
	{
		return;
	}

	const FVector Location = CalculateLocation(SpawnEvent, ServerTime);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ANSProjectileVisual* VisualActor = GetWorld()->SpawnActor<ANSProjectileVisual>(
		VisualClass,
		Location,
		SpawnEvent.Direction.Rotation(),
		SpawnParameters);

	if (!IsValid(VisualActor))
	{
		return;
	}

	FNSClientProjectileVisualState VisualState;
	VisualState.SpawnEvent = SpawnEvent;
	VisualState.VisualActor = VisualActor;

	ActiveVisuals.Add(SpawnEvent.ProjectileId, MoveTemp(VisualState));
}

void UNSProjectileVisualManagerComponent::HandleEndEvent(const FNSProjectileEndEvent& EndEvent)
{
	RemoveVisual(EndEvent.ProjectileId);
}

void UNSProjectileVisualManagerComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float ServerTime = GetEstimatedServerTime();
	
	// 이번 Tick에서 제거해야 할 투사체 ID Array 
	TArray<int32> ExpiredProjectileIds;

	// 현재 클라이언트가 표시 중인 모든 시각 투사체 순회
	// Pair.Key   = ProjectileId
	// Pair.Value = 해당 투사체의 시각 상태
	for (TPair<int32, FNSClientProjectileVisualState>& Pair : ActiveVisuals)
	{
		FNSClientProjectileVisualState& State = Pair.Value;

		// 서버에서 발사된 이후 현재까지 몇 초가 지났는지 계산
		const float Age = ServerTime - State.SpawnEvent.ServerFireTime;

		// 유효한 시간이나 상태가 아니면 제거할 수 있도록 Id 저장
		if (Age >= State.SpawnEvent.MaxLifeTime || !State.VisualActor.IsValid())
		{
			ExpiredProjectileIds.Add(Pair.Key);
			continue;
		}

		// 발사 위치, 진행 방향, 속도, 경과 시간으로 현재 서버 시간에 해당하는 예상 위치 계산
		const FVector Location = CalculateLocation(State.SpawnEvent, ServerTime);

		// 시각 투사체를 계산된 위치와 발사 방향으로 이동
		State.VisualActor->SetVisualTransform(
			Location,
			State.SpawnEvent.Direction);
	}

	// 유효하지 않은 대상을 순회하며 제거
	for (const int32 ProjectileId : ExpiredProjectileIds)
	{
		RemoveVisual(ProjectileId);
	}
}

float UNSProjectileVisualManagerComponent::GetEstimatedServerTime() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	const AGameStateBase* GameState = World->GetGameState();
	if (GameState)
	{
		/**
		 * 서버에서 동기화된 현재 월드 시간을 반환
		 * 
		 * 클라이언트 내부적으로 추정된 서버 시간 오프셋을 사용하므로
		 * 클라이언트와 서버가 같은 시간 기준으로 투사체 위치를 계산할 수 있다
		 * 120.25f 이런 형태로 값이 반환됨
		 */
		return GameState->GetServerWorldTimeSeconds();
	}

	// GameState를 얻지 못한 경우, 로컬 World 시간을 대체값으로 사용
	return World->GetTimeSeconds();
}

FVector UNSProjectileVisualManagerComponent::CalculateLocation(
	const FNSProjectileSpawnEvent& SpawnEvent, // 서버가 RPC로 전달한 투사체의 발사 정보
	float ServerTime // 클라이언트가 추정한 현재 서버 시간
	) const
{
	// 네트워크 시간 동기화 오차로 현재 서버 시간이 발사 시간보다 작게 계산되는 경우 방지
	const float Age = FMath::Max(
		0.0f,
		ServerTime - SpawnEvent.ServerFireTime);

	// 현재 위치 = 시작 위치 + 진행 방향 * 속도 * 경과 시간
	return FVector(SpawnEvent.StartLocation) + FVector(SpawnEvent.Direction) * SpawnEvent.Speed * Age;
}

void UNSProjectileVisualManagerComponent::RemoveVisual(int32 ProjectileId)
{
	FNSClientProjectileVisualState* State = ActiveVisuals.Find(ProjectileId);

	if (!State)
	{
		return;
	}

	if (State->VisualActor.IsValid())
	{
		State->VisualActor->Destroy();
	}

	ActiveVisuals.Remove(ProjectileId);
}
