// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "AIController.h"
#include "NSDroneAI.generated.h"

UCLASS()
class NEOSANCTUM_API ANSDroneAI : public ANSBaseCompanionAI
{
	GENERATED_BODY()

public:
	ANSDroneAI();
	
	UFUNCTION()
	void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);
	
	UFUNCTION()
	void MoveToTargetOwningCharacter();
	
	UFUNCTION()
	void FindTargetOwninigCharacter();
	
	UFUNCTION()
	void StartMoving();
	
protected:
	virtual void BeginPlay() override;
	
private:
	// 소유한 플레이어
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone", meta=(AllowPrivateAccess=true))
	TObjectPtr<ACharacter> TargetOwningCharacter;
	
	// 추적할 엑터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone", meta=(AllowPrivateAccess=true))
	TObjectPtr<AActor> TargetActor;
	
	// 도착 거리 반경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone", meta=(AllowPrivateAccess=true))
	float TargetRange;
	
	UPROPERTY()
	AAIController* AIController;
	
	UPROPERTY()
	bool bIsMoving;
};
