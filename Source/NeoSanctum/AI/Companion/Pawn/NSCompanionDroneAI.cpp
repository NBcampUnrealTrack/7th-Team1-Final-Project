// Copyright 2026 One Team. All rights reserved.

#include "NSCompanionDroneAI.h"

#include "NeoSanctum/AI/Components/NSFlyingLocomotionComponent.h" 
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/AI/NSCompanionDefinition.h"
#include "NeoSanctum/GAS/AttributeSet/NSCompanionAttributeSet.h"
#include "NeoSanctum/System/Subsystem/NSCurrencyDropSubsystem.h"
#include "Components/SphereComponent.h"


ANSCompanionDroneAI::ANSCompanionDroneAI()
{
	PrimaryActorTick.bCanEverTick = true;
	
	TeamId = ETeamId::Player;
	AIControllerClass = ANSDroneAIController::StaticClass();
	CompanionAttributeSet = CreateDefaultSubobject<UNSCompanionAttributeSet>("AttributeSet");
	
	if (SphereComponent)
	{
		SphereComponent->SetCollisionResponseToChannel(ECC_WorldStatic,  ECR_Overlap);
		SphereComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
		SphereComponent->SetCollisionResponseToChannel(NSCollisionChannels::DestructibleObject, ECR_Overlap);
	}
}

void ANSCompanionDroneAI::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	// 서브 로직도 서버에서만 (Super가 조용히 실패해도 여기서 필터)
	if (!HasAuthority()) return;
	
	// [추가] 평상시 유지 고도를 캐시. Collect에서 낮췄다가 이 값으로 되돌린다.
	// (BP에서 Altitude를 300과 다르게 오버라이드했어도 실제 값을 그대로 반영)
	if (IsValid(FlyingMovementComponent))
	{
		DefaultAltitude = FlyingMovementComponent->GetAltitude();
	}
	
	// 오너 거리 체크 타이머 (0.25초 주기)
	GetWorldTimerManager().SetTimer(
		CheckDistanceToOwnerTimer,
		this,
		&ANSCompanionDroneAI::CheckDistanceToOwner,
		0.25f,
		true);
	
	// 재화 진공 타이머 (0.1초 주기)
	GetWorldTimerManager().SetTimer(
		CurrencyVacuumTimer,
		this,
		&ANSCompanionDroneAI::VacuumNearbyCurrency,
		0.1f,
		true);
}

void ANSCompanionDroneAI::InitializeFromData()
{
	Super::InitializeFromData();
	
	if (!CurrentDefinition) return;
	
	ApplyDroneDefinition(CurrentDefinition);
}

void ANSCompanionDroneAI::SetCurrentState(ECompanionState NewState)
{
	// 이 함수는 서비스 틱(0.15초)마다 호출되므로,
	// 상태가 실제로 바뀌는 "전이 순간"에만 고도를 재설정한다. (SetAltitude 스팸 방지)
	if (NewState == CurrentState) return;

	const ECompanionState PrevState = CurrentState;
	CurrentState = NewState;

	if (!IsValid(FlyingMovementComponent)) return;

	if (NewState == ECompanionState::Collect)
	{
		// 재화 수집 진입 → 지상 가까이 하강 (이동/회피는 그대로 살아있음)
		FlyingMovementComponent->SetAltitude(CollectAltitude);
	}
	else if (PrevState == ECompanionState::Collect)
	{
		// 재화 수집 이탈 → 평상 고도로 복귀
		FlyingMovementComponent->SetAltitude(DefaultAltitude);
	}
}

