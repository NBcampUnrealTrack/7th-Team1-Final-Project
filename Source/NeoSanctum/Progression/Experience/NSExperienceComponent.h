// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSExperienceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNSExpChanged, float, CurrentExp, float, MaxExp);

/**
 * 몬스터 처치 경험치를 누적하고, 통이 가득 찰 때마다 레벨업 보상 트리거를 발생시키는 컴포넌트.
 * 실제 레벨 개념은 없으며 경험치 통과 레벨업 횟수만 관리.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSExperienceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSExperienceComponent();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * 기본 지급량에 이 플레이어의 획득 배율을 적용해 누적하고,
	 * Max를 넘긴 횟수(레벨업 횟수)를 반환. 서버 권한에서만 동작.
	 */
	int32 AddExperience(float BaseAmount);

	float GetCurrentExp() const { return CurrentExp; }

	UPROPERTY(BlueprintAssignable, Category = "NS|Experience")
	FOnNSExpChanged OnExpChanged;

private:
	// OutRun 계정 영구 업그레이드의 경험치 획득 배율.
	// TODO(원종): 공통 업그레이드 시스템 브랜치에서 ProgressComponent 저장값과 연결. 현재는 항상 1.0
	float GetExpGainMultiplier() const;

	UFUNCTION()
	void OnRep_CurrentExp(float OldCurrentExp);

	UPROPERTY(ReplicatedUsing = OnRep_CurrentExp)
	float CurrentExp = 0.0f;
};
