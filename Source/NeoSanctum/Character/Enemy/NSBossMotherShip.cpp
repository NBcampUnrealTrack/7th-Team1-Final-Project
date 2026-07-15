// Copyright 2026 One Team. All rights reserved.


#include "NSBossMotherShip.h"

#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "NSEnemyDrone.h"
#include "GameFramework/GameStateBase.h"
#include "NeoSanctum/AI/Components/NSFlyingLocomotionComponent.h"
#include "NeoSanctum/AI/Manager/NSDronePoolManager.h"
#include "NeoSanctum/Combat/Component/NSEnemyPartComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSGameFlowSubsystem.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/Interaction/Prop/NSBossControlDevice.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "NeoSanctum/AI/Enemy/HelperActor/NSBossArenaBounds.h"
#include "NeoSanctum/System/Component/NSDissolveComponent.h"

ANSBossMotherShip::ANSBossMotherShip()
{
	PrimaryActorTick.bCanEverTick = false;

	FlyingLocomotionComponent = CreateDefaultSubobject<UNSFlyingLocomotionComponent>(FName("FlyingMovementComponent"));
}

void ANSBossMotherShip::ApplyDeadState()
{
	Super::ApplyDeadState();
	RecallAllDrones();
	if (FlyingLocomotionComponent)
	{
		FlyingLocomotionComponent->StartDeathFall();
	}
}

void ANSBossMotherShip::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;
	
	if (AttributeSet)
	{
		AttributeSet->OnOutOfShield.AddUObject(this, &ThisClass::HandleBossOutOfShield);
	}
	
	if (PhaseComponent)
	{
		PhaseComponent->ResetPhaseState();
	}
	
	if (!ArenaBounds)
	{
		for (TActorIterator<ANSBossArenaBounds> It(GetWorld()); It; ++It) { ArenaBounds = *It; break; }
	}
	
	DronePool = NewObject<UNSDronePoolManager>(this);
	StartDronePattern();
	// TODO : 이상적으로는 '전투 진입' 콜백에서 호출. 전투 트리거가 생기면 그쪽으로 옮길 것.
	InitControlDevices();
	
	GetFlyingLocomotion()->SetSteeringInputLocked(true);
}

UNSFlyingLocomotionComponent* ANSBossMotherShip::GetFlyingLocomotion() const
{
	return FlyingLocomotionComponent;
}

#pragma region SpawnDronePattern

void ANSBossMotherShip::StartDronePattern()
{
	if (!HasAuthority()) return;
	QueueFullWave();
}

void ANSBossMotherShip::StopDronePattern()
{
	
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

void ANSBossMotherShip::SpawnMaturedDrones()
{
	if (!HasAuthority()) return;

	const double CurrentTime = GetWorld()->GetTimeSeconds();

	const int32 MatureRespawnCount = CountMaturedRespawns(CurrentTime);
	const int32 RoomCount = GetCurrentMaxDrones() - GetActiveDroneCount();

	const int32 DesiredCount = FMath::Min(MatureRespawnCount, RoomCount);
	if (DesiredCount <= 0) return;

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

bool ANSBossMotherShip::HasSpawnWorkReady() const
{
	const double Now = GetWorld()->GetTimeSeconds();
	const int32 RoomCount = GetCurrentMaxDrones() - GetActiveDroneCount();

	return CountMaturedRespawns(Now) > 0 && RoomCount > 0;
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
	
	// 디졸브 완료 시점에 풀 반환 → 사망 몽타주 + 디졸브가 끝나는 순간과 정렬
	if (UNSDissolveComponent* Dissolve = DeadDrone->GetDissolveComponent())
	{
		Dissolve->OnDissolveComplete.BindUObject(
			this, &ANSBossMotherShip::ReturnDroneAfterDelay, DeadDrone);
		return;
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
	}

	// 소켓만으로 Count를 다 채웠으면 종료
	if (OutTransforms.Num() >= Count)
	{
		return true;
	}
	
	// 부족분을 링으로 보충. 링을 못 쓰면 소켓으로 담은 만큼만 반환
	if (SpawnRingRadius <= 0.f)
	{
		return !OutTransforms.IsEmpty();
	}

	const int32 Remaining = Count - OutTransforms.Num();
	const FVector Center = GetActorLocation();
	const float AngleStep = 360.f / Remaining;   // 부족분 기준 균등 배치

	for (int32 i = 0; i < Remaining; ++i)
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

#pragma endregion

#pragma region ControlDevice

void ANSBossMotherShip::InitControlDevices()
{
	if (!HasAuthority()) return;
	
	UNSEnemyPartComponent* MotherShipPartComponent = GetPartComponent();
	if (!MotherShipPartComponent) return;
	
	ControlDevices.Reset();
	AliveControlDeviceCount = 0;
	bControlDevicesCleared = false;
	
	for (const FName& PartId : ControlDevicePartIds)
	{
		AActor* PartActor = MotherShipPartComponent->FindSpawnedPartActor(PartId);
		if (!PartActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ControlDevice] PartId=%s 스폰 액터 없음 (DT_EnemyParts 확인)"), *PartId.ToString());
			continue;
		}
		
		ANSBossControlDevice* ControlDevice = Cast<ANSBossControlDevice>(PartActor);
		if (!ControlDevice)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ControlDevice] PartId=%s 가 ANSBossControlDevice 아님 (BP 부모/ActorClass 확인)"), *PartId.ToString());
			continue;
		}
		
		if (ControlDevices.Contains(ControlDevice)) continue;
		
		ControlDevices.Add(ControlDevice);
		AliveControlDeviceCount++;
		
		ControlDevice->OnControlDeviceDestroyed.RemoveAll(this);
		ControlDevice->OnControlDeviceDestroyed.AddUObject(this, &ThisClass::HandleControlDeviceDestroyed);
		
		ControlDevice->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		
		// 서버 권위 바닥 스냅: 소켓 상대 배치가 만든 XY 레이아웃은 보존하고 Z만 지형에 맞게 교정
		PlaceControlDeviceOnGround(ControlDevice);
	}
	
	if (AliveControlDeviceCount > 0)
	{
		ApplyBossInvincibility();
		
		// Minimal 복제 모드(NSEnemyPawnBase.cpp:42)의 보스 ASC는 평범한 AddGameplayCue로는
		// 원격 클라에 복제되지 않음(ActiveGameplayCues는 Full 전용) → MinimalReplication 경로 사용.
		// 서버 가드는 이미 InitControlDevices 최상단(HasAuthority)에서 통과됨.
		if (UAbilitySystemComponent* MotherShipASC = GetAbilitySystemComponent())
		{
			MotherShipASC->AddGameplayCue_MinimalReplication(Phase1BarrierCueTag);
		}
	}
}

