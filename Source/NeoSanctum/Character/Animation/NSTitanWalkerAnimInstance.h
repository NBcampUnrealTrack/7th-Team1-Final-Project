// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSBossAnimInstance.h"
#include "NSTitanWalkerAnimInstance.generated.h"

class UNSEnemyThreatComponent;

/**
 * Stage1 TitanWalker 전용 AnimInstance입니다.
 * 지상형 보스의 이동, 몸체 회전, 상체 조준 값을 AnimBP와 Control Rig에 제공합니다.
 */
UCLASS()
class NEOSANCTUM_API UNSTitanWalkerAnimInstance : public UNSBossAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// TitanWalker 전용 컴포넌트와 런타임 값을 캐싱하는 함수
	void CacheTitanWalker();

	// Pawn Velocity를 기준으로 이동 애니메이션 값을 갱신하는 함수
	void UpdateLocomotion();

	// Actor Yaw 변화량을 기준으로 회전 애니메이션 값을 갱신하는 함수
	void UpdateTurn(float DeltaSeconds);

	// 현재 공격 Row와 현재 타깃을 기준으로 상체 조준 값을 갱신하는 함수
	void UpdateUpperAim(float DeltaSeconds);

	// 타깃 Actor의 Bounds 기준 조준 위치를 반환하는 함수
	FVector GetTargetAimLocation(const AActor* TargetActor) const;

protected:
	// TitanWalker의 수평 이동 속도
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Locomotion")
	float MoveSpeed = 0.0f;

	// TitanWalker가 이동 중인지 AnimBP에서 판단하기 위한 값
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Locomotion")
	bool bIsMoving = false;

	// TitanWalker 몸체의 Yaw 회전 속도입니다. 단위는 degree/sec
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Locomotion")
	float TurnYaw = 0.0f;

	// TitanWalker 상체가 타깃을 향해 좌우로 조준해야 하는 각도
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Aim")
	float UpperAimYaw = 0.0f;

	// TitanWalker 상체가 타깃을 향해 상하로 조준해야 하는 각도
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Aim")
	float UpperAimPitch = 0.0f;

	// 이 속도보다 빠르면 이동 중으로 판단
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float MovingSpeedThreshold = 3.0f;

	// TurnYaw 보간 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float TurnInterpSpeed = 8.0f;

	// 상체 조준값 보간 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float UpperAimInterpSpeed = 10.0f;

	// AttackRow에 YawLimit이 없을 때 사용할 기본 상체 좌우 제한 각도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float DefaultUpperYawLimit = 60.0f;

	// AttackRow에 PitchLimit이 없을 때 사용할 기본 상체 상하 제한 각도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float DefaultUpperPitchLimit = 35.0f;

	// 타깃 Bounds에서 조준 위치를 위로 보정하는 비율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float AimZRatio = 0.15f;

private:
	// 현재 전투 타깃을 읽기 위한 컴포넌트
	UPROPERTY()
	TObjectPtr<UNSEnemyThreatComponent> ThreatComponent;

	// TurnYaw 계산을 위해 이전 프레임의 Actor Yaw
	float LastActorYaw = 0.0f;

	// LastActorYaw가 초기화되었는지 확인하는 값
	bool bHasLastActorYaw = false;
};
