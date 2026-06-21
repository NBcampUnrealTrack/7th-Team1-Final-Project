// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyAIController.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Combat/Component/NSMeleeAttackReservationComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"


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
	
	const double CurrentTime = World->GetTimeSeconds();

	if (CurrentTime >= NextTargetEvaluationTime)
	{
		UpdateTargetSelection();

		NextTargetEvaluationTime = CurrentTime + TargetEvaluationInterval;
	}

	AActor* TargetActor = GetCurrentTargetActor();
	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(GetPawn());

	if (IsValidLivingTarget(TargetActor))
	{
		SetFocus(TargetActor, EAIFocusPriority::Gameplay);

		if (Enemy)
		{
			Enemy->UpdateCombatAimTarget(TargetActor);
			UpdateRetreatState(Enemy, TargetActor);
		}
	}
	else
	{
		ClearFocus(EAIFocusPriority::Gameplay);

		if (Enemy)
		{
			Enemy->ClearCombatAimTarget();
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
	const FNSEnemyAttackDefinition* UsableAttack = FindAttackDefinitionByDistance(false);

	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), UsableAttack != nullptr);
	}

	return UsableAttack != nullptr;
}

void ANSEnemyAIController::RecordAttackUsed(const FNSEnemyAttackDefinition& AttackDefinition)
{
	NotifyAttackStarted();
	
	if (AttackDefinition.Cooldown <= 0.0f || AttackDefinition.AttackId.IsNone())
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	LastAttackTimeById.FindOrAdd(AttackDefinition.AttackId) = World->GetTimeSeconds();
}

const FNSEnemyAttackDefinition* ANSEnemyAIController::GetAttackDefinitionByDistance()
{
	const FNSEnemyAttackDefinition* SelectedAttack = FindAttackDefinitionByDistance(true);

	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), SelectedAttack != nullptr);
	}

	return SelectedAttack;
}

AActor* ANSEnemyAIController::GetCurrentTargetActor() const
{
	return CurrentCombatTarget.Get();
}

void ANSEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	// 쿨다운 시간 초기화
	LastAttackTimeById.Reset();

	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(InPawn);
	if (!Enemy) return;

	UNSEnemyData* EnemyData = Enemy->GetEnemyData();
	if (!EnemyData) return;

	if (EnemyData->BehaviorTree)
	{
		RunBehaviorTree(EnemyData->BehaviorTree);
		CachedBBComp = GetBlackboardComponent();
		ResetTargetingState();
	}
}

void ANSEnemyAIController::OnUnPossess()
{
	ResetTargetingState();
	CachedBBComp = nullptr;
	
	Super::OnUnPossess();
}

void ANSEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// 타깃이 존재하지 않으면 반환
	if (!Actor)
	{
		return;
	}

	// 타깃 액터가 아군 혹은 중립인 경우 반환
	if (GetTeamAttitudeTo(*Actor) != ETeamAttitude::Hostile)
	{
		return;
	}

	// 타깃이 될 수 없으면 반환
	if (!IsValidLivingTarget(Actor))
	{
		ThreatRecords.Remove(Actor);

		if (CurrentCombatTarget == Actor)
		{
			ClearCurrentCombatTarget(false);
		}

		return;
	}

	// 시각·청각·피해 감지 결과를 해당 타깃의 Threat 기록에 반영
	UpdateThreatFromStimulus(Actor, Stimulus);

	// 새로 감지된 타깃은 다음 Tick까지 기다리지 않고 평가
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

