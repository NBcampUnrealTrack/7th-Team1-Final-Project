// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSDamageFlashComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMeshComponent;
class UCurveFloat;

UENUM(BlueprintType)
enum class ENSDamageFlashTriggerPolicy : uint8
{
	Health,
	Shield,
	Both
};

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

	// 플래시 재생 조건 변경
	void SetTriggerPolicy(ENSDamageFlashTriggerPolicy NewTriggerPolicy);

protected:
	// 플래시 재생 조건
	UPROPERTY(EditDefaultsOnly, Category = "Flash")
	ENSDamageFlashTriggerPolicy TriggerPolicy = ENSDamageFlashTriggerPolicy::Health;

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

	// Shield 플래시에 사용할 색상
	UPROPERTY(EditDefaultsOnly, Category = "Flash")
	FLinearColor ShieldFlashColor = FLinearColor(0.05f, 0.45f, 1.0f, 1.0f);

private:
	void EnsureDynamicMaterials();
	void UpdateFlash(); 
	void StopFlash();                 
	bool ShouldPlayFlash() const;
	bool HasActiveShield() const;
	bool TryGetShieldRatio(float& OutShieldRatio) const;
	FLinearColor ResolveFlashColor() const;
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> OverlayMIDs;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> CachedMeshes;

	FTimerHandle FlashTimerHandle;
	float FlashStartTime = 0.0f;

#pragma region MID 방식

public:
	// 피격 효과가 적용될 실제 외형 MID 배열을 등록하는 함수
	void SetMaterialFlashTargets(const TArray<UMaterialInstanceDynamic*>& InTargetMaterials);

	// 등록된 실제 외형 MID를 초기화하고 해제하는 함수
	void ClearMaterialFlashTargets();

	// 등록된 MID가 있으면 머티리얼 파라미터 방식의 피격 효과를 재생하는 함수
	bool TryPlayMaterialFlash(float Strength = 1.0f);

	// 진행 중인 머티리얼 파라미터 방식의 피격 효과를 중지하는 함수
	void CancelMaterialFlash();

protected:
	// 0~1 시간 구간에서 MID 피격 강도를 결정하는 커브
	UPROPERTY(EditDefaultsOnly, Category = "Flash|Material")
	TObjectPtr<UCurveFloat> MaterialFlashCurve;

	// MID 피격 효과의 전체 지속 시간
	UPROPERTY(EditDefaultsOnly, Category = "Flash|Material", meta = (ClampMin = "0.01"))
	float MaterialFlashDuration = 0.2f;

	// MID에 전달할 피격 색상
	UPROPERTY(EditDefaultsOnly, Category = "Flash|Material")
	FLinearColor MaterialFlashColor = FLinearColor(1.0f, 0.02f, 0.02f, 1.0f);

	// MID에서 피격 색상을 읽는 벡터 파라미터 이름
	UPROPERTY(EditDefaultsOnly, Category = "Flash|Material")
	FName MaterialFlashColorParameterName = TEXT("HitFlashColor");

	// MID에서 피격 강도를 읽는 스칼라 파라미터 이름
	UPROPERTY(EditDefaultsOnly, Category = "Flash|Material")
	FName MaterialFlashAmountParameterName = TEXT("HitFlashAmount");

	// MID의 피격 강도 파라미터에 적용할 최대값
	UPROPERTY(EditDefaultsOnly, Category = "Flash|Material", meta = (ClampMin = "0.0"))
	float MaterialFlashPeakAmount = 1.0f;
	
	// 머티리얼 피격 플래시 색상을 기존 체력 기반 색상 규칙으로 계산할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flash")
	bool bUseHealthBasedMaterialFlashColor = true;

private:
	// 타이머 주기마다 MID 피격 강도를 갱신하는 함수
	void UpdateMaterialFlash();

	// 등록된 모든 MID에 피격 강도를 적용하는 함수
	void ApplyMaterialFlashAmount(float Amount);

	// 정규화된 시간에 대응하는 MID 피격 감쇠값을 반환하는 함수
	float EvaluateMaterialFlashCurve(float NormalizedTime) const;
	
	// 머티리얼 피격 플래시에 적용할 최종 색상을 반환하는 함수
	FLinearColor ResolveMaterialFlashColor() const;

	// 등록된 머티리얼 MID들에 피격 플래시 색상을 적용하는 함수
	void ApplyMaterialFlashColor(const FLinearColor& InFlashColor);

	// 피격 파라미터를 직접 변경할 실제 외형 MID 배열
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> MaterialFlashMIDs;

	// MID 피격 효과 갱신 타이머를 식별하는 핸들
	FTimerHandle MaterialFlashTimerHandle;

	// 현재 MID 피격 효과가 시작된 월드 시간
	float MaterialFlashStartTime = 0.0f;

	// 현재 재생 중인 MID 피격 효과의 입력 강도
	float ActiveMaterialFlashStrength = 1.0f;
#pragma endregion
};
