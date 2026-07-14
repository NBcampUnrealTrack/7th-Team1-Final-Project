// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyAIController.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/Interaction/Prop/NSDestructibleObjectBase.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "Perception/AIPerceptionComponent.h"
#include "NeoSanctum/AI/Enemy/Controller/NSEnemyControllerBase.h"
#include "NeoSanctum/Combat/Component/NSEnemyAttackComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyMeleeComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyMoveComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyTargetComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"
#include "NeoSanctum/Type/NSBBTypes.h"


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
	const bool bIsTraversingNavLink = Enemy && Enemy->IsTraversingNavLink();

	UpdateEnemyPhase();

	if (IsPhasePatternLocked())
	{
		StopMovement();

		ClearAttackBB();
		ClearRetreatBB();

		return;
	}

	if (Enemy && Enemy->IsHitReacting())
	{
		StopMovement();

		ClearAttackBB();
		ClearRetreatBB();

		return;
	}

	if (UNSEnemyMoveComponent* MoveComponent = GetEnemyMoveComponent())
	{
		if (IsAttackingBB() || bIsTraversingNavLink)
		{
			MoveComponent->ResetNavigationRecovery();
		}
		else
		{
			MoveComponent->UpdateNavigationRecovery(this, DeltaTime);
		}
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

	if (bIsTraversingNavLink)
	{
		ClearRetreatBB();
		SetCanAttackBB(false);
		SetAttackActorBlackboard(nullptr);
		return;
	}

	if (IsValidLivingTarget(TargetActor))
	{
		UpdateResolvedTargetMoveBlackboard(TargetActor);
		UpdateRetreatState(TargetActor);
		UpdateFacingMode(TargetActor);
	}
	else
	{
		ResetResolvedTargetMoveState();

		if (UNSEnemyMoveComponent* MoveComponent = GetEnemyMoveComponent())
		{
			MoveComponent->ApplyFacing(this, nullptr, nullptr, false);
			MoveComponent->ClearRetreat();
		}

		if (CachedBBComp)
		{
			CachedBBComp->SetValueAsBool(NSBB::Movement::ShouldRetreat, false);
			CachedBBComp->ClearValue(NSBB::Movement::RetreatLocation);
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
		SetCanAttackBB(false);
		return false;
	}

	if (IsControlledEnemyHitReacting())
	{
		SetCanAttackBB(false);
		return false;
	}

	const ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(GetPawn());
	if (Enemy && Enemy->IsTraversingNavLink())
	{
		SetCanAttackBB(false);
		return false;
	}

	const FNSEnemyAttackRow* UsableAttack = FindAttackRowByDistance(false);

	SetCanAttackBB(UsableAttack != nullptr);

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
		SetCanAttackBB(false);
		return nullptr;
	}

	if (IsControlledEnemyHitReacting())
	{
		SetCanAttackBB(false);
		return nullptr;
	}

	const ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(GetPawn());
	if (Enemy && Enemy->IsTraversingNavLink())
	{
		SetCanAttackBB(false);
		SetAttackActorBlackboard(nullptr);
		return nullptr;
	}

	const FNSEnemyAttackRow* SelectedAttack = FindAttackRowByDistance(true);

	SetCanAttackBB(SelectedAttack != nullptr);

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

	if (UNSEnemyMeleeComponent* MeleeComponent = GetEnemyMeleeComponent())
	{
		MeleeComponent->ResetMeleeState();
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
	
	if (UNSEnemyMoveComponent* MoveComponent = GetEnemyMoveComponent())
	{
		MoveComponent->ResetNavigationRecovery();
	}

	StartEnemyBrain(EnemyData);

	InitBBState();
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
	
	if (UNSEnemyMoveComponent* MoveComponent = GetEnemyMoveComponent())
	{
		MoveComponent->ResetNavigationRecovery();
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
		const bool bWasCurrentTarget = GetCurrentTargetActor() == Actor;

		ThreatComponent->RemoveTarget(Actor, false);

		if (bWasCurrentTarget)
		{
			ClearCurrentCombatTarget(false);
		}
		else
		{
			UpdateCurrentTargetBlackboard();
		}

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

	AActor* TargetActor = GetTargetActorBB();

	APawn* AIPawn = GetPawn();
	if (!AIPawn || !IsValidLivingTarget(TargetActor))
	{
		SetTargetActorBB(nullptr);
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

void ANSEnemyAIController::UpdateRetreatState(AActor* TargetActor)
{
	if (!CachedBBComp)
	{
		return;
	}

	UNSEnemyMoveComponent* MoveComponent = GetEnemyMoveComponent();

	if (!MoveComponent || !IsValid(TargetActor))
	{
		if (MoveComponent)
		{
			MoveComponent->ClearRetreat();
		}

		CachedBBComp->SetValueAsBool(NSBB::Movement::ShouldRetreat, false);
		CachedBBComp->ClearValue(NSBB::Movement::RetreatLocation);
		return;
	}

	const bool bWasRetreating =
		CachedBBComp->GetValueAsBool(NSBB::Movement::ShouldRetreat);

	const bool bHasRetreatLocation =
		CachedBBComp->IsVectorValueSet(NSBB::Movement::RetreatLocation);

	const FVector CurrentRetreatLocation =
		CachedBBComp->GetValueAsVector(NSBB::Movement::RetreatLocation);

	const FNSRetreatResult Result = MoveComponent->UpdateRetreat(
		TargetActor,
		bWasRetreating,
		bHasRetreatLocation,
		CurrentRetreatLocation
	);

	CachedBBComp->SetValueAsBool(NSBB::Movement::ShouldRetreat, Result.bShouldRetreat);

	if (Result.bShouldRetreat && Result.bHasLocation)
	{
		CachedBBComp->SetValueAsVector(NSBB::Movement::RetreatLocation, Result.Location);
	}
	else
	{
		CachedBBComp->ClearValue(NSBB::Movement::RetreatLocation);
	}
}

void ANSEnemyAIController::NotifyAttackStarted()
{
	Super::NotifyAttackStarted();

	if (UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent())
	{
		ThreatComponent->NotifyAttackStarted();
	}
}

void ANSEnemyAIController::NotifyAttackFinished()
{
	Super::NotifyAttackFinished();

	ClearAttackBB();
}

void ANSEnemyAIController::UpdateTargetSelection()
{
	UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent();
	if (!ThreatComponent)
	{
		return;
	}

	AActor* CurrentTarget = ThreatComponent->GetCurrentTarget();

	const bool bIsAttacking = IsAttackingBB();

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
		ResetResolvedTargetMoveState();
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

	ClearTargetBB(true);
}

void ANSEnemyAIController::ResetTargetingState()
{
	CancelMeleeReservationRequest(false);

	if (UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent())
	{
		ThreatComponent->ResetThreatState();
	}

	ResetMeleeEQSForCurrentTarget();

	ClearTargetBB(true);
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
		ClearTargetBB(false);
		return;
	}

	SetTargetActorBB(TargetActor);

	FVector LastKnownLocation = TargetActor->GetActorLocation();

	if (const UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent())
	{
		ThreatComponent->TryGetLastKnownLocation(TargetActor, LastKnownLocation);
	}

	SetTargetLastKnownLocationBB(LastKnownLocation);

	bool bHasDirectLineOfSight = false;
	AActor* AttackActor = ResolveAttackActor(TargetActor, bHasDirectLineOfSight);

	SetAttackActorBlackboard(AttackActor);
	SetHasTargetLineOfSightBB(bHasDirectLineOfSight);
}

void ANSEnemyAIController::UpdateResolvedTargetMoveBlackboard(AActor* TargetActor)
{
	if (!CachedBBComp)
	{
		return;
	}

	UNSEnemyMoveComponent* MoveComponent = GetEnemyMoveComponent();
	if (!MoveComponent || !IsValidLivingTarget(TargetActor))
	{
		ClearResolvedTargetMoveBlackboard();
		return;
	}

	const FNSResolvedTargetMoveResult Result =
		MoveComponent->ResolveTargetMoveLocation(TargetActor, this);

	CachedBBComp->SetValueAsVector(
		NSBB::Target::ActualLocation,
		Result.ActualLocation);

	CachedBBComp->SetValueAsEnum(
		NSBB::Target::MoveResolveType,
		static_cast<uint8>(Result.ResolveType));

	CachedBBComp->SetValueAsBool(
		NSBB::Target::IsAirborne,
		Result.bTargetAirborne);

	CachedBBComp->SetValueAsBool(
		NSBB::Movement::HasResolvedTargetMoveLocation,
		Result.bHasMoveLocation);

	CachedBBComp->SetValueAsBool(
		NSBB::Movement::ArrivedBelowAirborneTarget,
		Result.bArrivedBelowAirborneTarget);

	if (Result.bHasMoveLocation)
	{
		CachedBBComp->SetValueAsVector(
			NSBB::Movement::ResolvedTargetMoveLocation,
			Result.MoveLocation);
	}
	else
	{
		CachedBBComp->ClearValue(
			NSBB::Movement::ResolvedTargetMoveLocation);
	}
}

void ANSEnemyAIController::ClearResolvedTargetMoveBlackboard()
{
	if (!CachedBBComp)
	{
		return;
	}

	CachedBBComp->ClearValue(NSBB::Target::ActualLocation);

	CachedBBComp->SetValueAsEnum(
		NSBB::Target::MoveResolveType,
		static_cast<uint8>(ENSTargetMoveResolveType::Invalid));

	CachedBBComp->SetValueAsBool(NSBB::Target::IsAirborne, false);

	CachedBBComp->ClearValue(NSBB::Movement::ResolvedTargetMoveLocation);
	CachedBBComp->SetValueAsBool(NSBB::Movement::HasResolvedTargetMoveLocation, false);
	CachedBBComp->SetValueAsBool(NSBB::Movement::ArrivedBelowAirborneTarget, false);
}

void ANSEnemyAIController::ResetResolvedTargetMoveState()
{
	if (UNSEnemyMoveComponent* MoveComponent = GetEnemyMoveComponent())
	{
		MoveComponent->ResetTargetMoveResolveState();
	}

	ClearResolvedTargetMoveBlackboard();
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
	UNSEnemyMeleeComponent* MeleeComponent = GetEnemyMeleeComponent();
	if (!MeleeComponent)
	{
		SetMeleeReservationBlackboard(false, true);
		return false;
	}

	const FNSMeleeState State = MeleeComponent->RequestReservation(
		GetCurrentTargetActor(),
		GetLatestDamageTimeFromCurrentTarget()
	);

	SetMeleeReservationBlackboard(State.bHasReservation, State.bCanApproach);

	return State.bAccepted;
}

bool ANSEnemyAIController::HasMeleeAttackReservation() const
{
	const UNSEnemyMeleeComponent* MeleeComponent = GetEnemyMeleeComponent();
	return MeleeComponent && MeleeComponent->HasReservation();
}

bool ANSEnemyAIController::CanApproachMeleeTarget() const
{
	const UNSEnemyMeleeComponent* MeleeComponent = GetEnemyMeleeComponent();

	return MeleeComponent
		       ? MeleeComponent->CanApproachTarget(GetCurrentTargetActor())
		       : true;
}

bool ANSEnemyAIController::CurrentTargetRequiresMeleeReservation() const
{
	const UNSEnemyMeleeComponent* MeleeComponent = GetEnemyMeleeComponent();

	return MeleeComponent &&
		MeleeComponent->TargetRequiresReservation(GetCurrentTargetActor());
}

void ANSEnemyAIController::NotifyMeleeReservationAttackStarted()
{
	if (UNSEnemyMeleeComponent* MeleeComponent = GetEnemyMeleeComponent())
	{
		MeleeComponent->NotifyAttackStarted();
	}
}

void ANSEnemyAIController::ReleaseMeleeAttackReservation(bool bStartReacquireCooldown)
{
	CancelMeleeReservationRequest(bStartReacquireCooldown);

	ResetMeleeEQSForCurrentTarget();
}

void ANSEnemyAIController::UpdateMeleeReservationState()
{
	UNSEnemyMeleeComponent* MeleeComponent = GetEnemyMeleeComponent();

	if (!MeleeComponent)
	{
		SetMeleeReservationBlackboard(false, true);
		return;
	}

	const FNSMeleeState State =
		MeleeComponent->UpdateState(GetCurrentTargetActor());

	SetMeleeReservationBlackboard(State.bHasReservation, State.bCanApproach);
}

void ANSEnemyAIController::CancelMeleeReservationRequest(bool bStartReacquireCooldown)
{
	if (UNSEnemyMeleeComponent* MeleeComponent = GetEnemyMeleeComponent())
	{
		MeleeComponent->ReleaseReservation(bStartReacquireCooldown);
	}

	SetMeleeReservationBlackboard(false, false);
}

bool ANSEnemyAIController::UsesMeleeAttackReservation() const
{
	const UNSEnemyMeleeComponent* MeleeComponent = GetEnemyMeleeComponent();
	return MeleeComponent && MeleeComponent->UsesReservation();
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

	CachedBBComp->SetValueAsBool(NSBB::Melee::HasAttackReservation, bHasReservation);
	CachedBBComp->SetValueAsBool(NSBB::Melee::CanApproachTarget, bCanApproach);
}

void ANSEnemyAIController::InitializeMeleeEQSBlackboard(const UNSEnemyData* EnemyData)
{
	if (!CachedBBComp)
	{
		return;
	}

	CachedBBComp->ClearValue(NSBB::Melee::ApproachLocation);
	CachedBBComp->SetValueAsBool(NSBB::Melee::EQSNeedsRefresh, false);

	if (EnemyData && EnemyData->EQSQuery)
	{
		CachedBBComp->SetValueAsObject(NSBB::Melee::EQSQuery, EnemyData->EQSQuery);
	}
	else
	{
		CachedBBComp->ClearValue(NSBB::Melee::EQSQuery);
	}
}

void ANSEnemyAIController::ResetMeleeEQSForCurrentTarget()
{
	if (!CachedBBComp)
	{
		return;
	}

	CachedBBComp->ClearValue(NSBB::Melee::ApproachLocation);

	UObject* QueryTemplate = CachedBBComp->GetValueAsObject(NSBB::Melee::EQSQuery);

	const bool bCanRunMeleeEQS =
		IsValid(GetCurrentTargetActor()) &&
		UsesMeleeAttackReservation() &&
		IsValid(QueryTemplate);

	CachedBBComp->SetValueAsBool(NSBB::Melee::EQSNeedsRefresh, bCanRunMeleeEQS);
}

void ANSEnemyAIController::HandleHitReactionStarted()
{
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);

	if (UNSEnemyMoveComponent* MoveComponent = GetEnemyMoveComponent())
	{
		MoveComponent->ApplyFacing(this, nullptr, nullptr, false);
		MoveComponent->ClearRetreat();
	}

	if (UNSEnemyMeleeComponent* MeleeComponent = GetEnemyMeleeComponent())
	{
		MeleeComponent->MarkAttackInterrupted();
	}

	SetHitReactingBB(true);
	ClearAttackBB();
	ClearRetreatBB();
}

void ANSEnemyAIController::HandleHitReactionFinished()
{
	SetHitReactingBB(false);
	SetCanAttackBB(false);

	UpdateMeleeReservationState();
}

void ANSEnemyAIController::UpdateFacingMode(AActor* TargetActor)
{
	UNSEnemyMoveComponent* MoveComponent = GetEnemyMoveComponent();
	if (!MoveComponent)
	{
		return;
	}

	const ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(GetPawn());
	if (Enemy && Enemy->IsTraversingNavLink())
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}

	if (!IsValidLivingTarget(TargetActor))
	{
		MoveComponent->ApplyFacing(this, nullptr, nullptr, false);
		return;
	}

	const bool bIsAttacking = IsAttackingBB();

	const bool bShouldRetreat =
		CachedBBComp && CachedBBComp->GetValueAsBool(NSBB::Movement::ShouldRetreat);

	const bool bPreparingAttack =
		MoveComponent->IsWithinAttackRange(TargetActor);

	const bool bFaceTarget =
		bIsAttacking || bShouldRetreat || bPreparingAttack;

	AActor* AimActor = GetCurrentAttackActor();
	if (!IsValid(AimActor))
	{
		AimActor = TargetActor;
	}

	MoveComponent->ApplyFacing(this, TargetActor, AimActor, bFaceTarget);
}

AActor* ANSEnemyAIController::GetCurrentAttackActor() const
{
	return GetAttackActorBB();
}

void ANSEnemyAIController::SetAttackActorBlackboard(AActor* AttackActor)
{
	SetAttackActorBB(AttackActor);
}

void ANSEnemyAIController::InitBBState()
{
	if (!CachedBBComp)
	{
		return;
	}

	SetHitReactingBB(false);
	ClearAttackBB();
	ClearRetreatBB();
	ClearTargetBB(false);
	SetMeleeReservationBlackboard(false, false);

	CachedBBComp->SetValueAsBool(NSBB::Phase::PhasePatternLocked, false);
	CachedBBComp->SetValueAsName(NSBB::Phase::CurrentPhaseId, NAME_None);
}

void ANSEnemyAIController::ClearAttackBB()
{
	if (!CachedBBComp)
	{
		return;
	}

	SetCanAttackBB(false);
	SetIsAttackingBB(false);
	SetAttackActorBlackboard(nullptr);
}

void ANSEnemyAIController::ClearTargetBB(bool bClearCanAttack)
{
	ClearCommonTargetBB(bClearCanAttack);
	ResetResolvedTargetMoveState();
}

void ANSEnemyAIController::ClearRetreatBB()
{
	if (!CachedBBComp)
	{
		return;
	}

	CachedBBComp->SetValueAsBool(NSBB::Movement::ShouldRetreat, false);
	CachedBBComp->ClearValue(NSBB::Movement::RetreatLocation);
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

UNSEnemyMeleeComponent* ANSEnemyAIController::GetEnemyMeleeComponent() const
{
	const APawn* ControlledPawn = GetPawn();

	return ControlledPawn
		       ? ControlledPawn->FindComponentByClass<UNSEnemyMeleeComponent>()
		       : nullptr;
}

UNSEnemyMoveComponent* ANSEnemyAIController::GetEnemyMoveComponent() const
{
	const APawn* ControlledPawn = GetPawn();

	return ControlledPawn
		       ? ControlledPawn->FindComponentByClass<UNSEnemyMoveComponent>()
		       : nullptr;
}