void ANSCompanionDroneAI::ApplyStatUpgrade(FGameplayTag NodeTag, int32 NewLevel)
{
	// 서버 체크
	if (!HasAuthority() || !CurrentDefinition) return;
	
	// 컴패니언 전용 Definition으로 캐스팅 (베이스 정의에는 UpgradeNodes 없음)
	const UNSCompanionDefinition* CompDef = Cast<UNSCompanionDefinition>(CurrentDefinition);
	if (!CompDef) return;
	
	// 현재 업그레이드 정보 받아오기 순회
	for (const FNSCompanionUpgradeNode& CurrentUpgradeNode : CompDef->UpgradeNodes)
	{
		// 업그레이드가 존재하는 노드 태그 찾기
		if (CurrentUpgradeNode.NodeTag != NodeTag) continue;
		
		// 들어온 업그레이드 레벨 값 정상화 방어 코드
		NewLevel = FMath::Clamp(NewLevel, 0, CurrentUpgradeNode.MaxLevel);
		// 실제 적용 수치
		float ApplyStat = NewLevel * CurrentUpgradeNode.MagnitudePerLevel;
		
		// 현재 적용중인 업그레이드 수치가 있는지 확인
		if (FActiveGameplayEffectHandle* BeforeEffectHandle = StatUpgradeHandles.Find(CurrentUpgradeNode.NodeTag))
		{
			// 있다면 모두 제거
			AbilitySystemComponent->RemoveActiveGameplayEffect(*BeforeEffectHandle);
		}
		
		if (NewLevel == 0)
		{
			StatUpgradeHandles.Remove(CurrentUpgradeNode.NodeTag);
			break;
		}
		
		// 새로운 ContextHandle
		FGameplayEffectContextHandle ContextHandle =
			AbilitySystemComponent->MakeEffectContext();
		
		// 새로운 SpecHandle
		FGameplayEffectSpecHandle UpgradeSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
			CurrentUpgradeNode.UpgradeEffect,
			NewLevel,
			ContextHandle);
		
		// SpecHandle을 만드는데 성공했다면
		if (UpgradeSpecHandle.IsValid())
		{
			// SetByCaller로 값 저장 후 현재 업그레이드 Map에 추가
			UpgradeSpecHandle.Data->SetSetByCallerMagnitude(CurrentUpgradeNode.SetByCallerTag, ApplyStat);
			
			StatUpgradeHandles.Add(
				NodeTag,
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*UpgradeSpecHandle.Data.Get())
			);
		}
		
		break;
	}
}

void ANSCompanionDroneAI::SetOwnerPlayer(AActor* Actor)
{
	if (!IsValid(Actor)) return;
	
	OwnerPlayer = Actor;
}

void ANSCompanionDroneAI::CheckDistanceToOwner()
{
	if (!OwnerPlayer || !HasAuthority()) return;
	
	float DistSq = FVector::DistSquared(GetActorLocation(), OwnerPlayer->GetActorLocation());
	
	// 하드 리쉬 초과 → 즉시 텔포
	if (DistSq > FMath::Square(HardLeashDistance))
	{
		// @TODO 민재 : 재화 탐색으로 인한 거리 멀어질시 텔포x
		TeleportToOwner();
		TimeBeyondLeash = 0.f;
		PrevDistSqToOwner = 0.f;
		return;
	}
	
	if (CurrentState != ECompanionState::Follow)
	{
		TimeBeyondLeash = 0.f;
		PrevDistSqToOwner = -1.f;
		return;
	}
	
	if (DistSq <= FMath::Square(MaxDistance))
	{
		TimeBeyondLeash = 0.f;
		PrevDistSqToOwner = 0.f;
		return;
	}
	
	// 접근 중이면 스턱 아님, 아니면 스턱 시간 누적
	if (PrevDistSqToOwner >= 0.f && DistSq < PrevDistSqToOwner)
	{
		TimeBeyondLeash = 0.f;
	}
	else
	{
		TimeBeyondLeash += CheckInterval;
	}
	
	// 스턱 시간 초과 → 텔포
	if (TimeBeyondLeash >= StuckRecoverTime)
	{
		TeleportToOwner();
		TimeBeyondLeash = 0.f;
	}
	
	PrevDistSqToOwner = DistSq;
}

void ANSCompanionDroneAI::TeleportToOwner()
{
	// 서버 및 오너 존재 체크
	if (!HasAuthority() || !OwnerPlayer) return;
	
	// 목표 지점
	const FVector Target = OwnerPlayer->GetActorLocation();
	
	// 텔레포트 적용
	SetActorLocation(Target, false, nullptr, ETeleportType::TeleportPhysics);
	
	// 로코모션 컴포넌트에 텔포 통지 (velocity 리셋 + 지형 재감지 유도)
	if (IsValid(FlyingMovementComponent))
	{
		FlyingMovementComponent->ResetLocomotionState();
	}
}

void ANSCompanionDroneAI::VacuumNearbyCurrency()
{
	if (!HasAuthority() || !OwnerPlayer) return;
	
	APawn* OwnerPawn = Cast<APawn>(OwnerPlayer);
	if (!OwnerPawn) return;
	
	ANSPlayerState* OwnerPS = Cast<ANSPlayerState>(OwnerPawn->GetPlayerState());
	if (!OwnerPS) return;
	
	UNSCurrencyDropSubsystem* DropSubsystem = GetWorld()->GetSubsystem<UNSCurrencyDropSubsystem>();
	if (!DropSubsystem) return;
	
	const FVector CompanionLocation = GetActorLocation();
	int32 OutDropId = INDEX_NONE;
	FVector OutLocation = FVector::ZeroVector;
	
	if (DropSubsystem->FindNearestTrackableDrop(OwnerPS, CompanionLocation, CurrencyVacuumRadius, OutDropId, OutLocation))
	{
		DropSubsystem->TryCollectByCompanion(OutDropId, OwnerPS, CompanionLocation);
	}
}