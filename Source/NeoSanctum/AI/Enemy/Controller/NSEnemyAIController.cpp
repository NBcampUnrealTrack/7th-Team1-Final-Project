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
		UpdateRetreatState(TargetActor);
		UpdateFacingMode(TargetActor);
	}
	else
	{
		if (UNSEnemyMoveComponent* MoveComponent = GetEnemyMoveComponent())
		{
			MoveComponent->ApplyFacing(this, nullptr, nullptr, false);
			MoveComponent->ClearRetreat();
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
		SetCanAttackBB(false);
		return false;
	}

	if (IsControlledEnemyHitReacting())
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

		CachedBBComp->SetValueAsBool(ShouldRetreatKey, false);
		CachedBBComp->ClearValue(RetreatLocationKey);
		return;
	}

	const bool bWasRetreating = CachedBBComp->GetValueAsBool(ShouldRetreatKey);
	const bool bHasRetreatLocation = CachedBBComp->IsVectorValueSet(RetreatLocationKey);
	const FVector CurrentRetreatLocation = CachedBBComp->GetValueAsVector(RetreatLocationKey);

	const FNSRetreatResult Result = MoveComponent->UpdateRetreat(
		TargetActor,
		bWasRetreating,
		bHasRetreatLocation,
		CurrentRetreatLocation
	);

	CachedBBComp->SetValueAsBool(ShouldRetreatKey, Result.bShouldRetreat);

	if (Result.bShouldRetreat && Result.bHasLocation)
	{
		CachedBBComp->SetValueAsVector(RetreatLocationKey, Result.Location);
	}
	else
	{
		CachedBBComp->ClearValue(RetreatLocationKey);
	}
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

	SetHitReactBB(true);
	ClearAttackBB();
	ClearRetreatBB();
}

void ANSEnemyAIController::HandleHitReactionFinished()
{
	SetHitReactBB(false);
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

	if (!IsValidLivingTarget(TargetActor))
	{
		MoveComponent->ApplyFacing(this, nullptr, nullptr, false);
		return;
	}

	const bool bIsAttacking =
		CachedBBComp && CachedBBComp->GetValueAsBool(TEXT("bIsAttacking"));

	const bool bShouldRetreat =
		CachedBBComp && CachedBBComp->GetValueAsBool(ShouldRetreatKey);

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

void ANSEnemyAIController::InitBBState()
{
	if (!CachedBBComp)
	{
		return;
	}

	SetHitReactBB(false);
	ClearAttackBB();
	ClearRetreatBB();
	ClearTargetBB(false);
	SetMeleeReservationBlackboard(false, false);

	CachedBBComp->SetValueAsBool(PhasePatternLockedKey, false);
}

void ANSEnemyAIController::SetCanAttackBB(bool bCanAttack)
{
	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), bCanAttack);
	}
}

void ANSEnemyAIController::SetHitReactBB(bool bHitReacting)
{
	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(IsHitReactingKey, bHitReacting);
	}
}

void ANSEnemyAIController::ClearAttackBB()
{
	if (!CachedBBComp)
	{
		return;
	}

	CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
	CachedBBComp->SetValueAsBool(TEXT("bIsAttacking"), false);
	SetAttackActorBlackboard(nullptr);
}

void ANSEnemyAIController::ClearTargetBB(bool bClearCanAttack)
{
	if (!CachedBBComp)
	{
		return;
	}

	CachedBBComp->ClearValue(TargetActorKey);
	CachedBBComp->ClearValue(AttackActorKey);
	CachedBBComp->ClearValue(TargetLastKnownLocationKey);
	CachedBBComp->SetValueAsBool(HasTargetLineOfSightKey, false);

	if (bClearCanAttack)
	{
		SetCanAttackBB(false);
	}
}

void ANSEnemyAIController::ClearRetreatBB()
{
	if (!CachedBBComp)
	{
		return;
	}

	CachedBBComp->SetValueAsBool(ShouldRetreatKey, false);
	CachedBBComp->ClearValue(RetreatLocationKey);
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
