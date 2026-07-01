// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyAIController.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Combat/Component/NSMeleeAttackReservationComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/Interaction/Prop/NSDestructibleObjectBase.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "NeoSanctum/AI/Enemy/Controller/NSEnemyControllerBase.h"
#include "NeoSanctum/Combat/Component/NSEnemyAttackComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyTargetComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"


ANSEnemyAIController::ANSEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;

	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));

	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ANSEnemyAIController::OnTargetPerceptionUpdated);
}

void ANSEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(GetPawn());

	UpdateEnemyPhase();

	if (IsPhasePatternLocked())
	{
		StopMovement();

		if (CachedBBComp)
		{
			CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
			CachedBBComp->SetValueAsBool(TEXT("bIsAttacking"), false);
			CachedBBComp->SetValueAsBool(ShouldRetreatKey, false);
		}

		return;
	}

	if (Enemy && Enemy->IsHitReacting())
	{
		StopMovement();

		if (CachedBBComp)
		{
			CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
			CachedBBComp->SetValueAsBool(TEXT("bIsAttacking"), false);
			CachedBBComp->SetValueAsBool(ShouldRetreatKey, false);
		}

		return;
	}

	if (UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent())
	{
		if (ThreatComponent->CanEvaluateTarget())
		{
			UpdateTargetSelection();
			UpdateMeleeReservationState();

			ThreatComponent->ScheduleNextEvaluation();
		}
	}

	AActor* TargetActor = GetCurrentTargetActor();

	if (IsValidLivingTarget(TargetActor))
	{
		if (Enemy)
		{
			UpdateRetreatState(Enemy, TargetActor);
			UpdateFacingMode(Enemy, TargetActor);
		}
	}
	else
	{
		ApplyFacingMode(Enemy, nullptr, false);

		if (Enemy)
		{
			Enemy->SetRetreating(false);
		}

		if (CachedBBComp)
		{
			CachedBBComp->SetValueAsBool(ShouldRetreatKey, false);
			CachedBBComp->ClearValue(RetreatLocationKey);
		}
	}

	CanUseAnyAttackByDistance();
}

ETeamAttitude::Type ANSEnemyAIController::GetTeamAttitudeTo(const AActor& Other) const
{
	// 센서에 포착된 대상이 팀 인터페이스 마크를 가지고 있는지 확인
	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(&Other))
	{
		if (TeamAgent->GetGenericTeamId() == FGenericTeamId(static_cast<uint8>(ETeamId::Player)))
		{
			if (IsValidLivingTarget(&Other))
			{
				return ETeamAttitude::Type::Hostile;
			}
		}
		else if (TeamAgent->GetGenericTeamId() == FGenericTeamId(static_cast<uint8>(ETeamId::Enemy)))
		{
			return ETeamAttitude::Type::Friendly;
		}
	}

	// 인터페이스가 없거나 그 외의 대상은 중립 처리
	return ETeamAttitude::Type::Neutral;
}

bool ANSEnemyAIController::CanUseAnyAttackByDistance()
{
	UpdateEnemyPhase();

	if (IsPhasePatternLocked())
	{
		if (CachedBBComp)
		{
			CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
		}

		return false;
	}

	if (IsControlledEnemyHitReacting())
	{
		if (CachedBBComp)
		{
			CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
		}

		return false;
	}

	const FNSEnemyAttackRow* UsableAttack = FindAttackRowByDistance(false);

	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), UsableAttack != nullptr);
	}

	return UsableAttack != nullptr;
}

void ANSEnemyAIController::RecordAttackUsed(const FNSEnemyAttackRow& AttackRow)
{
	NotifyAttackStarted();

	if (UNSEnemyAttackComponent* AttackComponent = GetEnemyAttackComponent())
	{
		AttackComponent->RecordAttackUsed(AttackRow);
	}
}

