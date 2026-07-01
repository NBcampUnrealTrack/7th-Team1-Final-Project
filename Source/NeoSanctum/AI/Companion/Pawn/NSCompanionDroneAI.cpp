// Copyright 2026 One Team. All rights reserved.


#include "NSCompanionDroneAI.h"

#include "GameFramework/FloatingPawnMovement.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/AI/NSCompanionDefinition.h"
#include "NeoSanctum/System/Subsystem/NSCurrencyDropSubsystem.h"
#include "NeoSanctum/GAS/AttributeSet/NSCompanionAttributeSet.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"


ANSCompanionDroneAI::ANSCompanionDroneAI()
{
	PrimaryActorTick.bCanEverTick = true;
	
	TeamId = ETeamId::Player;
	AIControllerClass = ANSDroneAIController::StaticClass();
	CompanionAttributeSet = CreateDefaultSubobject<UNSCompanionAttributeSet>("AttributeSet");
}

void ANSCompanionDroneAI::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	GetWorldTimerManager().SetTimer(
	CheckDistanceToOwnerTimer,
	this,
	&ANSCompanionDroneAI::CheckDistanceToOwner,
	0.25f,
	true);
	
	GetWorldTimerManager().SetTimer(
		CurrencyVacuumTimer, 
		this,
		&ANSCompanionDroneAI::VacuumNearbyCurrency,
		0.1f, true);
}

void ANSCompanionDroneAI::SetCurrentState(ECompanionState NewState)
{
	CurrentState = NewState;
}

void ANSCompanionDroneAI::ApplyStatUpgrade(FGameplayTag NodeTag, int32 NewLevel)
{
	// 서버 체크
	if (!HasAuthority() || !CurrentDefinition) return;
	
	// 현재 업그레이드 정보 받아오기 순회
	const UNSCompanionDefinition* CompDef = Cast<UNSCompanionDefinition>(CurrentDefinition);
	if (!CompDef) return;
	
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
		
		// 새로운 contextHandle
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
			// setsetbycaller로 값 저장 후 현재 업그레이드 Map에 추가
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
	
	// 오너와 거리 계산
	if (DistSq > FMath::Square(HardLeashDistance))
	{
		// 오너 쪽 순간이동
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
	
	if (PrevDistSqToOwner >= 0.f && DistSq < PrevDistSqToOwner)
	{
		TimeBeyondLeash = 0.f;
	}
	else
	{
		TimeBeyondLeash += CheckInterval;
	}
	
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
	
	// Owner도착 지점 값 가져오기
	const FVector Target = OwnerPlayer->GetActorLocation();
	
	// 텔레포트전 이동속도 0 세팅
	if (FloatingPawnMovementComponent)
	{
		FloatingPawnMovementComponent->Velocity = FVector::ZeroVector;
	}
	
	// 텔레포트 적용
	SetActorLocation(Target, false, nullptr, ETeleportType::TeleportPhysics);
	
	bHasValidGround = false;
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
	
	if(DropSubsystem->FindNearestTrackableDrop(OwnerPS, CompanionLocation, CurrencyVaccumRadius,OutDropId,OutLocation))
	{
		DropSubsystem->TryCollectByCompanion(OutDropId, OwnerPS, CompanionLocation);
	}
}

