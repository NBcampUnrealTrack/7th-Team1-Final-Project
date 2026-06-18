// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyAIController.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
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

	AActor* TargetActor = GetCurrentTargetActor();

	if (IsValidLivingTarget(TargetActor))
	{
		SetFocus(TargetActor, EAIFocusPriority::Gameplay);
	}
	else
	{
		ClearFocus(EAIFocusPriority::Gameplay);
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
	AActor* TargetActor = nullptr;
	const UNSEnemyData* EnemyData = nullptr;
	float Distance = 0.0f;
	bool bHasLineOfSight = false;

	if (!TryBuildAttackEvaluationContext(EnemyData, TargetActor, Distance, bHasLineOfSight))
	{
		return false;
	}

	for (const FNSEnemyAttackDefinition& AttackDefinition : EnemyData->AttackList)
	{
		if (CanUseAttackDefinition(AttackDefinition, TargetActor, Distance, bHasLineOfSight))
		{
			CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), true);
			return true;
		}
	}

	CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
	return false;
}

bool ANSEnemyAIController::TryBuildAttackEvaluationContext(
	const UNSEnemyData*& OutEnemyData,
	AActor*& OutTargetActor,
	float& OutDistance,
	bool& bOutHasLineOfSight)
{
	if (!CachedBBComp)
	{
		return false;
	}

	AActor* TargetActor = Cast<AActor>(CachedBBComp->GetValueAsObject(TargetActorKey));
	APawn* AIPawn = GetPawn();
	if (!AIPawn || !IsValidLivingTarget(TargetActor))
	{
		CachedBBComp->SetValueAsObject(TargetActorKey, nullptr);
		CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
		return false;
	}
	
	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(AIPawn);
	if (!Enemy)
	{
		CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
		return false;
	}

	const UNSEnemyData* EnemyData = Enemy->GetEnemyData();
	if (!EnemyData)
	{
		CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
		return false;
	}
	
	const FVector ToTarget = (TargetActor->GetActorLocation() - AIPawn->GetActorLocation()).GetSafeNormal2D();
	const FVector Forward = AIPawn->GetActorForwardVector().GetSafeNormal2D();

	const float FacingDot = FVector::DotProduct(Forward, ToTarget);
	const float RequiredDot = FMath::Cos(FMath::DegreesToRadians(AttackFacingAngleDegrees));
	const bool bFacingTarget = FacingDot >= RequiredDot;
	const bool bHasLineOfSight = LineOfSightTo(TargetActor);

	if (!bFacingTarget)
	{
		CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), false);
		return false;
	}
	// 몬스터와 플레이어 간의 실시간 직선 거리 계산
	const float Distance = FVector::Dist(AIPawn->GetActorLocation(), TargetActor->GetActorLocation());

	OutTargetActor = TargetActor;
	OutEnemyData = EnemyData;
	OutDistance = Distance;
	bOutHasLineOfSight = bHasLineOfSight;

	return true;
}

const FNSEnemyAttackDefinition* ANSEnemyAIController::GetAttackDefinitionByDistance()
{
	AActor* TargetActor = nullptr;
	const UNSEnemyData* EnemyData = nullptr;
	float Distance = 0.0f;
	bool bHasLineOfSight = false;

	if (!TryBuildAttackEvaluationContext(EnemyData, TargetActor, Distance, bHasLineOfSight))
	{
		return nullptr;
	}

	const FNSEnemyAttackDefinition* SelectedAttack =
		SelectAttackDefinition(EnemyData, TargetActor, Distance, bHasLineOfSight);
	CachedBBComp->SetValueAsBool(TEXT("bCanAttack"), SelectedAttack != nullptr);
	
	return SelectedAttack;
}

TSubclassOf<UGameplayAbility> ANSEnemyAIController::GetAttackAbilityClassByDistance()
{
	const FNSEnemyAttackDefinition* SelectedAttack = GetAttackDefinitionByDistance();

	if (!SelectedAttack)
	{
		return nullptr;
	}

	return SelectedAttack->AbilityClass;
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

	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(InPawn);
	if (!Enemy) return;

	UNSEnemyData* EnemyData = Enemy->GetEnemyData();
	if (!EnemyData) return;

	if (EnemyData->BehaviorTree)
	{
		RunBehaviorTree(EnemyData->BehaviorTree);

		CachedBBComp = GetBlackboardComponent();
		if (CachedBBComp)
		{
			CachedBBComp->SetValueAsFloat(TEXT("MinAttackRange"), EnemyData->MinAttackRange);
			CachedBBComp->SetValueAsFloat(TEXT("MaxAttackRange"), EnemyData->MaxAttackRange);
		}
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

const FNSEnemyAttackDefinition* ANSEnemyAIController::SelectAttackDefinition(
	const UNSEnemyData* EnemyData,
	const AActor* TargetActor,
	float Distance,
	bool bHasLineOfSight) const
{
	if (!EnemyData)
	{
		return nullptr;
	}

	TArray<const FNSEnemyAttackDefinition*> Candidates;
	int32 BestPriority = TNumericLimits<int32>::Lowest();

	for (const FNSEnemyAttackDefinition& AttackDefinition : EnemyData->AttackList)
	{
		if (!CanUseAttackDefinition(AttackDefinition, TargetActor, Distance, bHasLineOfSight))
		{
			continue;
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

	return true;
}
