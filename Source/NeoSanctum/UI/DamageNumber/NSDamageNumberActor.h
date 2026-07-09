// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSDamageNumberActor.generated.h"

class UWidgetComponent;
struct FNSDamageNumberFeedbackContext;

UCLASS()
class NEOSANCTUM_API ANSDamageNumberActor : public AActor
{
	GENERATED_BODY()

public:
	ANSDamageNumberActor();

	void InitializeDamageNumber(const FNSDamageNumberFeedbackContext& Context);

private:
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> WidgetComponent;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float LifeTime = 1.0f;
};