const FNSEnemyAttackRow* ANSEnemyAIController::GetAttackRowByDistance()
{
	UpdateEnemyPhase();

	if (IsPhasePatternLocked())
	{
		if (CachedBBComp)
		{
			CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
		}

		return nullptr;
	}

	if (IsControlledEnemyHitReacting())
	{
		if (CachedBBComp)
		{
			CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
		}

		return nullptr;
	}

	const FNSEnemyAttackRow* SelectedAttack = FindAttackRowByDistance(true);

	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), SelectedAttack != nullptr);
	}

	return SelectedAttack;
}

AActor* ANSEnemyAIController::GetCurrentTargetActor() const
{
	const UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent();
	return ThreatComponent ? ThreatComponent->GetCurrentTarget() : nullptr;
}

void ANSEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (UNSEnemyAttackComponent* AttackComponent = GetEnemyAttackComponent())
	{
		AttackComponent->ResetAttackState();
	}

	if (UNSEnemyPhaseComponent* PhaseComponent = GetEnemyPhaseComponent())
	{
		PhaseComponent->ResetPhaseState();
	}

	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(InPawn);
	if (!EnemyAgent)
	{
		return;
	}

	UNSEnemyData* EnemyData = EnemyAgent->GetEnemyData();
	if (!EnemyData)
	{
		return;
	}

	StartEnemyBrain(EnemyData);

	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(IsHitReactingKey, false);
		CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
		CachedBBComp->SetValueAsBool(TEXT("bIsAttacking"), false);
		CachedBBComp->SetValueAsBool(PhasePatternLockedKey, false);
	}

	ResetTargetingState();
	InitializeMeleeEQSBlackboard(EnemyData);
}

void ANSEnemyAIController::OnUnPossess()
{
	StopEnemyBrain(TEXT("UnPossess"));
	ResetTargetingState();
	InitializeMeleeEQSBlackboard(nullptr);

	CachedBBComp = nullptr;

	if (UNSEnemyPhaseComponent* PhaseComponent = GetEnemyPhaseComponent())
	{
		PhaseComponent->ResetPhaseState();
	}

	Super::OnUnPossess();
}

void ANSEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor)
	{
		return;
	}

	if (GetTeamAttitudeTo(*Actor) != ETeamAttitude::Hostile)
	{
		return;
	}

	UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent();
	if (!ThreatComponent)
	{
		return;
	}

	if (!IsValidLivingTarget(Actor))
	{
		ThreatComponent->RemoveTarget(Actor, true);
		UpdateCurrentTargetBlackboard();
		return;
	}

	ThreatComponent->UpdateThreatFromStimulus(Actor, Stimulus);

	UpdateTargetSelection();
}

bool ANSEnemyAIController::IsValidLivingTarget(const AActor* Target) const
{
	if (!Target) return false;

	// GAS 능력 여부 확인
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Target);
	if (!ASI) return false;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return false;

	bool bHasHealthAttribute = false;
	float CurrentHealth = 0.0f;

	// 대상 관계 없이 Health 추출
	for (UAttributeSet* AttributeSet : ASC->GetSpawnedAttributes())
	{
		if (AttributeSet)
		{
			if (FProperty* Prop = AttributeSet->GetClass()->FindPropertyByName(TEXT("Health")))
			{
				FGameplayAttribute HealthAttribute(Prop);
				if (ASC->HasAttributeSetForAttribute(HealthAttribute))
				{
					CurrentHealth = ASC->GetNumericAttribute(HealthAttribute);
					bHasHealthAttribute = true;
					break;
				}
			}
		}
	}

	// 체력 데이터가 없거나 체력이 0 이하인 경우 무효 타겟으로 판정
	if (!bHasHealthAttribute || CurrentHealth <= 0.0f)
	{
		return false;
	}

	return true;
}

