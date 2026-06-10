// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSDamageFlashComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMeshComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSDamageFlashComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UNSDamageFlashComponent();
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// CueNotify가 호출하는 단일 진입점 (순수 클라 비주얼)
	UFUNCTION(BlueprintCallable, Category = "Utility|Visuals")
	void PlayFlash();

	// 사망/풀링 시 진행 중 플래시 정리 (타이머 클리어 + 오버레이 해제)
	void CancelFlash();

protected:
	// 오버레이용 머티리얼 (Translucent + Unlit, FlashColor/Opacity 파라미터 보유)
	UPROPERTY(EditDefaultsOnly, Category = "Flash")
	TObjectPtr<UMaterialInterface> FlashMaterial;

	// 플래시 1회 지속 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Flash")
	float FlashDuration = 0.15f;

	// 최대 불투명도
	UPROPERTY(EditDefaultsOnly, Category = "Flash")
	float PeakOpacity = 1.0f;
	
	//색상 강도
	UPROPERTY(EditDefaultsOnly, Category = "Flash")
	float ColorPower = 10.0f;
	
	// 체력 구간 임계값(비율). >=High 흰색, >=Low 노랑, 그 밑 빨강
	UPROPERTY(EditDefaultsOnly, Category = "Flash")
	float HighHealthThreshold = 0.50f;

	UPROPERTY(EditDefaultsOnly, Category = "Flash")
	float LowHealthThreshold = 0.10f;

private:
	void EnsureDynamicMaterials();
	void UpdateFlash(); 
	void StopFlash();                 
	FLinearColor ResolveFlashColor() const;
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> OverlayMIDs;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> CachedMeshes;

	FTimerHandle FlashTimerHandle;
	float FlashStartTime = 0.0f;
};