const FNSEnemyAttackDefinition* ANSEnemyAIController::FindAttackDefinitionByDistance(bool bSelectWeightedAttack)
{
	if (!CachedBBComp)
	{
		return nullptr;
	}

	AActor* TargetActor = Cast<AActor>(CachedBBComp->GetValueAsObject(TargetActorKey));

	APawn* AIPawn = GetPawn();
	if (!AIPawn || !IsValidLivingTarget(TargetActor))
	{
		CachedBBComp->SetValueAsObject(TargetActorKey, nullptr);
		return nullptr;
	}

	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(AIPawn);
	if (!Enemy)
	{
		return nullptr;
	}

	const UNSEnemyData* EnemyData = Enemy->GetEnemyData();
	if (!EnemyData)
	{
		return nullptr;
	}

	const FVector ToTarget = (TargetActor->GetActorLocation() - AIPawn->GetActorLocation()).GetSafeNormal2D();
	const FVector Forward = AIPawn->GetActorForwardVector().GetSafeNormal2D();

	const float FacingDot = FVector::DotProduct(Forward, ToTarget);
	const float RequiredDot = FMath::Cos(FMath::DegreesToRadians(AttackFacingAngleDegrees));

	const bool bFacingTarget = FacingDot >= RequiredDot;
	if (!bFacingTarget)
	{
		return nullptr;
	}

	const bool bHasLineOfSight = LineOfSightTo(TargetActor);

	// 몬스터와 플레이어 간의 실시간 직선 거리 계산
	const float Distance = FVector::Dist(AIPawn->GetActorLocation(), TargetActor->GetActorLocation());

	TArray<const FNSEnemyAttackDefinition*> Candidates;
	int32 BestPriority = TNumericLimits<int32>::Lowest();

	for (const FNSEnemyAttackDefinition& AttackDefinition : EnemyData->AttackList)
	{
		// 개별 공격 조건 검사
		if (!CanUseAttackDefinition(AttackDefinition, TargetActor, Distance, bHasLineOfSight))
		{
			continue;
		}

		// 사용 가능한 공격 후보 추가
		if (!bSelectWeightedAttack)
		{
			return &AttackDefinition;
		}

		if (AttackDefinition.Priority > BestPriority)
		{
			BestPriority = AttackDefinition.Priority;
			Candidates.Reset();
		}

		if (AttackDefinition.Priority == BestPriority)
		{
			Candidates.Add(&AttackDefinition);
		}
	}

	if (Candidates.IsEmpty())
	{
		return nullptr;
	}

	// 가장 높은 가중치의 공격 선택
	float TotalWeight = 0.0f;
	for (const FNSEnemyAttackDefinition* Candidate : Candidates)
	{
		TotalWeight += FMath::Max(Candidate->Weight, 0.0f);
	}

	if (TotalWeight <= 0.0f)
	{
		return Candidates[0];
	}

	float Pick = FMath::FRandRange(0.0f, TotalWeight);

	for (const FNSEnemyAttackDefinition* Candidate : Candidates)
	{
		Pick -= FMath::Max(Candidate->Weight, 0.0f);

		if (Pick <= 0.0f)
		{
			return Candidate;
		}
	}

	return Candidates.Last();
}

bool ANSEnemyAIController::CanUseAttackDefinition(
	const FNSEnemyAttackDefinition& AttackDefinition,
	const AActor* TargetActor,
	float Distance,
	bool bHasLineOfSight) const
{
	if (!TargetActor)
	{
		return false;
	}
	
	if (!AttackDefinition.AbilityClass)
	{
		return false;
	}

	if (Distance < AttackDefinition.Condition.MinRange ||
		Distance > AttackDefinition.Condition.MaxRange)
	{
		return false;
	}

	if (AttackDefinition.Condition.bRequireLineOfSight && !bHasLineOfSight)
	{
		return false;
	}
	
	if (AttackDefinition.Cooldown > 0.0f)
	{
		if (AttackDefinition.AttackId.IsNone())
		{
			return false;
		}

		const float* LastAttackTime = LastAttackTimeById.Find(AttackDefinition.AttackId);
		if (LastAttackTime)
		{
			const UWorld* World = GetWorld();
			if (!World)
			{
				return false;
			}

			const float ElapsedTime = World->GetTimeSeconds() - *LastAttackTime;
			if (ElapsedTime < AttackDefinition.Cooldown)
			{
				return false;
			}
		}
	}

	return true;
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

	for (const FNSEnemyAttackDefinition& Attack : EnemyData->AttackList)
	{
		if (!Attack.AbilityClass)
		{
			continue;
		}

		bFoundAttack = true;
		MinimumRange = FMath::Min(MinimumRange, Attack.Condition.MinRange);
	}

	return bFoundAttack ? MinimumRange : 0.0f;
}


void ANSEnemyAIController::NotifyAttackStarted()
{
	if (!CurrentCombatTarget.IsValid() || !GetWorld())
	{
		return;
	}

	bAttackStartedOnCurrentTarget = true;
	LastCombatProgressTime = GetWorld()->GetTimeSeconds();
}