const FNSEnemyAttackRow* ANSEnemyAIController::FindAttackRowByDistance(bool bSelectWeightedAttack)
{
	if (!CachedBBComp)
	{
		SetAttackActorBlackboard(nullptr);
		return nullptr;
	}

	AActor* TargetActor = Cast<AActor>(CachedBBComp->GetValueAsObject(TargetActorKey));

	APawn* AIPawn = GetPawn();
	if (!AIPawn || !IsValidLivingTarget(TargetActor))
	{
		CachedBBComp->SetValueAsObject(TargetActorKey, nullptr);
		SetAttackActorBlackboard(nullptr);
		return nullptr;
	}

	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(AIPawn);
	if (!EnemyAgent || EnemyAgent->IsHitReacting())
	{
		SetAttackActorBlackboard(nullptr);
		return nullptr;
	}

	UNSEnemyAttackComponent* AttackComponent = GetEnemyAttackComponent();
	if (!AttackComponent)
	{
		SetAttackActorBlackboard(nullptr);
		return nullptr;
	}

	const FVector ToTarget =
		(TargetActor->GetActorLocation() - AIPawn->GetActorLocation()).GetSafeNormal2D();

	const FVector Forward = AIPawn->GetActorForwardVector().GetSafeNormal2D();

	const float FacingDot = FVector::DotProduct(Forward, ToTarget);
	const float RequiredDot = FMath::Cos(FMath::DegreesToRadians(AttackFacingAngleDegrees));

	if (FacingDot < RequiredDot)
	{
		SetAttackActorBlackboard(nullptr);
		return nullptr;
	}

	bool bHasDirectLineOfSight = false;
	AActor* AttackActor = ResolveAttackActor(TargetActor, bHasDirectLineOfSight);

	if (!IsValid(AttackActor))
	{
		SetAttackActorBlackboard(nullptr);
		return nullptr;
	}

	const float Distance =
		FVector::Dist(AIPawn->GetActorLocation(), TargetActor->GetActorLocation());

	const FNSEnemyAttackRow* SelectedAttack = AttackComponent->SelectAttack(
		TargetActor,
		AttackActor,
		Distance,
		bHasDirectLineOfSight,
		bSelectWeightedAttack
	);

	SetAttackActorBlackboard(SelectedAttack ? AttackActor : nullptr);

	return SelectedAttack;
}

UNSEnemyAttackComponent* ANSEnemyAIController::GetEnemyAttackComponent() const
{
	const APawn* ControlledPawn = GetPawn();
	return ControlledPawn
		       ? ControlledPawn->FindComponentByClass<UNSEnemyAttackComponent>()
		       : nullptr;
}

void ANSEnemyAIController::UpdateRetreatState(ANSEnemyCharacterBase* Enemy, const AActor* TargetActor)
{
	if (!CachedBBComp || !Enemy || !IsValid(TargetActor))
	{
		return;
	}

	const UNSEnemyData* EnemyData = Enemy->GetEnemyData();
	const float MinRange = GetMinimumAttackRange(EnemyData);

	// Melee Attack인 경우
	if (MinRange <= 0.0f)
	{
		CachedBBComp->SetValueAsBool(ShouldRetreatKey, false);
		CachedBBComp->ClearValue(RetreatLocationKey);
		Enemy->SetRetreating(false);
		return;
	}

	const FVector EnemyLocation = Enemy->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();

	const float Distance = FVector::Dist(
		EnemyLocation,
		TargetLocation);

	const bool bWasRetreating = CachedBBComp->GetValueAsBool(ShouldRetreatKey);

	// 경계에서 전진/후퇴 반복 방지
	const float ExitRange = MinRange + RetreatExitBuffer;

	const bool bShouldRetreat = bWasRetreating
		                            ? Distance < ExitRange
		                            : Distance < MinRange;

	CachedBBComp->SetValueAsBool(ShouldRetreatKey, bShouldRetreat);

	Enemy->SetRetreating(bShouldRetreat);

	if (!bShouldRetreat)
	{
		CachedBBComp->ClearValue(RetreatLocationKey);
		return;
	}

	const FVector CurrentDestination = CachedBBComp->GetValueAsVector(RetreatLocationKey);

	const bool bDestinationReached = FVector::DistSquared2D(
		EnemyLocation,
		CurrentDestination) <= FMath::Square(RetreatDestinationAcceptanceRadius);

	const bool bHasRetreatDestination = CachedBBComp->IsVectorValueSet(RetreatLocationKey);

	// 처음 후퇴 시 / 기존 후퇴 지점 도착 시
	if (bWasRetreating && bHasRetreatDestination && !bDestinationReached)
	{
		return;
	}

	FVector AwayDirection = (EnemyLocation - TargetLocation).GetSafeNormal2D();

	if (AwayDirection.IsNearlyZero())
	{
		AwayDirection = -Enemy->GetActorForwardVector().GetSafeNormal2D();
	}

	const float RequiredDistance = FMath::Max(
		ExitRange - Distance,
		RetreatStepDistance);

	const FVector DesiredLocation = EnemyLocation + AwayDirection * RequiredDistance;

	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	FNavLocation ProjectedLocation;

	if (NavigationSystem &&
		NavigationSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation))
	{
		CachedBBComp->SetValueAsVector(RetreatLocationKey, ProjectedLocation.Location);
	}
}

