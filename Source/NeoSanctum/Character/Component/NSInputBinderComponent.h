// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Input/NSInputConfig.h"
#include "NSInputBinderComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputComponent;
class UNSInputComponent;
class UNSInputConfig;
struct FInputActionValue;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSInputBinderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSInputBinderComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	void InitializePlayerInput(UInputComponent* PlayerInputComponent);
	void SetInputConfig(UNSInputConfig* NewConfig);
	void SetActiveInputModeTags(const FGameplayTagContainer& NewInputModeTags);

protected:
	void ApplyInputConfig();
	void RemoveInputConfig();
	void BindInputActions();
	void UnbindInputActions();

protected:
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_Jump();

	void Input_AbilityPressed(FGameplayTag InputTag);
	void Input_AbilityReleased(FGameplayTag InputTag);

protected:
	UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem() const;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UNSInputConfig> DefaultInputConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FGameplayTagContainer DefaultInputModeTags;

	UPROPERTY(Transient)
	TObjectPtr<UNSInputConfig> CurrentInputConfig;

	UPROPERTY(Transient)
	TObjectPtr<UNSInputComponent> InputComponent;

	UPROPERTY(Transient)
	FGameplayTagContainer ActiveInputModeTags;

	UPROPERTY(Transient)
	bool bHasAppliedInputConfig = false;

	UPROPERTY(Transient)
	TArray<uint32> NativeInputBindHandles;

	UPROPERTY(Transient)
	TArray<uint32> AbilityInputBindHandles;
};
