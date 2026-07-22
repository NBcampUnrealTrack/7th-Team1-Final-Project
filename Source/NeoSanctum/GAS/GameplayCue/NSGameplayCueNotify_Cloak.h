#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "NSGameplayCueNotify_Cloak.generated.h"

class UMaterialInstanceDynamic;

/**
 * 지속형 클로킹 GameplayCue
 * OnActive: 타겟 메시 머티리얼 MID 생성 + 클로킹 시작 파라미터(페이드 인)
 * OnRemove: 해제 파라미터(페이드 아웃) — 머티리얼의 Time 기반 로직이 전환 처리
 */
UCLASS(Blueprintable)
class NEOSANCTUM_API ANSGameplayCueNotify_Cloak : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	ANSGameplayCueNotify_Cloak();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
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
	void ApplyCloakParams(AActor* MyTarget, float Direction, float FadeTime);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> CloakMIDs;
};