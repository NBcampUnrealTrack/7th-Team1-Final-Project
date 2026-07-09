// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSCloakComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMeshComponent;
class UCurveFloat;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSCloakComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSCloakComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Utility|Visuals")
	void StartCloak();

	UFUNCTION(BlueprintCallable, Category = "Utility|Visuals")
	void StopCloak();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Cloak")
	float EnterDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Cloak")
	float ExitDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Cloak")
	float TargetCloakAmount = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Cloak")
	FName CloakAmountParamName = TEXT("CloakAmount");

	UPROPERTY(EditDefaultsOnly, Category = "Cloak")
	TObjectPtr<UCurveFloat> CloakCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Cloak")
	bool bIncludeAttachedActors = false;

private:
	void CollectCloakMeshes(TArray<UMeshComponent*>& OutMeshes) const;
	void CollectMeshesFromActor(AActor* TargetActor, TArray<UMeshComponent*>& OutMeshes) const;

	void EnsureDynamicMaterials();
	void BeginTransition(float FromAmount, float ToAmount, float Duration);
	void UpdateCloak();
	void ApplyCloakAmount(float Amount);
	float EvaluateCloakCurve(float NormalizedTime) const;
	void RestoreOriginalMaterials();

	// 인덱스 대응: CachedMeshes[i] / CachedSlotIndices[i] / OriginalMaterials[i] / CloakMIDs[i]
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> CachedMeshes;

	UPROPERTY(Transient)
	TArray<int32> CachedSlotIndices;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> CloakMIDs;

	FTimerHandle CloakTimerHandle;
	float CloakStartTime = 0.0f;
	float CloakStartAmount = 0.0f;
	float CloakTargetAmount = 0.0f;
	float CloakTransitionDuration = 0.0f;
	float CurrentCloakAmount = 0.0f;
};