void ANSEnemyAIController::UpdateTargetSelection()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const double CurrentTime = World->GetTimeSeconds();

	PruneThreatRecords(CurrentTime);

	AActor* CurrentTarget = CurrentCombatTarget.Get();

	const bool bIsAttacking = CachedBBComp && CachedBBComp->GetValueAsBool(TEXT("bIsAttacking"));

	if (CurrentTarget && !IsValidLivingTarget(CurrentTarget))
	{
		ClearCurrentCombatTarget(false);
		CurrentTarget = nullptr;
	}

	// 공격 애니메이션 도중에는 유효한 타깃을 변경하지 않음
	if (CurrentTarget && bIsAttacking)
	{
		UpdateCurrentTargetBlackboard();
		return;
	}

	// 공격 없이 추적만 10초 이상 지속 시 현재 타깃에서 제거
	if (CurrentTarget &&
		CurrentTime - LastCombatProgressTime >= MaxPursuitWithoutAttackDuration)
	{
		ClearCurrentCombatTarget(true);
		CurrentTarget = nullptr;
	}

	// 시야 기억, 피해 기록, 청각 기록이 모두 만료되면 현재 타깃에서 제거
	if (CurrentTarget && !ThreatRecords.Contains(CurrentTarget))
	{
		ClearCurrentCombatTarget(false);
		CurrentTarget = nullptr;
	}

	AActor* BestTarget = FindBestTarget(CurrentTime);

	if (!CurrentTarget)
	{
		if (BestTarget)
		{
			SetCurrentCombatTarget(BestTarget);
		}

		return;
	}

	if (BestTarget && ShouldSwitchTarget(BestTarget, CurrentTime))
	{
		SetCurrentCombatTarget(BestTarget);
		return;
	}

	UpdateCurrentTargetBlackboard();
}

void ANSEnemyAIController::UpdateThreatFromStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
	// 월드 혹은 타깃이 존재하지 않으면 반환
	UWorld* World = GetWorld();
	if (!World || !Actor)
	{
		return;
	}

	const double CurrentTime = World->GetTimeSeconds();
	FNSTargetThreatRecord& Record = ThreatRecords.FindOrAdd(Actor);
	Record.TargetActor = Actor;

	const FAISenseID SightID = UAISense::GetSenseID<UAISense_Sight>();
	const FAISenseID HearingID = UAISense::GetSenseID<UAISense_Hearing>();
	const FAISenseID DamageID = UAISense::GetSenseID<UAISense_Damage>();

	if (Stimulus.Type == SightID)
	{
		Record.bCurrentlyVisible = Stimulus.WasSuccessfullySensed();

		if (Stimulus.WasSuccessfullySensed())
		{
			Record.LastSeenTime = CurrentTime;
			Record.LastKnownLocation = Actor->GetActorLocation();
		}
	}
	else if (Stimulus.Type == HearingID)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			Record.LastStimulusTime = CurrentTime;
			Record.LastKnownLocation = Actor->GetActorLocation();
		}
	}
	else if (Stimulus.Type == DamageID)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			Record.LastStimulusTime = CurrentTime;
			Record.LastKnownLocation = Actor->GetActorLocation();

			FNSThreatDamageSample& DamageSample = Record.DamageSamples.AddDefaulted_GetRef();

			DamageSample.Timestamp = CurrentTime;
			DamageSample.Damage = FMath::Max(Stimulus.Strength, 0.0f);
		}
	}
}

void ANSEnemyAIController::PruneThreatRecords(double CurrentTime)
{
	for (auto It = ThreatRecords.CreateIterator(); It; ++It)
	{
		FNSTargetThreatRecord& Record = It.Value();
		AActor* TargetActor = Record.TargetActor.Get();

		if (!IsValidLivingTarget(TargetActor))
		{
			It.RemoveCurrent();
			continue;
		}

		const double DamageCutoff = CurrentTime - DamageThreatWindow;

		Record.DamageSamples.RemoveAll(
			[DamageCutoff](const FNSThreatDamageSample& Sample)
			{
				return Sample.Timestamp < DamageCutoff;
			});

		if (!IsThreatRecordRelevant(Record, CurrentTime))
		{
			It.RemoveCurrent();
		}
	}

	for (auto It = ReacquireBlockedUntil.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() ||
			It.Value() <= CurrentTime)
		{
			It.RemoveCurrent();
		}
	}
}