float ANSEnemyAIController::GetMinimumAttackRange(const UNSEnemyData* EnemyData) const
{
	if (!EnemyData)
	{
		return 0.0f;
	}

	float MinimumRange = TNumericLimits<float>::Max();
	bool bFoundAttack = false;

	for (const FNSEnemyAttackRow* AttackRow : EnemyData->GetAttackRows())
	{
		if (!AttackRow || !AttackRow->AbilityClass)
		{
			continue;
		}

		bFoundAttack = true;
		MinimumRange = FMath::Min(MinimumRange, AttackRow->Condition.MinRange);
	}

	return bFoundAttack ? MinimumRange : 0.0f;
}


void ANSEnemyAIController::NotifyAttackStarted()
{
	if (UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent())
	{
		ThreatComponent->NotifyAttackStarted();
	}
}

void ANSEnemyAIController::UpdateTargetSelection()
{
	UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent();
	if (!ThreatComponent)
	{
		return;
	}

	AActor* CurrentTarget = ThreatComponent->GetCurrentTarget();

	const bool bIsAttacking =
		CachedBBComp && CachedBBComp->GetValueAsBool(TEXT("bIsAttacking"));

	const bool bCanMaintainCurrentTarget =
		CurrentTarget && CanMaintainCoverAttackTarget(CurrentTarget);

	const FNSEnemyThreatUpdateResult Result =
		ThreatComponent->UpdateTarget(
			bIsAttacking,
			bCanMaintainCurrentTarget
		);

	if (Result.bTargetChanged)
	{
		CancelMeleeReservationRequest(false);
		ResetMeleeEQSForCurrentTarget();
	}

	UpdateCurrentTargetBlackboard();
}

void ANSEnemyAIController::ClearCurrentCombatTarget(bool bBlockReacquisition)
{
	if (UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent())
	{
		ThreatComponent->ClearCurrentTarget(bBlockReacquisition);
	}

	CancelMeleeReservationRequest(false);
	ResetMeleeEQSForCurrentTarget();

	if (CachedBBComp)
	{
		CachedBBComp->ClearValue(TargetActorKey);
		CachedBBComp->ClearValue(AttackActorKey);
		CachedBBComp->ClearValue(TargetLastKnownLocationKey);

		CachedBBComp->SetValueAsBool(HasTargetLineOfSightKey, false);
		CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
	}
}

void ANSEnemyAIController::ResetTargetingState()
{
	CancelMeleeReservationRequest(false);

	if (UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent())
	{
		ThreatComponent->ResetThreatState();
	}

	ResetMeleeEQSForCurrentTarget();

	if (CachedBBComp)
	{
		CachedBBComp->ClearValue(TargetActorKey);
		CachedBBComp->ClearValue(AttackActorKey);
		CachedBBComp->ClearValue(TargetLastKnownLocationKey);

		CachedBBComp->SetValueAsBool(HasTargetLineOfSightKey, false);
		CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
	}
}

