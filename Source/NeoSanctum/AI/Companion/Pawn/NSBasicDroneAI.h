// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "NSBasicDroneAI.generated.h"

class USphereComponent;

UCLASS()
class NEOSANCTUM_API ANSBasicDroneAI : public ANSBaseCompanionAI
{
	GENERATED_BODY()

public:
	ANSBasicDroneAI();
	
	// AI Perceptin 시야 회전 동기화
	virtual void GetActorEyesViewPoint( FVector& Location, FRotator& Rotation ) const override;
	
	// SpawnOwner 소유자 지정
	void SetOwnerPlayer(AActor* Actor);
	AActor* GetOwnerPlayer() { return OwnerPlayer;};
	
protected:
	virtual void BeginPlay() override;

private:
	
	
	UPROPERTY(VisibleAnywhere)
	UBlackboardComponent* DroneAIBBComponent;
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OwnerPlayer")
	TObjectPtr<AActor> OwnerPlayer;
};
