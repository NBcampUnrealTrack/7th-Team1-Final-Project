// Copyright 2026 One Team. All rights reserved.


#include "NSBossMotherShip.h"

#include "AbilitySystemComponent.h"
#include "NSEnemyDrone.h"
#include "GameFramework/GameStateBase.h"
#include "NeoSanctum/AI/Components/NSFlyingLocomotionComponent.h"
#include "NeoSanctum/AI/Manager/NSDronePoolManager.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSGameFlowSubsystem.h"

ANSBossMotherShip::ANSBossMotherShip()
{
	PrimaryActorTick.bCanEverTick = false;

	FlyingLocomotionComponent = CreateDefaultSubobject<UNSFlyingLocomotionComponent>(FName("FlyingMovementComponent"));
}

void ANSBossMotherShip::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;
	
	DronePool = NewObject<UNSDronePoolManager>(this);
	StartDronePattern();
	// TODO : 이상적으로는 '전투 진입' 콜백에서 호출. 전투 트리거가 생기면 그쪽으로 옮길 것.
	// === 임시 진단 (테스트 후 제거) ===
	if (UAbilitySystemComponent* MotherShipASC = GetAbilitySystemComponent())
	{
		const FGameplayAbilitySpec* Spec = MotherShipASC->FindAbilitySpecFromClass(SpawnDroneAbilityClass);
		if (!Spec)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Summon] 어빌리티 미부여! EnemyData의 DefaultAbilities/AttackTable 확인"));
		}
		else
		{
			FGameplayTagContainer FailTags;
			const bool bCan = Spec->Ability->CanActivateAbility(
				Spec->Handle, MotherShipASC->AbilityActorInfo.Get(), nullptr, nullptr, &FailTags);
			UE_LOG(LogTemp, Warning, TEXT("[Summon] Granted / CanActivate=%d / Fail=[%s]"),
				bCan, *FailTags.ToStringSimple());
		}
	}
	// === 임시 끝 ===
}

#pragma region SpawnDronePattern

void ANSBossMotherShip::StartDronePattern()
{
	if (!HasAuthority()) return;
	LastObservedPhaseTag = PhaseComponent ? PhaseComponent->GetCurrentPhaseTag() : FGameplayTag();
	QueueFullWave();
	RequestSummonAbility();
	
	GetWorldTimerManager().SetTimer(
		DroneSpawnTimerHandle,
		this,
		&ANSBossMotherShip::MaintenanceTick,
		SpawnInterval,
		true);
}

void ANSBossMotherShip::StopDronePattern()
{
	GetWorldTimerManager().ClearTimer(DroneSpawnTimerHandle);
	bSummonInFlight = false;
}

void ANSBossMotherShip::MaintenanceTick()
{
	if (!HasAuthority() || IsDead())
	{
		StopDronePattern();
		return;
	}
	
	PruneActiveDrones();
	const double Now = GetWorld()->GetTimeSeconds();
	
	const FGameplayTag CurrentTag = PhaseComponent ? PhaseComponent->GetCurrentPhaseTag() : FGameplayTag();
	if (CurrentTag != LastObservedPhaseTag)
	{
		if (LastObservedPhaseTag.IsValid())   // 진짜 전환(P1→P2)만 회수. 빈→P1 초기화는 조용히 통과
		{
			RecallAllDrones();
			bPendingPhaseRefill = true;
		}
		LastObservedPhaseTag = CurrentTag;
	}
	
	if (PhaseComponent && PhaseComponent->IsPatternLocked()) return;
	
	if (bPendingPhaseRefill)
	{
		QueueFullWave();
		bPendingPhaseRefill = false;
	}
	
	if (GetActiveDroneCount() < GetCurrentMaxDrones() && CountMaturedRespawns(Now) > 0)
	{
		RequestSummonAbility();
	}
}

void ANSBossMotherShip::QueueFullWave()
{
	if (!HasAuthority()) return;
	
	const double CurrentTime = GetWorld()->GetTimeSeconds();
	const int32 DeficitCount  = FMath::Max(0, GetCurrentMaxDrones() - GetActiveDroneCount());
	
	for (int32 i = 0; i < DeficitCount; ++i)
	{
		PendingRespawnTimes.Add(CurrentTime);
	}
}

void ANSBossMotherShip::ScheduleRespawn()
{
	if (!HasAuthority()) return;
	
	const double RespawnTime = GetWorld()->GetTimeSeconds() + RespawnDelay;
	PendingRespawnTimes.Add(RespawnTime);
}

void ANSBossMotherShip::RequestSummonAbility()
{
	if (!HasAuthority() || !SpawnDroneAbilityClass) return;
	
	UAbilitySystemComponent* MotherShipASC = GetAbilitySystemComponent();
	if (!MotherShipASC) return;
	
	if (bSummonInFlight) return;
	
	bSummonInFlight = true;
	
	bool bSuccessSpawn = MotherShipASC->TryActivateAbilityByClass(SpawnDroneAbilityClass);
	
	if (!bSuccessSpawn)
	{
		bSummonInFlight = false;
	}
	
	
}

