// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Combat/Component/NSEnemyCombatComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyCoreComponent.h"
#include "NeoSanctum/Core/GameFlow/NSDifficultyType.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "NSEnemyCharacterBase.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnEnemyDead);

class UNSEnemyData;
class UGameplayAbility;
class UNSMonsterAttributeSet;
class ANSEnemyWeaponBase;
class UMaterialInstanceDynamic;
class UNSDamageFlashComponent;
class UNSHitReactionComponent;
class UNSEnemyPhaseComponent;
class UNSEnemyAttackComponent;
class UNSEnemyTargetComponent;
class UNSEnemyThreatComponent;
class UNSEnemyMeleeComponent;
class UNSEnemyMoveComponent;

UCLASS(Abstract)
class NEOSANCTUM_API ANSEnemyCharacterBase : public ACharacter, 
											 public IAbilitySystemInterface,
                                             public IGenericTeamAgentInterface,
											 public INSEnemyAgent
{
	GENERATED_BODY()

public:
	ANSEnemyCharacterBase();
	virtual void BeginPlay() override;
	
	/*
	 * EnemyCharacter가 타겟팅 혹은 피격되었을 때 진영을 알기 위한 TeamId 조회
	 */
	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(static_cast<uint8>(ETeamId::Enemy));
	}

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual UNSEnemyData* GetEnemyData() const override
	{
		return CoreComponent ? CoreComponent->GetEnemyData() : nullptr;
	}
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return ASC; }

	// Enemy 공통 Interface에서 사용하는 주 메시 반환 함수
	virtual USkeletalMeshComponent* GetEnemyMesh() const override { return GetMesh(); }
	
	// 현재 실행 중인 공격 Row를 저장하는 함수
	virtual void SetCurrentAttackRow(const FNSEnemyAttackRow& InAttackRow) override;

	// 현재 실행 중인 공격 Row를 반환하는 함수
	virtual const FNSEnemyAttackRow* GetCurrentAttackRow() const override;

	// 현재 실행 중인 공격 Row를 초기화하는 함수
	virtual void ClearCurrentAttackRow() override;
	
	// Enemy 공통 Interface에서 공격, 조준, Trace 기준으로 사용할 위치를 반환하는 함수
	virtual FVector GetAimLocation() const override;

	ANSEnemyWeaponBase* GetCurrentWeapon() const;

	FOnEnemyDead OnEnemyDead;

public:
	void Die();

	// (이용호 추가) 외부에서 생존 여부 확인용
	bool IsDead() const { return bIsDead; }
	bool IsInPool() const { return bIsInPool; }
	
	// 스폰 시 데이터 주입용 (BeginPlay 전 호출)
	void SetEnemyData(UNSEnemyData* InEnemyData);
	
	void PrepareForReuse(const FVector& SpawnLocation, const FRotator& SpawnRotation);
	void DeactivateForPool();
	
	void SetDifficultyScale(const FNSDifficultyScale& InScale)
	{
		if (CoreComponent)
		{
			CoreComponent->SetDifficultyScale(InScale);
		}
	}

protected:
	// Enemy의 현재 공격 상태를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyCombatComponent> CombatComponent;
	
	// Enemy의 Phase 상태를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyPhaseComponent> PhaseComponent;
	
	// EnemyData와 GAS 초기화를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyCoreComponent> CoreComponent;
	
	// Enemy의 공격 선택과 공격 쿨다운을 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyAttackComponent> AttackComponent;
	
	// Enemy의 공격 대상 판정과 엄폐물 Trace를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyTargetComponent> TargetComponent;
	
	// Enemy의 Threat 기록과 어그로 타겟 선택을 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyThreatComponent> ThreatComponent;
	
	// Enemy의 근접 예약 상태를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyMeleeComponent> MeleeComponent;
	
	// Enemy의 후퇴와 전투 회전 방향을 관리하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyMoveComponent> MoveComponent;
	
	// 디졸브 효과 컴포넌트
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	TObjectPtr<class UNSDissolveComponent> DissolveComponent;

	// 무기 장착 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UNSEnemyWeaponComponent> WeaponComponent;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY(Transient)
	TObjectPtr<UNSMonsterAttributeSet> AttributeSet;

