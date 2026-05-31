// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "NSInputConfig.generated.h"

class UInputMappingContext;

USTRUCT(BlueprintType)
struct FNSInputMappingContext
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<const UInputMappingContext> InputMappingContext = nullptr;
	
	UPROPERTY(EditDefaultsOnly)
	int32 Priority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InputMode")
	FGameplayTag InputModeTag;
};

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
	
	UFUNCTION(BlueprintCallable, Category = "InputAction")
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;
	
	UFUNCTION(BlueprintCallable, Category = "InputAction")
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InputAction")
	TArray<FNSInputMappingContext> MappingContexts;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FNSInputAction> NativeInputActions;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FNSInputAction> AbilityInputActions;

	// 증강 선택창 전용 입력 (IMC_Augment 활성 중에만 동작)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FNSInputAction> AugmentInputActions;
};