void ANSBossMotherShip::HandleControlDeviceDestroyed(ANSBossControlDevice* DestroyedDevice)
{
	if (!HasAuthority()) return;
	
	if (bControlDevicesCleared) return;
	
	AliveControlDeviceCount = FMath::Max(0, AliveControlDeviceCount - 1);
	if (DestroyedDevice)
	{
		DestroyedDevice->OnControlDeviceDestroyed.RemoveAll(this);
	}
	
	if (AliveControlDeviceCount == 0)
	{
		bControlDevicesCleared = true;
		ClearBossInvincibility();
		
		// Add 때와 동일하게 Minimal 복제 경로로 제거해야 원격 클라에서도 사라짐
		if (UAbilitySystemComponent* MotherShipASC = GetAbilitySystemComponent())
		{
			MotherShipASC->RemoveGameplayCue_MinimalReplication(Phase1BarrierCueTag);
		}
	}
}

void ANSBossMotherShip::PlaceControlDeviceOnGround(ANSBossControlDevice* ControlDevice) const
{
	// 호출부(InitControlDevices)가 이미 HasAuthority() 가드를 통과한 상태에서만 여기 도달한다.
	if (!ControlDevice) return;

	const FVector CurrentLocation = ControlDevice->GetActorLocation();

	// 바닥 스냅 전, 현재 피벗-바운드 관계를 먼저 캡처 (SetActorLocation으로 위치가 바뀌기 전에 계산해야 함)
	const float PivotToBottomOffset = ControlDevice->GetPivotToMeshBottomOffset();

	FHitResult Hit;
	const bool bFoundGround = UNSFlyingLocomotionComponent::TraceGroundAtWorldLocation(
		GetWorld(),
		CurrentLocation,
		ControlDeviceGroundTraceDistance,
		ControlDeviceGroundChannel,
		ControlDeviceMaxWalkableSlopeAngle,
		ControlDevice,
		Hit);

	if (!bFoundGround)
	{
		// 트레이스 실패 폴백: 원래(소켓 상대) 위치를 그대로 유지하고 경고만 남김
		UE_LOG(LogTemp, Warning, TEXT("[ControlDevice] %s 바닥 트레이스 실패, 원래 위치 유지"), *ControlDevice->GetName());
		return;
	}

	// XY는 소켓/RelativeTransform이 만든 레이아웃 그대로, Z만 바닥+피벗오프셋으로 교정
	const FVector GroundLocation(CurrentLocation.X, CurrentLocation.Y, Hit.ImpactPoint.Z + PivotToBottomOffset);

	// 소켓 상대 회전에서 Yaw만 유지하고 Pitch/Roll은 버려 수평으로 앉힘
	const FRotator LeveledRotation(0.f, ControlDevice->GetActorRotation().Yaw, 0.f);

	ControlDevice->ApplyGroundPlacement(GroundLocation, LeveledRotation);
}

void ANSBossMotherShip::ApplyBossInvincibility()
{
	UAbilitySystemComponent* MotherShipAbilitySystemComponent = GetAbilitySystemComponent();
	if (!MotherShipAbilitySystemComponent) return;
	
	MotherShipAbilitySystemComponent->AddLooseGameplayTag(NSGameplayTags::State_Invincible);
}

void ANSBossMotherShip::ClearBossInvincibility()
{
	UAbilitySystemComponent* MotherShipAbilitySystemComponent = GetAbilitySystemComponent();
	if (!MotherShipAbilitySystemComponent) return;
	
	MotherShipAbilitySystemComponent->RemoveLooseGameplayTag(NSGameplayTags::State_Invincible);
}

