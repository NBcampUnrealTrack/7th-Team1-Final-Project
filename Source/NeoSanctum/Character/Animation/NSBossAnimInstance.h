// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NSBossAnimInstance.generated.h"

class APawn;
class UNSEnemyStateComponent;

/**
 * Boss Pawn 계열 AnimInstance의 공통 부모 클래스입니다.
 * CharacterMovement나 특정 보스 이동 방식에 의존하지 않고, 공통 상태만 관리합니다.
 */
UCLASS()
class NEOSANCTUM_API UNSBossAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	// 몽타주 진입 전 보스별 조준 상태를 즉시 초기화하기 위한 함수
	virtual void ResetCombatAimImmediate();

protected:
	// AnimInstance를 소유한 Boss Pawn을 캐싱하는 함수
	void CacheOwner();

	// StateComponent에서 애니메이션 상태 플래그를 갱신하는 함수
	void UpdateState();

protected:
	// 이 AnimInstance를 소유한 Pawn
	UPROPERTY(BlueprintReadOnly, Category = "Boss|Owner")
	TObjectPtr<APawn> OwnerPawn;

	// Character/Pawn을 공통 Enemy로 다루기 위한 인터페이스 참조
	UPROPERTY(BlueprintReadOnly, Category = "Boss|Owner")
	TScriptInterface<INSEnemyAgent> EnemyAgent;

	// 사망, 피격 경직 같은 공통 Enemy 상태를 읽기 위한 컴포넌트
	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	TObjectPtr<UNSEnemyStateComponent> StateComponent;

	// Boss가 사망 상태인지 애니메이션 그래프에서 사용하기 위한 값
	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	bool bIsDead = false;

	// Boss가 피격 경직 상태인지 애니메이션 그래프에서 사용하기 위한 값
	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	bool bIsHitReacting = false;
};
