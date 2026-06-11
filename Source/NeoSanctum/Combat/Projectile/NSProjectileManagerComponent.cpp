// Copyright 2026 One Team. All rights reserved.


#include "NSProjectileManagerComponent.h"

#include "NSProjectileTypes.h"


UNSProjectileManagerComponent::UNSProjectileManagerComponent()
{
	// Tick 기능 사용
	PrimaryComponentTick.bCanEverTick = true;

	// 게임 시작할 때부터 투사체 위치 지속적으로 갱신
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// 이 ActorComponent 자체를 네트워크 복제하지 않음
	SetIsReplicatedByDefault(false);
}

void UNSProjectileManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ProjectileManagerComponent를 소유할 Actor: ANSRunGameState를 지칭함.
	AActor* OwnerActor = GetOwner();

	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		// ANSRunGameState에 붙어 클라이언트에도 이 ActorComponent가 존재하므로 
		// 서버에서만 수행하도록 함.
		SetComponentTickEnabled(false);
		return;
	}

	// 최대 예상 개수만큼 메모리를 미리 확보
	// 발사 중 TArray 재할당이 반복되는 것을 줄임
	ActiveProjectiles.Reserve(MaxActiveProjectiles);
}

bool UNSProjectileManagerComponent::FireProjectile(const FNSProjectileFireRequest& Request)
{
	AActor* OwnerActor = GetOwner();

	// 서버에서만 수행하도록 함.
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	// 활성 투사체가 설정된 최대 개수에 도달하면 실행 X
	if (ActiveProjectiles.Num() >= MaxActiveProjectiles)
	{
		return false;
	}

	// 이동 방향 정규화
	const FVector NormalizedDirection = Request.Direction.GetSafeNormal();

	// 이동할 수 없는 잘못된 요청 거부
	if (NormalizedDirection.IsNearlyZero() ||
		Request.Speed <= 0.0f ||
		Request.MaxLifeTime <= 0.0f)
	{
		return false;
	}

	// 배열에 기본값으로 초기화된 원소를 하나 추가하고 그 원소의 참조를 바로 받음
	FNSServerProjectileData& Projectile = ActiveProjectiles.AddDefaulted_GetRef();

	// 발사 요청을 서버 런타임 데이터로 복사함
	Projectile.CurrentLocation = Request.StartLocation;
	Projectile.Direction = NormalizedDirection;
	Projectile.Speed = Request.Speed;
	Projectile.LifeTime = 0.0f;
	Projectile.MaxLifeTime = Request.MaxLifeTime;

	return true;
}

void UNSProjectileManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* OwnerActor = GetOwner();

	// 서버에서만 수행하도록 함.
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	// 뒤에서 앞으로 순회한다.
	// 순회 중 RemoveAtSwap으로 제거해도 아직 처리하지 않은
	// 배열 인덱스가 영향을 받지 않게 하기 위함이다.
	for (int32 Index = ActiveProjectiles.Num() - 1; Index >= 0; --Index)
	{
		FNSServerProjectileData& Projectile = ActiveProjectiles[Index];

		// 현재 투사체에 남아 있는 수명
		const float RemainingLifeTime = Projectile.MaxLifeTime - Projectile.LifeTime;

		// 마지막 프레임에서 최대 수명을 초과한 시간만큼
		// 더 이동하지 않도록 실제 계산 시간을 제한한다.
		const float SimulationTime = FMath::Min(DeltaTime, RemainingLifeTime);

		// Debug Line의 시작점으로 사용할 이동 전 위치
		const FVector PreviousLocation = Projectile.CurrentLocation;

		Projectile.CurrentLocation += Projectile.Direction * Projectile.Speed * SimulationTime;

		Projectile.LifeTime += SimulationTime;

		if (bDrawDebugTrajectory)
		{
			// 이번 프레임의 이동 구간
			DrawDebugLine(
				World,
				PreviousLocation,
				Projectile.CurrentLocation,
				FColor::Cyan,
				false,
				0.0f,
				0,
				2.0f);

			// 현재 위치 점
			DrawDebugPoint(
				World,
				Projectile.CurrentLocation,
				8.0f,
				FColor::Yellow,
				false,
				0.0f);
		}

		// 최대 수명에 도달한 투사체를 배열에서 제거
		if (Projectile.LifeTime >= Projectile.MaxLifeTime)
		{
			ActiveProjectiles.RemoveAtSwap(
				Index,
				1,
				EAllowShrinking::No);
		}
	}
}
