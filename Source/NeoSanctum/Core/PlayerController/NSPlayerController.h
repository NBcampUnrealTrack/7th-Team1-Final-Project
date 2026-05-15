// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "NSPlayerController.generated.h"

struct FInputActionValue;
class UInputMappingContext;
class UNSInputConfig;

UCLASS()
class NEOSANCTUM_API ANSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ANSPlayerController();
	
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
protected:
	void Input_Move(const FInputActionValue& Value);
	
	void Input_Look(const FInputActionValue& Value);
	
	void Input_AbilityPressed(FGameplayTag InputTag);
	
	void Input_AbilityReleased(FGameplayTag InputTag);
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UNSInputConfig> InputConfig;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultIMC;
	
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> CurrentIMC;
};