#pragma endregion

#pragma region PhaseTransition

void ANSBossMotherShip::BeginPhase2Transition()
{
	// 기존 서버 가드 및 보스 무적상태 진입
	if (!HasAuthority()) return;
	ApplyBossInvincibility();

	// 보스ASC 캐스팅 및 끝나고 적용시킬 2페이즈 태그 가져오기
	UAbilitySystemComponent* MotherShipAbilitySystemComponent = GetAbilitySystemComponent();
	const FGameplayTag PhaseTagToDefer = PhaseComponent ? PhaseComponent->GetCurrentPhaseTag() : FGameplayTag();

	// ASC와 페이즈태그 둘다 존재한다면
	if (MotherShipAbilitySystemComponent && PhaseTagToDefer.IsValid())
	{
		// 2페이즈 이후 다시 적용시킬 태그 캐싱
		DeferredPhase2Tag = PhaseTagToDefer;
		
		// 태그가 현재 보류상태임을 전달
		bPhase2TagDeferred = true;
		
		// 연출시작시 현재 적용되어있는 연출동안 숨기려고 잠시 제거
		MotherShipAbilitySystemComponent->RemoveLooseGameplayTag(DeferredPhase2Tag);
	}
	
	RecallAllDrones();
	QueueFullWave();
}

void ANSBossMotherShip::CompletePhase2Transition()
{
	// 서버 가드
	if (!HasAuthority()) return;
	
	// 플레이어 쉴드 제거 함수 작동
	DrainAllPlayerShields(PlayerShieldDrainRatio);
	
	// 보스 쉴드 추가
	GrantBossShield();
	
	// Minimal 복제 모드이므로 반드시 MinimalReplication 경로로 큐 부여 (서버 가드는 함수 최상단에서 이미 통과)
	if (UAbilitySystemComponent* MotherShipASC = GetAbilitySystemComponent())
	{
		MotherShipASC->AddGameplayCue_MinimalReplication(Phase2ShieldCueTag);
	}
	
	// 보스 무적 해제
	ClearBossInvincibility();
	
	// 이동제한 해제
	bBossMovementUnlocked = true;
	
	// flyingComponent 이동불가 해제
	GetFlyingLocomotion()->SetSteeringInputLocked(false);

	// 2페이즈 태그 주입
	CommitDeferredPhase2Tag();
}

void ANSBossMotherShip::CommitDeferredPhase2Tag()
{
	// 서버 가드 및 숨긴 태그가 있는지 확인
	if (!HasAuthority() || !bPhase2TagDeferred) return;

	// ASC 가져와서 숨긴태그가 유효한지 확인
	UAbilitySystemComponent* MotherShipAbilitySystemComponent = GetAbilitySystemComponent();
	if (MotherShipAbilitySystemComponent && DeferredPhase2Tag.IsValid())
	{
		// 유효하다면 숨겼던 태그 다시 적용
		MotherShipAbilitySystemComponent->AddLooseGameplayTag(DeferredPhase2Tag);
		
		// 재호출 방지 플래그 해제
		bPhase2TagDeferred = false;
	}
}

void ANSBossMotherShip::DrainAllPlayerShields(float RemoveRatio)
{
	if (!HasAuthority()) return;
	ANSRunGameState* RunGameState = Cast<ANSRunGameState>(GetWorld()->GetGameState());
	if (!RunGameState) return;
	for (const TObjectPtr<APlayerState>& CurrentPlayerState : RunGameState->PlayerArray)
	{
		ANSPlayerState* NSPS = Cast<ANSPlayerState>(CurrentPlayerState);
		if (!NSPS) continue;
		
		UNSPlayerAttributeSet* PlayerAttributeSet = NSPS->GetPlayerAttributeSet();
		if (!PlayerAttributeSet) continue;
		
		const float RemainingShield = PlayerAttributeSet->GetShield() * (1.0f - RemoveRatio);
		PlayerAttributeSet->SetShield(RemainingShield);
	}
}

void ANSBossMotherShip::GrantBossShield()
{
	if (!HasAuthority()) return;
	if (!AttributeSet) return;
	AttributeSet->SetMaxShield(Phase2ShieldAmount);
	AttributeSet->SetShield(Phase2ShieldAmount);
	
	// 재고갈 시 OnOutOfShield가 다시 발동할 수 있도록 1회성 가드 리셋
	AttributeSet->ResetOutOfShieldGuard();
}

void ANSBossMotherShip::HandleBossOutOfShield()
{
	// AttributeSet::OnOutOfShield는 서버(HandlePreHealthDamage)에서만 브로드캐스트되므로
	// 이 콜백도 항상 서버에서만 실행됨. 별도 HasAuthority 체크 불필요하나 방어적으로 유지.
	if (!HasAuthority()) return;

	if (UAbilitySystemComponent* MotherShipASC = GetAbilitySystemComponent())
	{
		MotherShipASC->RemoveGameplayCue_MinimalReplication(Phase2ShieldCueTag);
	}
}

#pragma endregion
