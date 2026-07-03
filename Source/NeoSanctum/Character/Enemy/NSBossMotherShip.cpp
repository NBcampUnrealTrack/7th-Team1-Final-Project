// Copyright 2026 One Team. All rights reserved.


#include "NSBossMotherShip.h"

#include "AbilitySystemComponent.h"
#include "NSEnemyDrone.h"
#include "GameFramework/GameStateBase.h"
#include "NeoSanctum/AI/Components/NSFlyingLocomotionComponent.h"
#include "NeoSanctum/AI/Manager/NSDronePoolManager.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSGameFlowSubsystem.h"

ANSBossMotherShip::ANSBossMotherShip()
{
	PrimaryActorTick.bCanEverTick = false;

	FlyingLocomotionComponent = CreateDefaultSubobject<UNSFlyingLocomotionComponent>(FName("FlyingMovementComponent"));
}

void ANSBossMotherShip::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		DronePool = NewObject<UNSDronePoolManager>(this);   // Outer=보스 → GetWorld 유효, 수명 보스에 종속
		StartDroneSpawnLoop();  //(전투 진입 시점에 켜고 싶으면 여기 말고 그 콜백에서)
	}
}

void ANSBossMotherShip::StartDroneSpawnLoop()
{
	if (!HasAuthority()) return;
	
	GetWorldTimerManager().SetTimer(
		DroneSpawnTimerHandle, this, &ANSBossMotherShip::TickDroneSpawn, SpawnInterval, true);
}

void ANSBossMotherShip::StopDroneSpawnLoop()
{
	GetWorldTimerManager().ClearTimer(DroneSpawnTimerHandle);
}

void ANSBossMotherShip::TickDroneSpawn()
{
	if (IsDead())
	{
		StopDroneSpawnLoop();
		return;
	}
	
	PruneActiveDrones();
	
	if (GetPhaseComponent() && GetPhaseComponent()->IsPatternLocked()) return;
	
	if (ActiveDrones.Num() >= GetCurrentMaxDrones()) return;
	
	UAbilitySystemComponent* MotherShipASC = GetAbilitySystemComponent();
	if (!MotherShipASC) return;
	MotherShipASC->TryActivateAbilityByClass(SpawnDroneAbilityClass);
}

void ANSBossMotherShip::SpawnEnemyDrone()
{
	if (!HasAuthority()) return;
	
	PruneActiveDrones();
	
	const int32 Deficit = GetCurrentMaxDrones() - ActiveDrones.Num();
	if (Deficit <= 0) return;
	
	TArray<FTransform> SpawnTransforms;
	if (!BuildDroneSpawnTransforms(Deficit, SpawnTransforms)) return;
	
	for (const FTransform& Transform : SpawnTransforms)
	{
		if (ANSEnemyDrone* Drone = AcquireDroneFromPool(Transform))
		{
			ActiveDrones.Add(Drone);
		}
	}
}

int32 ANSBossMotherShip::GetCurrentMaxDrones() const
{
	if (GetPhaseComponent())
	{
		const FGameplayTag PhaseTag = GetPhaseComponent()->GetCurrentPhaseTag();
		if (const int32* Max = MaxDronesByPhase.Find(PhaseTag))
		{
			return *Max;
		}
	}
	
	return DefaultMaxDrones;
}

void ANSBossMotherShip::PruneActiveDrones()
{
	for (int32 i = ActiveDrones.Num() - 1; i >= 0; i--)
	{
		ANSEnemyDrone* Drone = ActiveDrones[i].Get();
		if (!IsValid(Drone))
		{
			ActiveDrones.RemoveAt(i);
		}
		else if (Drone->IsDead())
		{
			if (DronePool) DronePool->ReturnDroneToPool(Drone);
			ActiveDrones.RemoveAt(i);
		}
	}
}

bool ANSBossMotherShip::BuildDroneSpawnTransforms(int32 Count, TArray<FTransform>& OutTransforms) const
{
	if (Count <= 0) return false;

	// 소켓 보유시 우선 사용
	if (!SpawnSocketNames.IsEmpty() && IsValid(EnemyMesh))
	{
		for (const FName& SocketName : SpawnSocketNames)
		{
			if (EnemyMesh->DoesSocketExist(SocketName))
			{
				OutTransforms.Add(EnemyMesh->GetSocketTransform(SocketName));
				if (OutTransforms.Num() >= Count) break;
			}
		}
		if (!OutTransforms.IsEmpty()) return true;
	}

	// 소켓이 없다면 fallBack
	if (SpawnRingRadius <= 0.f) return false;

	const FVector Center = GetActorLocation();
	const float AngleStep = 360.f / Count;   // 반지름과 별개의 '각도 간격(도)'

	for (int32 i = 0; i < Count; ++i)
	{
		const float Yaw = AngleStep * i;

		const FVector OffSet = FRotator(0.f, Yaw, 0.f).RotateVector(FVector(SpawnRingRadius, 0.f, 0.f));
		const FVector Location = Center + OffSet;

		const FRotator Rotation = FRotator(0.f, Yaw, 0.f);

		OutTransforms.Emplace(Rotation, Location, FVector::OneVector);
	}

	return true;
}

ANSEnemyDrone* ANSBossMotherShip::AcquireDroneFromPool(const FTransform& SpawnTransform)
{
	if (!DronePool || !DroneClass || !DroneEnemyData) return nullptr;
	
	int32 PlayerCount = GetWorld()->GetGameState() ? GetWorld()->GetGameState()->PlayerArray.Num() : 1;
	UNSGameFlowSubsystem* Flow = GetGameInstance()->GetSubsystem<UNSGameFlowSubsystem>();
	FNSDifficultyScale Scale = Flow ? Flow->GetCurrentMonsterScale(PlayerCount) : FNSDifficultyScale();
	
	return DronePool->GetPooledDrone(DroneClass, DroneEnemyData, SpawnTransform.GetLocation(), SpawnTransform.Rotator(), Scale);
}