AActor* ANSEnemyAIController::FindBestTarget(double CurrentTime) const
{
	const APawn* EnemyPawn = GetPawn();
	if (!EnemyPawn)
	{
		return nullptr;
	}

	AActor* BestDamageTarget = nullptr;
	float BestDamageThreat = 0.0f;
	float BestDamageTargetDistanceSq = TNumericLimits<float>::Max();

	AActor* NearestTarget = nullptr;
	float NearestDistanceSq = TNumericLimits<float>::Max();

	for (const auto& Pair : ThreatRecords)
	{
		AActor* TargetActor = Pair.Key.Get();
		const FNSTargetThreatRecord& Record = Pair.Value;

		if (!IsValidLivingTarget(TargetActor) || !IsThreatRecordRelevant(Record, CurrentTime))
		{
			continue;
		}

		if (const double* BlockedUntil = ReacquireBlockedUntil.Find(Pair.Key))
		{
			if (*BlockedUntil > CurrentTime)
			{
				continue;
			}
		}

		const float DistanceSq = FVector::DistSquared(EnemyPawn->GetActorLocation(), TargetActor->GetActorLocation());
		const float DamageThreat = GetRecentDamageThreat(Record, CurrentTime);

		if (DamageThreat > BestDamageThreat ||
			(FMath::IsNearlyEqual(DamageThreat, BestDamageThreat) &&
			 DistanceSq < BestDamageTargetDistanceSq))
		{
			BestDamageThreat = DamageThreat;
			BestDamageTarget = TargetActor;
			BestDamageTargetDistanceSq = DistanceSq;
		}

		if (DistanceSq < NearestDistanceSq)
		{
			NearestTarget = TargetActor;
			NearestDistanceSq = DistanceSq;
		}
	}

	return BestDamageThreat > 0.0f ? BestDamageTarget : NearestTarget;
}

float ANSEnemyAIController::GetRecentDamageThreat(const FNSTargetThreatRecord& Record, double CurrentTime) const
{
	const double DamageCutoff = CurrentTime - DamageThreatWindow;

	float TotalDamage = 0.0f;

	for (const FNSThreatDamageSample& Sample : Record.DamageSamples)
	{
		if (Sample.Timestamp >= DamageCutoff)
		{
			TotalDamage += Sample.Damage;
		}
	}

	return TotalDamage;
}

bool ANSEnemyAIController::IsThreatRecordRelevant(const FNSTargetThreatRecord& Record, double CurrentTime) const
{
	if (!Record.TargetActor.IsValid())
	{
		return false;
	}

	if (Record.bCurrentlyVisible)
	{
		return true;
	}

	if (Record.LastSeenTime >= 0.0 &&
		CurrentTime - Record.LastSeenTime <= SightMemoryDuration)
	{
		return true;
	}

	if (Record.LastStimulusTime >= 0.0 &&
		CurrentTime - Record.LastStimulusTime <= StimulusMemoryDuration)
	{
		return true;
	}

	return !Record.DamageSamples.IsEmpty();
}