void ANSEnemyAIController::UpdateCurrentTargetBlackboard()
{
	if (!CachedBBComp)
	{
		return;
	}

	AActor* TargetActor = GetCurrentTargetActor();

	if (!IsValidLivingTarget(TargetActor))
	{
		CachedBBComp->ClearValue(TargetActorKey);
		CachedBBComp->ClearValue(AttackActorKey);
		CachedBBComp->ClearValue(TargetLastKnownLocationKey);
		CachedBBComp->SetValueAsBool(HasTargetLineOfSightKey, false);
		return;
	}

	CachedBBComp->SetValueAsObject(TargetActorKey, TargetActor);

	FVector LastKnownLocation = TargetActor->GetActorLocation();

	if (const UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent())
	{
		ThreatComponent->TryGetLastKnownLocation(TargetActor, LastKnownLocation);
	}

	CachedBBComp->SetValueAsVector(TargetLastKnownLocationKey, LastKnownLocation);

	bool bHasDirectLineOfSight = false;
	AActor* AttackActor = ResolveAttackActor(TargetActor, bHasDirectLineOfSight);

	SetAttackActorBlackboard(AttackActor);
	CachedBBComp->SetValueAsBool(HasTargetLineOfSightKey, bHasDirectLineOfSight);
}

bool ANSEnemyAIController::CanMaintainCoverAttackTarget(AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	const APawn* AIPawn = GetPawn();
	if (!AIPawn || !IsValidLivingTarget(TargetActor))
	{
		return false;
	}

	const UNSEnemyData* EnemyData = GetControlledEnemyData();
	if (!EnemyData)
	{
		return false;
	}

	const UNSEnemyAttackComponent* AttackComponent = GetEnemyAttackComponent();
	if (!AttackComponent)
	{
		return false;
	}

	bool bHasDirectLineOfSight = false;
	AActor* AttackActor = ResolveAttackActor(TargetActor, bHasDirectLineOfSight);

	if (bHasDirectLineOfSight)
	{
		return false;
	}

	if (!IsValidLivingTarget(AttackActor))
	{
		return false;
	}

	if (AttackActor == TargetActor)
	{
		return false;
	}

	if (!AttackActor->IsA<ANSDestructibleObjectBase>())
	{
		return false;
	}

	const float Distance =
		FVector::Dist(AIPawn->GetActorLocation(), TargetActor->GetActorLocation());

	for (const FNSEnemyAttackRow* AttackRow : EnemyData->GetAttackRows())
	{
		if (!AttackRow)
		{
			continue;
		}

		if (AttackComponent->CanUseAttack(
			*AttackRow,
			TargetActor,
			AttackActor,
			Distance,
			bHasDirectLineOfSight
		))
		{
			return true;
		}
	}

	return false;
}

bool ANSEnemyAIController::RequestMeleeAttackReservation()
{
	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(GetPawn());

	AActor* TargetActor = GetCurrentTargetActor();

	if (!Enemy || !TargetActor || !UsesMeleeAttackReservation())
	{
		return false;
	}

	UNSMeleeAttackReservationComponent* Component =
		TargetActor->FindComponentByClass<UNSMeleeAttackReservationComponent>();

	// 컴포넌트 없는 타깃은 EQS만 적용하고 접근 허용
	if (!Component)
	{
		CancelMeleeReservationRequest(false);
		SetMeleeReservationBlackboard(false, true);
		return true;
	}

	if (MeleeReservationTarget.IsValid() && MeleeReservationTarget.Get() != TargetActor)
	{
		CancelMeleeReservationRequest(false);
	}

	MeleeReservationTarget = TargetActor;

	const ENSMeleeReservationRequestResult Result =
		Component->RequestReservation(Enemy, GetLatestDamageTimeFromCurrentTarget());

	const bool bReserved = Result == ENSMeleeReservationRequestResult::Reserved;

	SetMeleeReservationBlackboard(bReserved, bReserved);

	return Result != ENSMeleeReservationRequestResult::Rejected;
}

