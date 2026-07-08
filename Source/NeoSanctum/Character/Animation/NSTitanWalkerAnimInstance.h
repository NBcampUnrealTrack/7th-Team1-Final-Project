// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSBossAnimInstance.h"
#include "NSTitanWalkerAnimInstance.generated.h"

class UNSEnemyThreatComponent;
class UNSEnemyPartComponent;

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
	
	// 현재 조준값이 목표 조준값에 충분히 가까운지 확인하는 함수
	UFUNCTION(BlueprintPure, Category = "TitanWalker|Aim")
	bool IsAimAligned(float ToleranceDegrees = 5.0f) const;
	
	// 몽타주 진입 전 Control Rig 조준 상태를 즉시 초기화하는 함수
	virtual void ResetCombatAimImmediate() override;

protected:
	// TitanWalker 전용 컴포넌트와 런타임 값을 캐싱하는 함수
	void CacheTitanWalker();

	// Pawn Velocity를 기준으로 이동 애니메이션 값을 갱신하는 함수
	void UpdateLocomotion();

	// Actor Yaw 변화량을 기준으로 회전 애니메이션 값을 갱신하는 함수
	void UpdateTurn(float DeltaSeconds);

	// 현재 공격 Row와 현재 타깃을 기준으로 상체/무기 조준 값을 갱신하는 함수
	void UpdateAim(float DeltaSeconds);
	
	// Control Rig 적용 여부와 Alpha 값을 갱신하는 함수
	void UpdateControlRigBlend(float DeltaSeconds);
	
	// 전투 중 Idle 애니메이션으로 돌아가지 않도록 보정하는 함수
	void UpdateCombatPoseHold();

	// 타깃 Actor의 Bounds 기준 조준 위치를 반환하는 함수
	FVector GetTargetAimLocation(const AActor* TargetActor) const;
	
	// TitanWalker의 상체/무기 조준 값과 Control Rig Alpha를 즉시 초기화하는 함수
	void ResetAimImmediate();

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
	
	// TitanWalker 무기 파츠가 타깃을 향해 좌우로 조준해야 하는 각도
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Aim")
	float WeaponAimYaw = 0.0f;

	// TitanWalker 무기 파츠가 타깃을 향해 상하로 조준해야 하는 각도
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Aim")
	float WeaponAimPitch = 0.0f;
	
	// Control Rig 상체 회전 Weight에 연결할 보간 값
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Aim")
	float UpperAimAlpha = 0.0f;

	// Control Rig 무기 회전 Weight에 연결할 보간 값
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Aim")
	float WeaponAimAlpha = 0.0f;

	// 현재 상체 조준을 Control Rig에 적용할지 판단하는 값
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Aim")
	bool bUseUpperAim = false;

	// 현재 무기 조준을 Control Rig에 적용할지 판단하는 값
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Aim")
	bool bUseWeaponAim = false;

	// 현재 실행 중이거나 무기 조준 Fade-Out에 필요한 공격 ID
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Aim")
	FName CurrentAttackId = NAME_None;
	
	// 현재 공격이 Control Rig 조준 포즈를 필요로 하는지 나타내는 값
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Aim")
	bool bUseControlRigAttack = false;
	
	// 정지 공격 중 Idle 대신 공격 기준 포즈를 사용할지 결정하는 값
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Aim")
	bool bUseAttackBasePose = false;

	// AnimGraph의 Blend Poses by Bool에서 Control Rig Pose 경로를 사용할지 결정하는 값
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Aim")
	bool bShouldUseControlRigPose = false;

	// Control Rig 노드의 Alpha에 연결할 최종 보간 값
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Aim")
	float ControlRigAlpha = 0.0f;

	// 이 속도보다 빠르면 이동 중으로 판단
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float MovingSpeedThreshold = 3.0f;

	// TurnYaw 보간 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float TurnInterpSpeed = 8.0f;

	// 상체 조준값 보간 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float UpperAimInterpSpeed = 10.0f;
	
	// 무기 조준값 보간 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float WeaponAimInterpSpeed = 12.0f;

	// 타깃 Bounds에서 조준 위치를 위로 보정하는 비율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float AimZRatio = 0.15f;
	
	// 공격 중이 아니어도 타깃 감지 시 상체 조준을 사용할지 결정하는 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config")
	bool bTrackUpperToTarget = true;

	// 타깃 추적 상체 좌우 제한 각도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float TrackYawLimit = 180.0f;

	// 타깃 추적 상체 상하 제한 각도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float TrackPitchLimit = 45.0f;

	// 타깃 추적 상체 조준 보간 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float TrackAimSpeed = 10.0f;
	
	// Control Rig Alpha가 0과 1 사이로 보간되는 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float ControlRigInterpSpeed = 10.0f;

	// Control Rig Pose 경로를 끄기 위한 Alpha 최소 기준값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float ControlRigPoseDisableThreshold = 0.01f;
	
	// 타깃을 잃은 뒤에도 전투 기준 포즈를 유지할 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float CombatPoseHoldTime = 10.0f;
	
	// 최근 타깃을 감지했거나 전투 상태였던 시간을 누적 추적하는 값
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Combat")
	float LastTargetSeenTime = -1000.0f;

	// 현재 Idle 대신 CombatBasePose를 사용할지 결정하는 값
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|Combat")
	bool bUseCombatBasePose = false;
	
	// Part Row에 UpperBody YawLimit이 없을 때 사용할 기본 상체 좌우 제한 각도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float DefaultUpperYawLimit = 180.0f;

	// Part Row에 UpperBody PitchLimit이 없을 때 사용할 기본 상체 상하 제한 각도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float DefaultUpperPitchLimit = 35.0f;

	// Part Row에 Weapon YawLimit이 없을 때 사용할 기본 무기 좌우 제한 각도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float DefaultWeaponYawLimit = 180.0f;

	// Part Row에 Weapon PitchLimit이 없을 때 사용할 기본 무기 상하 제한 각도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TitanWalker|Config", meta = (ClampMin = "0.0"))
	float DefaultWeaponPitchLimit = 45.0f;
	
	// 계산된 원본 조준 Yaw
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|AimDebug")
	float RawAimYaw = 0.0f;

	// 계산된 원본 조준 Pitch
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|AimDebug")
	float RawAimPitch = 0.0f;

	// Body에 적용하려는 목표 Yaw
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|AimDebug")
	float TargetUpperYaw = 0.0f;
	
	// Body에 적용하려는 목표 Pitch
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|AimDebug")
	float TargetUpperPitch = 0.0f;

	// Weapon에 적용하려는 목표 Yaw
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|AimDebug")
	float TargetWeaponYaw = 0.0f;
	
	// Weapon에 적용하려는 목표 Pitch
	UPROPERTY(BlueprintReadOnly, Category = "TitanWalker|AimDebug")
	float TargetWeaponPitch = 0.0f;

private:
	// 현재 전투 타깃을 읽기 위한 컴포넌트
	UPROPERTY()
	TObjectPtr<UNSEnemyThreatComponent> ThreatComponent;
	
	// 현재 공격의 상체 조준 제한값을 읽기 위한 파츠 컴포넌트
	UPROPERTY()
	TObjectPtr<UNSEnemyPartComponent> PartComponent;

	// TurnYaw 계산을 위해 이전 프레임의 Actor Yaw
	float LastActorYaw = 0.0f;

	// LastActorYaw가 초기화되었는지 확인하는 값
	bool bHasLastActorYaw = false;
	
	// 현재 프레임에 실제 공격 Row가 존재하는지 확인하는 값
	bool bHasCurrentAttackRow = false;
};
