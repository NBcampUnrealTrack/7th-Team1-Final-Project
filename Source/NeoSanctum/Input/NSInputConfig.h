// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "NSInputConfig.generated.h"

USTRUCT(BlueprintType)
struct FNSInputAction
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InputTag")
	FGameplayTag InputTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InputTag")
	ETriggerEvent PressedTriggerEvent = ETriggerEvent::Triggered;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InputTag")
	ETriggerEvent ReleasedTriggerEvent = ETriggerEvent::Completed;
};

UCLASS()
class NEOSANCTUM_API UNSInputConfig : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UNSInputConfig(const FObjectInitializer& ObjectInitializer);
	
	UFUNCTION(BlueprintCallable, Category = "Pawn")
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;
	
	UFUNCTION(BlueprintCallable, Category = "Pawn")
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FNSInputAction> NativeInputActions;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FNSInputAction> AbilityInputActions;	
};
