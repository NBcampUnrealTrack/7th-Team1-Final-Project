// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "NSBaseCompanionAI.generated.h"

class UNSCompanionAttributeSet;
class UNSCompanionAbilitySystemComponent;
class UFloatingPawnMovement;
class USceneComponent;

UCLASS()
class NEOSANCTUM_API ANSBaseCompanionAI : public APawn
{
	GENERATED_BODY()

public:
	ANSBaseCompanionAI();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UFloatingPawnMovement> MovementComp;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> RootScene;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComponent;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystemComponent")
	TObjectPtr<UNSCompanionAbilitySystemComponent> CompanionAbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystemComponent")
	TObjectPtr<UNSCompanionAttributeSet> CompanionAttributeSet;
	
public:
	FORCEINLINE UNSCompanionAbilitySystemComponent* GetCompanionAbilitySystemComponent() const {return CompanionAbilitySystemComponent;}
	
	FORCEINLINE UNSCompanionAttributeSet* GetCompanionAttributeSet() const {return CompanionAttributeSet;}
};
