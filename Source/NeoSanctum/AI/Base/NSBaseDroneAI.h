// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "NeoSanctum/Data/Ability/NSCompanionAbilitySetTypes.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "NSBaseDroneAI.generated.h"

class UNSBaseDroneDefinition;
class UGameplayEffect;
class USphereComponent;
class USkeletalMeshComponent;
class UFloatingPawnMovement;
class UNSFlyingLocomotionComponent;
class AAIController;

UCLASS()
class NEOSANCTUM_API ANSBaseDroneAI : public APawn,
                                           public IAbilitySystemInterface,
                                           public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ANSBaseDroneAI();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

#pragma region DroneTeamId
public:
	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(static_cast<uint8>(TeamId));
	}

protected:
	ETeamId TeamId = ETeamId::Player;

#pragma endregion

public:
	USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }

	// @민재 : 멀티 관련
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	// @민재 : 컴포넌트 초기화 연결 지점 (Sphere/Mesh/FloatingPawnMovement/Locomotion 모두 유효)
	virtual void PostInitializeComponents() override;

	// @민재 : 드론 움직임 함수 BT 연동. 실제 로직은 LocomotionComponent에 위임
	UFUNCTION()
	void MoveTowards(
		const FVector& TargetLocation);                  // 목표 지점 (XY 평면)

protected:
	// @민재 : 데이터 기반 초기화 훅. 서브클래스에서 오버라이드하여 Definition 로드/적용
	virtual void InitializeFromData();

protected:
	// @민재 : 컴포넌트 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> SphereComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UFloatingPawnMovement> FloatingPawnMovementComponent;

	// @민재 : 비행 로코모션 (회전/고도/스티어링/회피 통합)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Locomotion")
	TObjectPtr<UNSFlyingLocomotionComponent> LocomotionComponent;

#pragma region CachedData
	// @민재 : 캐싱 데이터
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|CachedData")
	TObjectPtr<AAIController> CachedAIController;

public:
	// @민재 : 현재 적 지정. 로코모션 컴포넌트의 회전 타겟도 함께 동기화
	void SetCurrentEnemy(
		AActor* InEnemy);                                // 새 타겟 (nullptr이면 해제)

	AActor* GetCurrentEnemy() const { return CurrentEnemy.Get(); }

protected:
	UPROPERTY(VisibleAnywhere, Category="GAS|WeakPtr")
	TWeakObjectPtr<AActor> CurrentEnemy;

#pragma endregion

#pragma region CompanionGAS
public:
	FORCEINLINE UAbilitySystemComponent* GetDroneAbilitySystemComponent() const { return AbilitySystemComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(EditDefaultsOnly, Category="GAS|Init")
	TSubclassOf<UGameplayEffect> DefaultStatsEffect;

	UPROPERTY(EditDefaultsOnly, Category="GAS|Init")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

protected:
	void InitAbilityActorInfo();
	void InitializeDefaultStats();
	void GiveDefaultAbilities();

	bool bDefaultAbilitiesGranted = false;

	// @ 민재 : 어빌리티에서 Pawn이 지정하고있는 Enemy접근

#pragma endregion

#pragma region DataDriven
public:
	void SetPendingDefinition(const UNSBaseDroneDefinition* InDefinition);

	void ApplyDroneDefinition(const UNSBaseDroneDefinition* NewDefinition);

	void ApplyDroneVisual(const UNSBaseDroneDefinition* NewDefinition);

	UFUNCTION()
	void OnRep_CurrentDefinition();

protected:
	// @민재 : 스폰엑터 디퍼드에서 PS로부터 정보를 전달 받으면 될것같음
	// 현재 드론이 보유중인 어빌리티 정보
	UPROPERTY()
	FNSCompanionAbilitySet_GrantedHandles CurrentAbilityHandles;

	// 현재 드론의 타입 정보
	UPROPERTY(ReplicatedUsing = OnRep_CurrentDefinition)
	TObjectPtr<const UNSBaseDroneDefinition> CurrentDefinition;

#pragma endregion
};