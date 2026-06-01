// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "NSBaseCompanionAI.generated.h"

class USphereComponent;
class USkeletalMeshComponent;
class UFloatingPawnMovement;
class ANSDroneAIController;


UCLASS()
class NEOSANCTUM_API ANSBaseCompanionAI : public APawn
{
	GENERATED_BODY()

public:
	ANSBaseCompanionAI();

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
	
	FVector ComputeAvoidanceVector() const;
	
protected:
	// @민재 : 컴포넌트 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> SphereComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UFloatingPawnMovement> FloatingPawnMovementComponent;
	
	//@민재 : 드론 고도 유지위한 변수
	UPROPERTY(EditAnywhere, Category="DroneAI|Altitude")
	float Altitude = 300.f;
	
	UPROPERTY(EditAnywhere, Category="DroneAI|Altitude")
	float GroundTraceDistance = 2000.f;
	
	UPROPERTY(EditAnywhere, Category = "DroneAI|Altitude")
	float AltitudeDeadZone = 20.f;			// 떨림 방지 데드존
 
	UPROPERTY(EditAnywhere, Category = "DroneAI|Altitude")
	float AltitudeCorrectionRange = 200.f;
	
	//@민재 : 커스텀 회피 변수
	UPROPERTY(EditAnywhere, Category="DroneAI|Avoidance")
	float AvoidanceTraceDistance  = 400.f;
	
	UPROPERTY(EditAnywhere, Category = "DroneAI|Avoidance")
	float AvoidanceTraceRadius = 80.f;
 
	UPROPERTY(EditAnywhere, Category = "DroneAI|Avoidance")
	float AvoidanceStrength = 1.0f;
	
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
	
	FORCEINLINE UAttributeSet* GetCompanionAttributeSet() const {return AttributeSet;}
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAttributeSet> AttributeSet;

};
