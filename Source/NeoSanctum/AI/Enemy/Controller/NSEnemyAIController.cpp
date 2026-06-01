// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyAIController.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "Perception/AIPerceptionComponent.h"


ANSEnemyAIController::ANSEnemyAIController()
{
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));

	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ANSEnemyAIController::OnTargetPerceptionUpdated);
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

FGameplayTag ANSEnemyAIController::GetAttackAbilityTagByDistance()
{
	if (!CachedBBComp) return FGameplayTag();

	AActor* TargetActor = Cast<AActor>(CachedBBComp->GetValueAsObject(TargetActorKey));
	APawn* AIPawn = GetPawn();
	if (!AIPawn || !TargetActor) return FGameplayTag();

	if (!IsValidLivingTarget(TargetActor))
	{
		CachedBBComp->SetValueAsObject(TargetActorKey, nullptr);
		return FGameplayTag();
	}

	// 몬스터와 플레이어 간의 실시간 직선 거리 계산
	float Distance = FVector::Dist(AIPawn->GetActorLocation(), TargetActor->GetActorLocation());

	// 공격 사거리 이내인 경우 공격 태그 반환
	if (ANSEnemyCharacterBase* EnemyChar = Cast<ANSEnemyCharacterBase>(AIPawn))
	{
		if (UNSEnemyData* EnemyData = EnemyChar->GetEnemyData())
		{
			if (Distance <= EnemyData->MaxAttackRange)
			{
				return AttackAbilityTag;
			}
		}
	}

	return FGameplayTag();
}

void ANSEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ANSEnemyCharacterBase* EnemyChar = Cast<ANSEnemyCharacterBase>(InPawn);
	if (!EnemyChar) return;

	UNSEnemyData* EnemyData = EnemyChar->GetEnemyData();
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
	if (!CachedBBComp) return;

	// 감지된 대상이 플레이어인지 재검증
	if (GetTeamAttitudeTo(*Actor) == ETeamAttitude::Type::Hostile && IsValidLivingTarget(Actor))
	{
		// 시야에 적이 들어왔으면 주소 저장, 시야에서 완전히 놓쳤으면 nullptr 처리
		AActor* Target = Stimulus.WasSuccessfullySensed() ? Actor : nullptr;

		// 블랙보드 TargetActor 키에 실시간 업데이트
		CachedBBComp->SetValueAsObject(TargetActorKey, Target);
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