void ANSBossMotherShip::SpawnMaturedDrones()
{
	if (!HasAuthority()) return;
	
	bSummonInFlight = false;
	
	const double CurrentTime = GetWorld()->GetTimeSeconds();
	
	const int32 MatureRespawnCount = CountMaturedRespawns(CurrentTime);
	const int32 RoomCount = GetCurrentMaxDrones() - GetActiveDroneCount();
	
	const int32 DesiredCount = FMath::Min(MatureRespawnCount, RoomCount);
	if(DesiredCount <= 0) return;
	
	PendingRespawnTimes.Sort();
	
	TArray<FTransform> SpawnTransforms;
	if (!BuildDroneSpawnTransforms(DesiredCount, SpawnTransforms) || SpawnTransforms.Num() == 0)
	{
		return;
	}
	
	const int32 ActualCount = FMath::Min(DesiredCount, SpawnTransforms.Num());
	
	PendingRespawnTimes.RemoveAt(0, ActualCount);
	
	for (int32 i = 0; i < ActualCount; ++i)
	{
		ANSEnemyDrone* Drone = AcquireDroneFromPool(SpawnTransforms[i]);
		if (!Drone) continue;
		
		if (UNSEnemyStateComponent* DroneState = Drone->GetStateComponent())
		{
			DroneState->OnDeathStarted.RemoveAll(this);
			DroneState->OnDeathStarted.AddUObject(
				this, &ANSBossMotherShip::HandleDroneDied, TWeakObjectPtr<ANSEnemyDrone>(Drone));
		}
		
		ActiveDrones.Add(Drone);
	}
}

void ANSBossMotherShip::RecallAllDrones()
{
	if (!HasAuthority()) return;
	
	for (const TWeakObjectPtr<ANSEnemyDrone>& Drone : ActiveDrones)
	{
		if (!Drone.IsValid()) continue;
		
		if (UNSEnemyStateComponent* DroneState = Drone->GetStateComponent())
		{
			DroneState->OnDeathStarted.RemoveAll(this);
		}
		
		if (DronePool)
		{
			DronePool->ReturnDroneToPool(Drone.Get());
		}
	}
	
	ActiveDrones.Reset();
	PendingRespawnTimes.Reset();
}

void ANSBossMotherShip::HandleDroneDied(TWeakObjectPtr<ANSEnemyDrone> DeadDrone)
{
	if (!DeadDrone.IsValid()) return;

	ScheduleRespawn();
	ActiveDrones.Remove(DeadDrone);

	if (UNSEnemyStateComponent* DroneState = DeadDrone->GetStateComponent())
	{
		DroneState->OnDeathStarted.RemoveAll(this);
	}
	
	if (ReturnToPoolDelay <= 0.f)
	{
		if (DronePool) DronePool->ReturnDroneToPool(DeadDrone.Get());
	}
	else
	{
		FTimerHandle TmpHandle;   // 취소할 일 없으니 로컬 핸들(fire-and-forget)로 충분
		FTimerDelegate Del = FTimerDelegate::CreateUObject(
			this, &ANSBossMotherShip::ReturnDroneAfterDelay, DeadDrone);
		GetWorldTimerManager().SetTimer(TmpHandle, Del, ReturnToPoolDelay, false);
	}
}

int32 ANSBossMotherShip::GetActiveDroneCount() const
{
	int32 AliveCount = 0;
	
	for (const TWeakObjectPtr<ANSEnemyDrone>& Drone : ActiveDrones)
	{
		if (Drone.IsValid())
		{
			AliveCount++;
		}
	}
	return AliveCount;
}

int32 ANSBossMotherShip::CountMaturedRespawns(double NowSeconds) const
{
	int32 CanSpawnCount = 0;
	
	for (const double PendingTime : PendingRespawnTimes)
	{
		if (PendingTime <= NowSeconds)
		{
			CanSpawnCount++;
		}
	}
	
	return CanSpawnCount;
}

void ANSBossMotherShip::ReturnDroneAfterDelay(TWeakObjectPtr<ANSEnemyDrone> Drone)
{
	if (DronePool && Drone.IsValid())
	{
		DronePool->ReturnDroneToPool(Drone.Get());
	}
}

#pragma endregion

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
	ActiveDrones.RemoveAll([](const TWeakObjectPtr<ANSEnemyDrone>& Drone)
	{
		return !Drone.IsValid();
	});
}

bool ANSBossMotherShip::BuildDroneSpawnTransforms(int32 Count, TArray<FTransform>& OutTransforms) const
{
	UE_LOG(LogTemp, Warning, TEXT("BuildDroneSpawnTransforms called !"));
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


