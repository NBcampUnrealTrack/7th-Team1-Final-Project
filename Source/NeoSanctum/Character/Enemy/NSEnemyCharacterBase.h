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
DECLARE_DELEGATE(FNSNavLinkTraversalFinishedDelegate);

// NavLink 특수 이동의 현재 진행 단계를 나타내는 열거형
UENUM(BlueprintType)
enum class ENSNavLinkTraversalPhase : uint8
{
	// NavLink 특수 이동을 수행하지 않는 상태
	None UMETA(DisplayName = "None"),

	// NavLink 목적지 방향으로 몸을 정렬하는 상태
	Rotating UMETA(DisplayName = "Rotating"),

	// LaunchCharacter 이후 착지를 기다리는 상태
	Jumping UMETA(DisplayName = "Jumping")
};

class UNSEnemyData;
class UGameplayAbility;
class UNSMonsterAttributeSet;
class UMaterialInstanceDynamic;
class UNSDamageFlashComponent;
class UNSHitReactionComponent;
class UNSEnemyPhaseComponent;
class UNSEnemyAttackComponent;
class UNSEnemyTargetComponent;
class UNSEnemyThreatComponent;
class UNSEnemyMeleeComponent;
class UNSEnemyMoveComponent;
class UNSEnemyStateComponent;
class UNSEnemyPartComponent;
class UNSMinimapIconComponent;

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
	virtual void Tick(float DeltaSeconds) override;

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

	// Enemy의 미니맵 아이콘 컴포넌트를 반환하는 함수
	UNSMinimapIconComponent* GetMinimapIconComponent() const { return MinimapIconComponent; }

	// 현재 실행 중인 공격 Row를 저장하는 함수
	virtual void SetCurrentAttackRow(const FNSEnemyAttackRow& InAttackRow) override;

	// 현재 실행 중인 공격 Row를 반환하는 함수
	virtual const FNSEnemyAttackRow* GetCurrentAttackRow() const override;

	// 현재 실행 중인 공격 Row를 초기화하는 함수
	virtual void ClearCurrentAttackRow() override;

	// Enemy 공통 Interface에서 공격, 조준, Trace 기준으로 사용할 위치를 반환하는 함수
	virtual FVector GetAimLocation() const override;

	FOnEnemyDead OnEnemyDead;

public:
	void Die();

	// (이용호 추가) 외부에서 생존 여부 확인용
	bool IsDead() const;
	bool IsInPool() const;

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

	// Enemy의 사망, 비활성, 피격 경직 상태를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyStateComponent> StateComponent;

	// Enemy의 장착형 무기와 파츠 스폰을 관리하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyPartComponent> PartComponent;

	// 디졸브 효과 컴포넌트
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	TObjectPtr<class UNSDissolveComponent> DissolveComponent;

	// Enemy의 미니맵 아이콘 표시를 처리하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSMinimapIconComponent> MinimapIconComponent;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY(Transient)
	TObjectPtr<UNSMonsterAttributeSet> AttributeSet;

protected:
	//(이용호 추가)
	void ApplyAliveVisual();
	void ApplyDeadVisual();
	bool StartDeathRagdoll();
	void ApplyVisualData();
	void InitializeFromData(bool bFullInit);

private:
	//(이용호 추가)
	void OnDissolveFinished();

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

#pragma region NavLink 점프 관련
	// NavLink 점프 관련 - 이준로, 최준혁 추가
public:
	virtual void Landed(const FHitResult& Hit) override;

	// NavLink 점프 시작을 회전-점프 트래버설 흐름으로 요청하는 함수
	UFUNCTION(BlueprintCallable)
	void StartNavLinkJump(const FVector& DestPoint);

	// 자동 생성 NavLink 도달 시 회전, 점프, 착지 완료까지 관리하는 함수
	bool StartNavLinkTraversal(
		const FVector& DestPoint,
		FNSNavLinkTraversalFinishedDelegate OnTraversalFinished);

	// 현재 NavLink 특수 이동을 수행 중인지 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Movement|NavLink")
	bool IsTraversingNavLink() const { return bIsTraversingNavLink; }

	// 현재 NavLink 특수 이동의 NavMesh 도착 위치를 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Movement|NavLink")
	FVector GetNavLinkDestination() const { return NavLinkDestination; }

	// 현재 NavLink 특수 이동 단계를 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Movement|NavLink")
	ENSNavLinkTraversalPhase GetNavLinkTraversalPhase() const { return NavLinkTraversalPhase; }

