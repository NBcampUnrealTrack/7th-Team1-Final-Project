// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSDissolveComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSDissolveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSDissolveComponent();

	UFUNCTION(BlueprintCallable, Category = "Utility|Visuals")
	void StartDissolve();
	
protected:
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;
	
	// 디졸브 효과 지속 시간
	UPROPERTY(EditDefaultsOnly, Category = "Dissolve Settings")
	float DissolveDuration = 5.0f;

private:
	void UpdateDissolveAlpha();

	FTimerHandle DissolveTimerHandle;
	
	float DissolveStartTime = 0.0f;
};