bool ANSEnemyAIController::HasMeleeAttackReservation() const
{
	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(GetPawn());

	AActor* ReservedTarget = MeleeReservationTarget.Get();

	if (!Enemy || !ReservedTarget)
	{
		return false;
	}

	UNSMeleeAttackReservationComponent* Component =
		ReservedTarget->FindComponentByClass<UNSMeleeAttackReservationComponent>();

	return Component && Component->HasReservation(Enemy);
}

bool ANSEnemyAIController::CanApproachMeleeTarget() const
{
	if (!UsesMeleeAttackReservation())
	{
		return true;
	}

	AActor* TargetActor = GetCurrentTargetActor();

	if (!TargetActor)
	{
		return false;
	}

	const bool bReservationRequired =
		TargetActor->FindComponentByClass<UNSMeleeAttackReservationComponent>() != nullptr;

	return !bReservationRequired || this->HasMeleeAttackReservation();
}

bool ANSEnemyAIController::CurrentTargetRequiresMeleeReservation() const
{
	if (!UsesMeleeAttackReservation())
	{
		return false;
	}

	AActor* TargetActor = GetCurrentTargetActor();

	return TargetActor && TargetActor->FindComponentByClass<UNSMeleeAttackReservationComponent>() != nullptr;
}

void ANSEnemyAIController::NotifyMeleeReservationAttackStarted()
{
	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(GetPawn());

	AActor* ReservedTarget = MeleeReservationTarget.Get();

	if (!Enemy || !ReservedTarget)
	{
		return;
	}

	if (UNSMeleeAttackReservationComponent* Component =
		ReservedTarget->FindComponentByClass<UNSMeleeAttackReservationComponent>())
	{
		Component->MarkAttackStarted(Enemy);
	}
}

void ANSEnemyAIController::ReleaseMeleeAttackReservation(bool bStartReacquireCooldown)
{
	CancelMeleeReservationRequest(bStartReacquireCooldown);

	ResetMeleeEQSForCurrentTarget();
}

void ANSEnemyAIController::UpdateMeleeReservationState()
{
	if (!UsesMeleeAttackReservation())
	{
		SetMeleeReservationBlackboard(false, true);
		return;
	}

	AActor* CurrentTarget = GetCurrentTargetActor();
	if (!CurrentTarget)
	{
		CancelMeleeReservationRequest(false);
		return;
	}

	UNSMeleeAttackReservationComponent* Component =
		CurrentTarget->FindComponentByClass<UNSMeleeAttackReservationComponent>();
	if (!Component)
	{
		if (MeleeReservationTarget.IsValid())
		{
			CancelMeleeReservationRequest(false);
		}

		SetMeleeReservationBlackboard(false, true);
		return;
	}

	if (MeleeReservationTarget.IsValid() && MeleeReservationTarget.Get() != CurrentTarget)
	{
		CancelMeleeReservationRequest(false);
		return;
	}

	const bool bReserved = HasMeleeAttackReservation();
	SetMeleeReservationBlackboard(bReserved, bReserved);
}

void ANSEnemyAIController::CancelMeleeReservationRequest(bool bStartReacquireCooldown)
{
	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(GetPawn());

	if (AActor* ReservedTarget = MeleeReservationTarget.Get())
	{
		if (UNSMeleeAttackReservationComponent* Component =
			ReservedTarget->FindComponentByClass<UNSMeleeAttackReservationComponent>())
		{
			Component->ReleaseReservation(Enemy, bStartReacquireCooldown);
		}
	}

	MeleeReservationTarget.Reset();
	SetMeleeReservationBlackboard(false, false);
}

bool ANSEnemyAIController::UsesMeleeAttackReservation() const
{
	const ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(GetPawn());
	const UNSEnemyData* EnemyData = Enemy ? Enemy->GetEnemyData() : nullptr;

	if (!EnemyData)
	{
		return false;
	}

	for (const FNSEnemyAttackRow* AttackRow : EnemyData->GetAttackRows())
	{
		if (AttackRow &&
			AttackRow->AbilityClass &&
			AttackRow->AttackType == ENSEnemyAttackType::MeleeSweep)
		{
			return true;
		}
	}

	return false;
}

