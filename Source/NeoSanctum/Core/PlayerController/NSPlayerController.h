// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Input/NSInputConfig.h"
#include "NSPlayerController.generated.h"

struct FInputActionValue;

UCLASS()
class NEOSANCTUM_API ANSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ANSPlayerController();
	
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
public:
	void SetInputConfig(UNSInputConfig* NewInputConfig, ENSInputRoute NewInputRoute);

	void SetInputRoute(ENSInputRoute NewInputRoute);
	
protected:
	void BindInputActions();

	void UnbindInputActions();

	void Input_Move(const FInputActionValue& Value);
	
	void Input_Look(const FInputActionValue& Value);
	
	void Input_AbilityPressed(FGameplayTag InputTag);
	
	void Input_AbilityReleased(FGameplayTag InputTag);
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UNSInputConfig> DefaultInputConfig;

	UPROPERTY(Transient)
	TObjectPtr<UNSInputConfig> CurrentInputConfig;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	ENSInputRoute DefaultInputRoute = ENSInputRoute::KeyboardMouse;
	
	UPROPERTY(Transient)
	ENSInputRoute CurrentInputRoute = ENSInputRoute::KeyboardMouse;

	UPROPERTY(Transient)
	bool bHasAppliedInputConfig = false;

	UPROPERTY(Transient)
	TArray<uint32> NativeInputBindHandles;

	UPROPERTY(Transient)
	TArray<uint32> AbilityInputBindHandles;
};