protected:
	// 현재 NavLink 특수 이동을 수행 중인지 나타내는 변수
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Movement|NavLink")
	bool bIsTraversingNavLink = false;

	// 현재 NavLink 특수 이동의 NavMesh 도착 위치를 나타내는 변수
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Movement|NavLink")
	FVector NavLinkDestination = FVector::ZeroVector;

	// 현재 NavLink 특수 이동 단계를 나타내는 변수
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Movement|NavLink")
	ENSNavLinkTraversalPhase NavLinkTraversalPhase = ENSNavLinkTraversalPhase::None;

	// NavLink 점프 전 목적지 방향으로 회전하는 초당 각속도 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|NavLink", meta = (ClampMin = "1.0"))
	float NavLinkRotationSpeed = 720.0f;

	// NavLink 점프 전 회전 완료로 인정할 Yaw 오차 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|NavLink",
		meta = (ClampMin = "0.1", ClampMax = "45.0"))
	float NavLinkRotationAcceptableYaw = 5.0f;

	// NavLink 점프 전 회전을 기다릴 최대 시간 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|NavLink", meta = (ClampMin = "0.05"))
	float NavLinkRotationTimeout = 0.7f;

private:
	// NavLink 점프로 인한 착지인지 확인하는 변수
	bool bNavLinkJumping = false;

	// NavLink 회전 시작 전 bOrientRotationToMovement 값을 저장하는 변수
	bool bCachedNavLinkOrientRotationToMovement = true;

	// NavLink 회전 시작 전 bUseControllerDesiredRotation 값을 저장하는 변수
	bool bCachedNavLinkUseControllerDesiredRotation = false;

	// NavLink 회전 대기 시간을 누적하는 변수
	float NavLinkRotationElapsed = 0.0f;

	// NavLink 점프 전에 도달해야 하는 목표 회전값을 저장하는 변수
	FRotator NavLinkTargetRotation = FRotator::ZeroRotator;

	// NavLink 특수 이동의 캡슐 중심 도착 위치를 나타내는 변수
	FVector NavLinkActorDestination = FVector::ZeroVector;

	// NavLink 특수 이동 종료 시 PathFollowing 복구를 호출하는 델리게이트 변수
	FNSNavLinkTraversalFinishedDelegate NavLinkTraversalFinishedDelegate;

	// NavLink 특수 이동 회전을 매 프레임 갱신하는 함수
	void UpdateNavLinkTraversal(float DeltaSeconds);

	// NavLink 회전 완료 후 실제 점프를 시작하는 함수
	void StartNavLinkJumpAfterRotation();

	// 저장된 NavLink 캡슐 중심 도착 위치로 LaunchCharacter를 실행하는 함수
	bool ExecuteNavLinkJump();

	// NavLink 착지 후 도착점 기준으로 위치와 이동 상태를 정리하는 함수
	void FinalizeNavLinkLanding();

	// NavLink 특수 이동 상태를 정리하고 필요하면 PathFollowing을 복구하는 함수
	void FinishNavLinkTraversal(bool bNotifyPathFollowing);

	// NavLink 특수 이동 상태를 Blackboard에 반영하는 함수
	void RefreshNavLinkTraversalBlackboard();
#pragma endregion

public:
	void SetRetreating(bool bInRetreating);
	bool IsRetreating() const { return bIsRetreating; }

protected:
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Combat|Movement")
	bool bIsRetreating = false;


#pragma region 피격 관리

public:
	// 현재 피격 게이지를 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Combat|Hit Gauge")
	float GetHitGauge() const;

	// 최대 피격 게이지를 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Combat|Hit Gauge")
	float GetMaxHitGauge() const;

	// 피격 게이지를 0으로 초기화하는 함수
	UFUNCTION(BlueprintCallable, Category = "Combat|Hit Gauge")
	void ResetHitGauge();

	// 현재 몬스터가 피격 경직 행동을 수행 중인지 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Combat|Hit Reaction")
	virtual bool IsHitReacting() const override;

	// 피격 경직 Ability 종료 후 이동과 Behavior Tree 행동을 복구하는 함수
	void FinishHitReaction();

private:
	// StateComponent에서 사망 처리가 시작됐을 때 서버 전용 후처리를 수행하는 함수
	void HandleDeathStarted();

	// StateComponent의 사망 상태 변경을 받아 시각 상태를 갱신하는 함수
	void HandleDeadStateChanged(bool bDead);

	// StateComponent의 비활성 상태 변경을 받아 풀링 시각 상태를 갱신하는 함수
	void HandleInactiveStateChanged(bool bInactive);

	// StateComponent의 피격 경직 상태 변경을 받아 AI 상태를 갱신하는 함수
	void HandleHitReactionStateChanged(bool bHitReacting);

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
