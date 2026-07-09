// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Combat/Cosmetic/NSCosmeticEventHandler.h"
#include "NSCosmeticHandler_Laser.generated.h"

class UAudioComponent;
class UNiagaraComponent;

USTRUCT()
struct FNSActiveLaserCosmetic
{
	GENERATED_BODY()

	// 레이저 차징 사운드 컴포넌트를 저장하는 변수
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ChargeAudioComponent;

	// 레이저 발사 사운드 컴포넌트를 저장하는 변수
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> BeamAudioComponent;

	// 레이저 차징 Niagara 컴포넌트를 저장하는 변수
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ChargeVFXComponent;

	// 레이저 Beam Niagara 컴포넌트 목록을 저장하는 변수
	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> BeamVFXComponents;
};

UCLASS(Blueprintable, BlueprintType)
class NEOSANCTUM_API UNSCosmeticHandler_Laser : public UNSCosmeticEventHandler
{
	GENERATED_BODY()

public:
	// Laser Handler가 처리할 EventTag 목록을 반환하는 함수
	virtual void GetHandledEventTags(TArray<FGameplayTag>& OutEventTags) const override;

	// Laser 코스메틱 이벤트를 처리하는 함수
	virtual void HandleEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData) override;

	// Handler가 제거될 때 유지 중인 레이저 코스메틱을 정리하는 함수
	virtual void BeginDestroy() override;

private:
	// 레이저 차징 사운드 ID를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Laser")
	FName LaserChargeSoundID = FName(TEXT("Monster_TitanWalker_Laser_Charge"));

	// 레이저 Beam 사운드 ID를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Laser")
	FName LaserBeamSoundID = FName(TEXT("Monster_TitanWalker_Laser_Beam"));

	// 레이저 차징 Niagara VFX ID를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Laser")
	FName LaserChargeVFXID = FName(TEXT("Monster_TitanWalker_Laser_Charge"));

	// 레이저 Beam Niagara VFX ID를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Laser")
	FName LaserBeamVFXID = FName(TEXT("Monster_TitanWalker_Laser_Beam"));

	// Charge Niagara의 차징 시간 User Parameter 이름을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Laser")
	FName LaserChargeDurationParameterName = FName(TEXT("User.ChargeDuration"));

	// Beam Niagara의 끝점 User Parameter 이름을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Laser")
	FName LaserBeamEndParameterName = FName(TEXT("User.Beam End"));

	// Beam Niagara의 두께 User Parameter 이름을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Laser")
	FName LaserBeamWidthParameterName = FName(TEXT("User.Beam Width"));

	// Beam Niagara 컴포넌트 스케일을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Laser", meta = (ClampMin = "0.01"))
	float LaserBeamVFXScale = 1.0f;

	// AreaData.Radius를 Beam 표시 두께로 변환할 배율을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Laser", meta = (ClampMin = "0.0"))
	float LaserBeamWidthRadiusMultiplier = 2.0f;

	// Beam Niagara 최소 표시 두께를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Laser", meta = (ClampMin = "0.0"))
	float LaserBeamMinVisualWidth = 4.0f;

	// 차징 사운드에 Beam 사운드까지 포함되어 있는지 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Laser")
	bool bLaserBeamSoundIncludedInChargeSound = true;

	// 레이저 사운드 FadeOut 시간을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Laser", meta = (ClampMin = "0.0"))
	float SoundFadeOutTime = 0.1f;

	// InstanceId별 활성 레이저 코스메틱을 저장하는 변수
	UPROPERTY(Transient)
	TMap<int32, FNSActiveLaserCosmetic> ActiveLasers;

private:
	// 레이저 차징 시작 이벤트를 처리하는 함수
	void HandleChargeStartEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData);

	// 레이저 차징 갱신 이벤트를 처리하는 함수
	void HandleChargeUpdateEvent(const FNSCosmeticEventNetData& EventData);

	// 레이저 Beam 시작 이벤트를 처리하는 함수
	void HandleBeamStartEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData);

	// 레이저 Beam 갱신 이벤트를 처리하는 함수
	void HandleBeamUpdateEvent(const FNSCosmeticEventNetData& EventData);

	// 레이저 종료 이벤트를 처리하는 함수
	void HandleStopEvent(const FNSCosmeticEventNetData& EventData);

	// 현재 EventData 기준으로 Charge VFX 위치를 갱신하는 함수
	void UpdateChargeVFX(FNSActiveLaserCosmetic& ActiveLaser, const FNSCosmeticEventNetData& EventData) const;

	// 현재 EventData 기준으로 Beam VFX 위치와 크기를 갱신하는 함수
	void UpdateBeamVFX(FNSActiveLaserCosmetic& ActiveLaser, AActor* OwnerActor,
	                   const FNSCosmeticEventNetData& EventData) const;

	// 레이저 Niagara 컴포넌트를 즉시 비활성화하고 제거하는 함수
	void DestroyLaserVFXComponent(UNiagaraComponent* VFX) const;

	// Charge VFX만 정리하는 함수
	void StopChargeVFX(FNSActiveLaserCosmetic& ActiveLaser) const;

	// Charge 사운드만 정리하는 함수
	void StopChargeSound(FNSActiveLaserCosmetic& ActiveLaser) const;

	// 특정 InstanceId의 레이저 코스메틱을 정리하는 함수
	void StopLaserCosmetic(int32 InstanceId);

	// EventData의 Radius 기준 Beam 표시 두께를 계산하는 함수
	float GetLaserBeamVisualWidth(const FNSCosmeticEventNetData& EventData) const;
};
