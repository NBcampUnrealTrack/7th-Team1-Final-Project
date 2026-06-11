// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "NSBaseCompanionAI.generated.h"

class UGameplayEffect;
class UNSCompanionAttributeSet;
class USphereComponent;
class USkeletalMeshComponent;
class UFloatingPawnMovement;
class ANSDroneAIController;


UCLASS()
class NEOSANCTUM_API ANSBaseCompanionAI : public APawn, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ANSBaseCompanionAI();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	USkeletalMeshComponent* GetSkeletalMeshComponent() const {return SkeletalMeshComponent;}
	
	// @민재 : 멀티 관련
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	
	// @민재 : 드론 움직임 함수 BT연동
	UFUNCTION(BlueprintCallable, Category = "Drone|Movement")
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
	float MinSpeedToRotate = 50.f;
	
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
	
	// @민재 : 도착거리 판정
	UPROPERTY(EditAnywhere, Category = "DroneAI|Avoidance")
	float ArrivalRadius = 60.f;
	
	// @민재 : 캐싱 데이터
public:
	void SetOwnerPlayer(AActor* Actor);
	AActor* GetOwnerPlayer() { return OwnerPlayer;}
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "AI|CachedData")
	TObjectPtr<ANSDroneAIController> CachedAIController;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OwnerPlayer")
	TObjectPtr<AActor> OwnerPlayer;
	
	// @민재 : GAS관련
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
};
