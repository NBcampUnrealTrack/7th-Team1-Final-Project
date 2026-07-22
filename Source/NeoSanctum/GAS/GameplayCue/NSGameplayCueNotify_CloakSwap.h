#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "NSGameplayCueNotify_CloakSwap.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS(Blueprintable)
class NEOSANCTUM_API ANSGameplayCueNotify_CloakSwap : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	ANSGameplayCueNotify_CloakSwap();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cloak")
	TObjectPtr<UMaterialInterface> CloakMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cloak")
	FName StartTimeParam = TEXT("CloakStartTime");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cloak")
	FName DirectionParam = TEXT("CloakDirection");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cloak")
	FName FadeTimeParam = TEXT("CloakFadeTime");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cloak", meta = (ClampMin = "0.0"))
	float FadeInTime = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cloak", meta = (ClampMin = "0.0"))
	float FadeOutTime = 0.35f;

private:
	USkeletalMeshComponent* GetTargetMesh(AActor* MyTarget) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> CloakMIDs;
};