bool ANSEnemyAIController::ShouldSwitchTarget(AActor* CandidateTarget, double CurrentTime) const
{
	AActor* CurrentTarget = CurrentCombatTarget.Get();

	if (!CandidateTarget || CandidateTarget == CurrentTarget)
	{
		return false;
	}

	if (!IsValidLivingTarget(CurrentTarget))
	{
		return true;
	}

	if (CurrentTime - LastTargetSwitchTime < TargetSwitchCooldown)
	{
		return false;
	}

	const bool bInitialLockFinished = bAttackStartedOnCurrentTarget ||
		CurrentTime - CurrentTargetSelectedTime >= InitialTargetLockDuration;

	if (!bInitialLockFinished)
	{
		return false;
	}

	const FNSTargetThreatRecord* CurrentRecord = ThreatRecords.Find(CurrentTarget);
	const FNSTargetThreatRecord* CandidateRecord = ThreatRecords.Find(CandidateTarget);

	if (!CandidateRecord)
	{
		return false;
	}

	if (!CurrentRecord)
	{
		return true;
	}

	const float CurrentDamage = GetRecentDamageThreat(*CurrentRecord, CurrentTime);
	const float CandidateDamage = GetRecentDamageThreat(*CandidateRecord, CurrentTime);

	if (CandidateDamage > 0.0f)
	{
		if (CurrentDamage <= 0.0f)
		{
			return true;
		}

		return CandidateDamage >= CurrentDamage * DamageThreatSwitchRatio;
	}

	if (CurrentDamage > 0.0f)
	{
		return false;
	}

	const APawn* EnemyPawn = GetPawn();
	if (!EnemyPawn)
	{
		return false;
	}

	const float CurrentDistance = FVector::Dist(EnemyPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
	const float CandidateDistance = FVector::Dist(EnemyPawn->GetActorLocation(), CandidateTarget->GetActorLocation());

	return CandidateDistance <= CurrentDistance * DistanceSwitchRatio;
}

void ANSEnemyAIController::SetCurrentCombatTarget(AActor* NewTarget)
{
	if (!IsValidLivingTarget(NewTarget) || CurrentCombatTarget == NewTarget)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	CurrentCombatTarget = NewTarget;

	const double CurrentTime = World->GetTimeSeconds();

	CurrentTargetSelectedTime = CurrentTime;
	LastTargetSwitchTime = CurrentTime;
	LastCombatProgressTime = CurrentTime;

	bAttackStartedOnCurrentTarget = false;

	UpdateCurrentTargetBlackboard();
}

void ANSEnemyAIController::ClearCurrentCombatTarget(bool bBlockReacquisition)
{
	AActor* PreviousTarget = CurrentCombatTarget.Get();

	if (bBlockReacquisition && PreviousTarget && GetWorld())
	{
		ReacquireBlockedUntil.FindOrAdd(PreviousTarget) = GetWorld()->GetTimeSeconds() + TargetReacquireCooldown;
	}

	CurrentCombatTarget.Reset();
	bAttackStartedOnCurrentTarget = false;

	if (CachedBBComp)
	{
		CachedBBComp->ClearValue(TargetActorKey);
		CachedBBComp->ClearValue(TargetLastKnownLocationKey);

		CachedBBComp->SetValueAsBool(HasTargetLineOfSightKey, false);
		CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
	}
}

void ANSEnemyAIController::ResetTargetingState()
{
	ThreatRecords.Reset();
	ReacquireBlockedUntil.Reset();
	CurrentCombatTarget.Reset();

	CurrentTargetSelectedTime = 0.0;
	LastTargetSwitchTime = 0.0;
	LastCombatProgressTime = 0.0;
	NextTargetEvaluationTime = 0.0;

	bAttackStartedOnCurrentTarget = false;

	if (CachedBBComp)
	{
		CachedBBComp->ClearValue(TargetActorKey);
		CachedBBComp->ClearValue(TargetLastKnownLocationKey);

		CachedBBComp->SetValueAsBool(HasTargetLineOfSightKey, false);
	}
}

void ANSEnemyAIController::UpdateCurrentTargetBlackboard()
{
	if (!CachedBBComp)
	{
		return;
	}

	AActor* TargetActor = CurrentCombatTarget.Get();

	if (!IsValidLivingTarget(TargetActor))
	{
		CachedBBComp->ClearValue(TargetActorKey);
		return;
	}

	CachedBBComp->SetValueAsObject(TargetActorKey, TargetActor);

	if (const FNSTargetThreatRecord* Record = ThreatRecords.Find(TargetActor))
	{
		CachedBBComp->SetValueAsVector(TargetLastKnownLocationKey, Record->LastKnownLocation);
		CachedBBComp->SetValueAsBool(HasTargetLineOfSightKey, Record->bCurrentlyVisible);
	}
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
}

bool ANSEnemyAIController::UsesMeleeAttackReservation() const
{
	const ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(GetPawn());

	const UNSEnemyData* EnemyData = Enemy ? Enemy->GetEnemyData() : nullptr;

	if (!EnemyData)
	{
		return false;
	}

	for (const FNSEnemyAttackDefinition& Attack : EnemyData->AttackList)
	{
		if (Attack.AttackType == ENSEnemyAttackType::MeleeSweep)
		{
			return true;
		}
	}

	return false;
}