double ANSEnemyAIController::GetLatestDamageTimeFromCurrentTarget() const
{
	const UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent();
	return ThreatComponent
		       ? ThreatComponent->GetLatestDamageTime(GetCurrentTargetActor())
		       : -1.0;
}

void ANSEnemyAIController::SetMeleeReservationBlackboard(bool bHasReservation, bool bCanApproach)
{
	if (!CachedBBComp)
	{
		return;
	}

	CachedBBComp->SetValueAsBool(HasMeleeAttackReservationKey, bHasReservation);
	CachedBBComp->SetValueAsBool(CanApproachMeleeTargetKey, bCanApproach);
}

void ANSEnemyAIController::InitializeMeleeEQSBlackboard(const UNSEnemyData* EnemyData)
{
	if (!CachedBBComp)
	{
		return;
	}

	CachedBBComp->ClearValue(MeleeApproachLocationKey);

	CachedBBComp->SetValueAsBool(MeleeEQSNeedsRefreshKey, false);

	if (EnemyData && EnemyData->EQSQuery)
	{
		CachedBBComp->SetValueAsObject(MeleeEQSQueryKey, EnemyData->EQSQuery);
	}
	else
	{
		CachedBBComp->ClearValue(MeleeEQSQueryKey);
	}
}

void ANSEnemyAIController::ResetMeleeEQSForCurrentTarget()
{
	if (!CachedBBComp)
	{
		return;
	}

	CachedBBComp->ClearValue(MeleeApproachLocationKey);

	UObject* QueryTemplate = CachedBBComp->GetValueAsObject(MeleeEQSQueryKey);

	const bool bCanRunMeleeEQS =
		IsValid(GetCurrentTargetActor()) &&
		UsesMeleeAttackReservation() &&
		IsValid(QueryTemplate);

	CachedBBComp->SetValueAsBool(MeleeEQSNeedsRefreshKey, bCanRunMeleeEQS);
}

void ANSEnemyAIController::HandleHitReactionStarted()
{
	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(GetPawn());

	if (!Enemy)
	{
		return;
	}

	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);

	Enemy->ClearCombatAimTarget();
	Enemy->SetRetreating(false);

	if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = true;
		Movement->bUseControllerDesiredRotation = false;
	}

	// 근접 예약은 반환하지 않고 공격 중에서 접근 중 상태로 되돌림
	if (AActor* ReservedTarget = MeleeReservationTarget.Get())
	{
		if (UNSMeleeAttackReservationComponent* Component =
			ReservedTarget->FindComponentByClass<UNSMeleeAttackReservationComponent>())
		{
			Component->MarkAttackInterrupted(Enemy);
		}
	}

	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(IsHitReactingKey, true);
		CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
		CachedBBComp->SetValueAsBool(TEXT("bIsAttacking"), false);
		CachedBBComp->SetValueAsBool(ShouldRetreatKey, false);
	}
}

void ANSEnemyAIController::HandleHitReactionFinished()
{
	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(IsHitReactingKey, false);
		CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
	}

	UpdateMeleeReservationState();
}

void ANSEnemyAIController::UpdateFacingMode(
	ANSEnemyCharacterBase* Enemy,
	AActor* TargetActor)
{
	if (!Enemy || !IsValidLivingTarget(TargetActor))
	{
		ApplyFacingMode(Enemy, nullptr, false);
		return;
	}

	const bool bIsAttacking = CachedBBComp && CachedBBComp->GetValueAsBool(TEXT("bIsAttacking"));
	const bool bShouldRetreat = CachedBBComp && CachedBBComp->GetValueAsBool(ShouldRetreatKey);
	const bool bPreparingAttack = IsWithinPotentialAttackRange(Enemy, TargetActor);
	const bool bFaceTarget = bIsAttacking || bShouldRetreat || bPreparingAttack;

	ApplyFacingMode(Enemy, TargetActor, bFaceTarget);
}

