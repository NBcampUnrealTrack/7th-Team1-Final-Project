#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "NSGameplayCueNotify_WarningDecal.generated.h"

class UDecalComponent;
class UMaterialInterface;

/**
 * 지속형 경고 데칼 GameplayCue
 * OnActive(AddGameplayCue) 시 Parameters.Location에 데칼 스폰, OnRemove 시 정리
 */
UCLASS(Blueprintable)
class NEOSANCTUM_API ANSGameplayCueNotify_WarningDecal : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	ANSGameplayCueNotify_WarningDecal();

	virtual bool OnActive_Implementation(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters) override;

	virtual bool OnRemove_Implementation(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters) override;

protected:
	// 경고 데칼 머티리얼 (존별로 다르게 하려면 태그별 큐 BP에서 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WarningDecal")
	TObjectPtr<UMaterialInterface> DecalMaterial;

	// 데칼 크기 (X=투영 깊이, Y/Z=바닥 위 반경)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WarningDecal")
	FVector DecalSize = FVector(256.f, 256.f, 256.f);

	// 바닥 투영 회전 (기본: 아래로 투영)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WarningDecal")
	FRotator DecalRotation = FRotator(-90.f, 0.f, 0.f);

private:
	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> SpawnedDecal;
};