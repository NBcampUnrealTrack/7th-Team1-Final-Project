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

	void InitializeDamageNumber(const FNSDamageNumberFeedbackContext& Context, const FVector2D& DisplayOffset);

private:
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> WidgetComponent;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float LifeTime = 1.0f;

	// BP에서 조절하는, 피격 지점 위로 올릴 월드 높이.
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DamageNumberHeightOffset = 0.0f;
};
