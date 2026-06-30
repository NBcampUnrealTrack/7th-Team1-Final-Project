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

class UNSCompanionDefinition;
class UGameplayEffect;
class UNSCompanionAttributeSet;
class USphereComponent;
class USkeletalMeshComponent;
class UFloatingPawnMovement;
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
	
	USkeletalMeshComponent* GetSkeletalMeshComponent() const {return SkeletalMeshComponent;}
	
	// @민재 : 멀티 관련
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	
	// @민재 : 드론 움직임 함수 BT연동
	UFUNCTION()
	void MoveTowards(const FVector& TargetLocation);
	
protected:
	// @민재 : 고도유지 이동관련 함수
	void MaintainAltitude(float DeltaSeconds);
	
	// @민재 : 회전기능 함수
	void DroneAIRotate(float DeltaSeconds);
	
	// @민재 : 회피기능 함수
	void InitSteeringDirections();
	
	void BuildInterestMap(const FVector& DesiredDirection);
	
	void BuildDangerMap();
	
	bool IsWalkableSurface(const FVector& SurfaceNormal) const;
	
	FVector ChooseSteeringDirection() const;
	
	// @민재 : 지형추적 기능
	bool TraceGroundAt(const FVector& WorldXY, float& OutZ) const; 
	
	bool SampleHighestGround(float& OutGroundZ) const;
	
protected:
	// @민재 : 컴포넌트 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> SphereComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UFloatingPawnMovement> FloatingPawnMovementComponent;
	
	// @민재 : 드론 회전관련 변수
	UPROPERTY(EditAnywhere, Category="DroneAI|Rotation")
	float YawInterpSpeed = 5.f;
	
	UPROPERTY(EditAnywhere, Category="DroneAI|Rotation")
	float CombatYawInterpSpeed = 10.f;
	
	UPROPERTY(EditAnywhere, Category="DroneAI|Rotation")
	float MinSpeedToRotate = 50.f;
	
#pragma region Altitude
	
	// @민재 : 드론 고도 유지위한 변수
	UPROPERTY(EditAnywhere, Category="DroneAI|Altitude")
	float Altitude = 300.f;
	
	UPROPERTY(EditAnywhere, Category="DroneAI|Altitude")
	float GroundTraceDistance = 2000.f;
	
	UPROPERTY(EditAnywhere, Category = "DroneAI|Altitude")
	float AltitudeDeadZone = 20.f;
 
	UPROPERTY(EditAnywhere, Category = "DroneAI|Altitude")
	float AltitudeCorrectionRange = 200.f;
	
	// @민재 : 지형추적 기능
	UPROPERTY(EditAnywhere, Category = "DroneAI|Altitude")
	float GroundSampleRadius = 150.f;
	
	UPROPERTY(EditAnywhere, Category = "DroneAI|Altitude")
	int32 GroundSampleCount = 4;
	
	UPROPERTY(EditAnywhere, Category = "DroneAI|Altitude")
	float GroundLookAheadDistance = 200.f;
	
	UPROPERTY(EditAnywhere, Category = "DroneAI|Altitude")
	float MaxClimbSpeed = 600.f;
	
	UPROPERTY(EditAnywhere, Category = "DroneAI|Altitude")
	float MaxDescendSpeed = 150.f;
	
	float SmoothedTargetHeight = 0.f;
	bool bHasValidGround = false;
	
#pragma endregion 
	
#pragma region CustomAvoid
	// @민재 : 커스텀 회피 기능 관련
	UPROPERTY(EditAnywhere, Category="DroneAI|Avoidance")
	float AvoidanceTraceDistance = 400.f;
	
	UPROPERTY(EditAnywhere, Category="DroneAI|Avoidance")
	float AvoidanceTraceRadius = 100.f;
	
	UPROPERTY(EditAnywhere, Category="DroneAI|Avoidance")
	int32 NumSteeringDirections = 16;
	
	UPROPERTY(EditAnywhere, Category = "DroneAI|Avoidance")
	float DangerThreshold = 0.1f;
	
	UPROPERTY(EditAnywhere, Category = "DroneAI|Avoidance")
	float MaxWalkableSlopeAngle = 50.f;
 
	TArray<FVector> SteeringDirections;
	
	TArray<float> InterestMap;
	
	TArray<float> DangerMap;
	
#pragma endregion 
	
	// @민재 : 도착거리 판정
	UPROPERTY(EditAnywhere, Category = "DroneAI|Movement")
	float ArrivalRadius = 60.f;
	
#pragma region CachedData
	// @민재 : 캐싱 데이터
public:
	void SetPendingDefinition(const UNSCompanionDefinition* InDefinition);
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "AI|CachedData")
	TObjectPtr<AAIController> CachedAIController;
	
public:
	void SetCurrentEnemy(AActor* InEnemy) {CurrentEnemy = InEnemy;}
	AActor* GetCurrentEnemy() const {return CurrentEnemy.Get();}
protected:
	UPROPERTY(VisibleAnywhere, Category="GAS|WeakPtr")
	TWeakObjectPtr<AActor> CurrentEnemy;
	
#pragma endregion
	
#pragma region CompanionGAS
	
public:
	FORCEINLINE UAbilitySystemComponent* GetCompanionAbilitySystemComponent() const {return AbilitySystemComponent;}
	
	FORCEINLINE UNSCompanionAttributeSet* GetCompanionAttributeSet() const {return CompanionAttributeSet;}
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UNSCompanionAttributeSet> CompanionAttributeSet;

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
	void ApplyDroneDefinition(const UNSCompanionDefinition* NewDefinition);
	
	void ApplyCompanionVisual(const UNSCompanionDefinition* NewDefinition);
	
	UFUNCTION()
	void OnRep_CurrentDefinition();
	
protected:
	// @민재 : 스폰엑터 디퍼드에서 PS로부터 정보를 전달 받으면 될것같음
	// 현재 드론이 보유중인 어빌리티 정보
	UPROPERTY() 
	FNSCompanionAbilitySet_GrantedHandles CurrentAbilityHandles;
	
	// 현재 드론의 타입 정보
	UPROPERTY(ReplicatedUsing = OnRep_CurrentDefinition)
	TObjectPtr<const UNSCompanionDefinition> CurrentDefinition;
	
#pragma endregion
	
};
