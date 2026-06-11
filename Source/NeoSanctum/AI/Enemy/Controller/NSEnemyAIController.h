// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "Perception/AIPerceptionTypes.h" // FAIStimulus
#include "NSEnemyAIController.generated.h"

UCLASS()
class NEOSANCTUM_API ANSEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ANSEnemyAIController();
	
	virtual void Tick(float DeltaTime) override;

	/*
	 * AI Controller가 적을 필터링하기 위한 TeamId 조회
	 */
	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(static_cast<uint8>(ETeamId::Enemy));
	}

	// 타 진영을 보았을 때의 적대/우호 규칙 정의 수립
	virtual ETeamAttitude::Type GetTeamAttitudeTo(const AActor& Other) const;

	// 타겟과의 실시간 거리 계산하여 현재 거리에 맞는 GAS 태그 결정 및 반환
	FGameplayTag GetAttackAbilityTagByDistance();
	
	// Blackboard에 저장된 현재 공격 대상을 반환
	AActor* GetCurrentTargetActor() const;

protected:
	// 빙의 시점에 에디터에서 할당된 BT 가동
	virtual void OnPossess(APawn* InPawn) override;

	// 시각 센서가 갱신될 때 블랙보드로 데이터를 전달할 콜백 함수
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
private:
	// 대상이 체력 데이터를 갖고 있는지, 살아 있는 유효한 타겟인지 검증
	bool IsValidLivingTarget(const AActor* Target) const;

protected:
	// 시야/청각 설정 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

private:
	// 타겟 액터
	FName TargetActorKey = TEXT("TargetActor");

	UPROPERTY(Transient)
	TObjectPtr<UBlackboardComponent> CachedBBComp;
};
