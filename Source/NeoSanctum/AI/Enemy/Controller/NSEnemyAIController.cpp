// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyAIController.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
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
	if (!CachedBBComp)
	{
		return nullptr;
	}
	
	return Cast<AActor>(CachedBBComp->GetValueAsObject(TargetActorKey));
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
	}
}

void ANSEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!CachedBBComp || !Actor) return;

	const FAISenseID SightID = UAISense::GetSenseID<UAISense_Sight>();
	const FAISenseID HearingID = UAISense::GetSenseID<UAISense_Hearing>();
	const FAISenseID DamageID = UAISense::GetSenseID<UAISense_Damage>();

	// 감지된 대상이 플레이어인지 재검증
	if (GetTeamAttitudeTo(*Actor) == ETeamAttitude::Type::Hostile && IsValidLivingTarget(Actor))
	{
		// 시각
		if (Stimulus.Type == SightID)
		{
			// 시야에 적이 들어왔으면 주소 저장, 시야에서 완전히 놓쳤으면 nullptr 처리
			AActor* Target = Stimulus.WasSuccessfullySensed() ? Actor : nullptr;

			// 블랙보드 TargetActor 키에 실시간 업데이트
			CachedBBComp->SetValueAsObject(TargetActorKey, Target);
		}

		// 청각 / 데미지
		else if (Stimulus.Type == HearingID || Stimulus.Type == DamageID)
		{
			if (Stimulus.WasSuccessfullySensed())
			{
				CachedBBComp->SetValueAsObject(TargetActorKey, Actor);
			}
		}
	}
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