protected:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_bIsDead, Category = "GAS|Death")
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_bIsDead();
	
	//(이용호 추가)
	void ApplyAliveVisual();
	void ApplyDeadVisual();
	void ApplyVisualData();
	void InitializeFromData(bool bFullInit);

private:
	//(이용호 추가)
	void OnDissolveFinished();
	UPROPERTY(ReplicatedUsing = OnRep_bIsInPool)
	bool bIsInPool = false;
	
	UFUNCTION()
	void OnRep_bIsInPool();
	
	// CoreComponent의 EnemyData가 변경됐을 때 시각 데이터를 갱신하는 함수
	void HandleEnemyDataChanged(UNSEnemyData* NewEnemyData);

public:
	void UpdateCombatAimTarget(AActor* TargetActor);
	void ClearCombatAimTarget();

	bool HasCombatAimTarget() const { return bHasCombatAimTarget; }
	FVector GetCombatAimTargetLocation() const { return CombatAimTargetLocation; }

protected:
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Combat|Aim")
	bool bHasCombatAimTarget = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Combat|Aim")
	FVector CombatAimTargetLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Aim")
	float AimTargetZOffsetRatio = 0.15f;
	
	//NavLink 점프 관련 - 이준로 추가
public:
	virtual void Landed(const FHitResult& Hit) override;

	// NavLink 점프 시작: 점프 플래그를 켜고 LaunchCharacter.
	UFUNCTION(BlueprintCallable)
	void StartNavLinkJump(const FVector& DestPoint);

private:
	// NavLink 점프로 인한 착지인지 확인 (일반 착지엔 NavMesh 보정 개입 X)
	bool bNavLinkJumping = false;
	
public:
	void SetRetreating(bool bInRetreating);
	bool IsRetreating() const { return bIsRetreating; }

protected:
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Combat|Movement")
	bool bIsRetreating = false;

#pragma region 피격 관리

public:
	// 피격 게이지가 최대치에 도달했음을 전달하는 델리게이트
	DECLARE_MULTICAST_DELEGATE(FOnHitGaugeThresholdReached);

	// 피격 게이지가 최대치에 도달했을 때 서버에서 발생하는 이벤트
	FOnHitGaugeThresholdReached OnHitGaugeThresholdReached;

	// 현재 피격 게이지를 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Combat|Hit Gauge")
	float GetHitGauge() const;

	// 최대 피격 게이지를 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Combat|Hit Gauge")
	float GetMaxHitGauge() const;

	// 피격 게이지를 0으로 초기화하는 함수
	UFUNCTION(BlueprintCallable, Category = "Combat|Hit Gauge")
	void ResetHitGauge();

	// AttributeSet이 게이지 최대치 도달을 캐릭터에 알려줄 때 사용하는 함수
	void NotifyHitGaugeThresholdReached();

	// 현재 몬스터가 피격 경직 행동을 수행 중인지 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Combat|Hit Reaction")
	virtual bool IsHitReacting() const override { return bIsHitReacting; }

	// 피격 경직 Ability 종료 후 이동과 Behavior Tree 행동을 복구하는 함수
	void FinishHitReaction();

protected:
	// 현재 피격 경직 여부를 서버에서 관리하고 클라이언트에 복제
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Combat|Hit Reaction")
	bool bIsHitReacting = false;

private:
	// 피격 게이지 임계 이벤트를 받아 경직 Ability 실행을 시도하는 함수
	void HandleHitGaugeThresholdReached();

	// 경직 상태를 변경하고 AI Controller에 시작 또는 종료를 알리는 함수
	void SetHitReactionState(bool bNewHitReacting);

#pragma endregion
	
#pragma region MID 적용

protected:
	// 몬스터의 외형 MID 또는 기존 Overlay에 피격 효과를 적용하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSDamageFlashComponent> DamageFlashComponent;

	// 실제 Health Damage를 받았을 때 월드 피격 리액션을 재생하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSHitReactionComponent> HitReactionComponent;
	
	// 현재 몬스터 외형에 적용된 런타임 MID 배열
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RuntimeVisualMaterials;

	// EnemyData를 바탕으로 외형 MID를 생성하고 피격 컴포넌트에 등록하는 함수
	void InitializeRuntimeMaterials();
#pragma endregion
};
