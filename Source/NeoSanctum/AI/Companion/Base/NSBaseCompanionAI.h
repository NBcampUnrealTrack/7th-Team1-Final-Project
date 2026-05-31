// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "NSBaseCompanionAI.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UFloatingPawnMovement;
class ANSDroneAIController;


UCLASS()
class NEOSANCTUM_API ANSBaseCompanionAI : public APawn
{
	GENERATED_BODY()

public:
	ANSBaseCompanionAI();

	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	
protected:
	// @민재 : 컴포넌트 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCapsuleComponent> CapsuleComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UFloatingPawnMovement> FloatingPawnMovementComponent;
	
	
	// @민재 : 캐싱 데이터
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "AI|CachedData")
	TObjectPtr<ANSDroneAIController> CachedAIController;
	
	// @민재 : GAS관련 로직
public:
	FORCEINLINE UAbilitySystemComponent* GetCompanionAbilitySystemComponent() const {return AbilitySystemComponent;}
	
	FORCEINLINE UAttributeSet* GetCompanionAttributeSet() const {return AttributeSet;}
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAttributeSet> AttributeSet;

};