bool ANSEnemyAIController::IsWithinPotentialAttackRange(
	const ANSEnemyCharacterBase* Enemy,
	AActor* TargetActor
) const
{
	if (!Enemy || !IsValidLivingTarget(TargetActor))
	{
		return false;
	}

	const UNSEnemyData* EnemyData = Enemy->GetEnemyData();
	if (!EnemyData)
	{
		return false;
	}

	const UNSEnemyAttackComponent* AttackComponent = GetEnemyAttackComponent();
	if (!AttackComponent)
	{
		return false;
	}

	bool bHasDirectLineOfSight = false;
	AActor* AttackActor = ResolveAttackActor(TargetActor, bHasDirectLineOfSight);

	if (!IsValid(AttackActor))
	{
		return false;
	}

	const float Distance =
		FVector::Dist(Enemy->GetActorLocation(), TargetActor->GetActorLocation());

	for (const FNSEnemyAttackRow* AttackRow : EnemyData->GetAttackRows())
	{
		if (!AttackRow)
		{
			continue;
		}

		if (AttackComponent->CanUseAttack(
			*AttackRow,
			TargetActor,
			AttackActor,
			Distance,
			bHasDirectLineOfSight
		))
		{
			return true;
		}
	}

	return false;
}

void ANSEnemyAIController::ApplyFacingMode(
	ANSEnemyCharacterBase* Enemy,
	AActor* TargetActor,
	bool bFaceTarget)
{
	if (!Enemy)
	{
		return;
	}

	UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();

	if (!Movement)
	{
		return;
	}

	if (bFaceTarget && IsValid(TargetActor))
	{
		AActor* AimActor = GetCurrentAttackActor();
		if (!IsValid(AimActor))
		{
			AimActor = TargetActor;
		}

		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = true;

		SetFocus(AimActor, EAIFocusPriority::Gameplay);

		Enemy->UpdateCombatAimTarget(AimActor);
	}
	else
	{
		Movement->bOrientRotationToMovement = true;
		Movement->bUseControllerDesiredRotation = false;

		ClearFocus(EAIFocusPriority::Gameplay);
		Enemy->ClearCombatAimTarget();
	}
}

AActor* ANSEnemyAIController::GetCurrentAttackActor() const
{
	return CachedBBComp ? Cast<AActor>(CachedBBComp->GetValueAsObject(AttackActorKey)) : nullptr;
}

void ANSEnemyAIController::SetAttackActorBlackboard(AActor* AttackActor)
{
	if (!CachedBBComp) return;

	if (IsValid(AttackActor))
	{
		CachedBBComp->SetValueAsObject(AttackActorKey, AttackActor);
	}
	else
	{
		CachedBBComp->ClearValue(AttackActorKey);
	}
}

AActor* ANSEnemyAIController::ResolveAttackActor(
	AActor* TargetActor,
	bool& bOutHasDirectLineOfSight
) const
{
	bOutHasDirectLineOfSight = false;

	const UNSEnemyTargetComponent* TargetComponent = GetEnemyTargetComponent();
	if (!TargetComponent)
	{
		return nullptr;
	}

	return TargetComponent->ResolveAttackActor(
		TargetActor,
		bOutHasDirectLineOfSight
	);
}

UNSEnemyTargetComponent* ANSEnemyAIController::GetEnemyTargetComponent() const
{
	const APawn* ControlledPawn = GetPawn();

	return ControlledPawn
		       ? ControlledPawn->FindComponentByClass<UNSEnemyTargetComponent>()
		       : nullptr;
}

UNSEnemyThreatComponent* ANSEnemyAIController::GetEnemyThreatComponent() const
{
	const APawn* ControlledPawn = GetPawn();

	return ControlledPawn
		       ? ControlledPawn->FindComponentByClass<UNSEnemyThreatComponent>()
		       : nullptr;
}
