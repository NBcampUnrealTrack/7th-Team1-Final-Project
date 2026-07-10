// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Combat/Cosmetic/NSCosmeticEventHandler.h"
#include "NSCosmeticHandler_Flame.generated.h"

class UAudioComponent;
class UNiagaraComponent;

USTRUCT()
struct FNSActiveFlameCosmetic
{
	GENERATED_BODY()

	// 화염 지속 사운드 컴포넌트를 저장하는 변수
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> AudioComponent;

	// 화염 Niagara 컴포넌트 목록을 저장하는 변수
	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> VFXComponents;
};

UCLASS(Blueprintable, BlueprintType)
class NEOSANCTUM_API UNSCosmeticHandler_Flame : public UNSCosmeticEventHandler
{
	GENERATED_BODY()

public:
	// Flame Handler가 처리할 EventTag 목록을 반환하는 함수
	virtual void GetHandledEventTags(TArray<FGameplayTag>& OutEventTags) const override;

	// Flame 코스메틱 이벤트를 처리하는 함수
	virtual void HandleEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData) override;

	// Handler가 제거될 때 유지 중인 화염 코스메틱을 정리하는 함수
	virtual void BeginDestroy() override;

private:
	// 화염 지속 사운드 ID를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Flame")
	FName FlameSoundID = FName(TEXT("Monster_TitanWalker_Flame_Loop"));

	// 화염 Niagara VFX ID를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Flame")
	FName FlameVFXID = FName(TEXT("Monster_TitanWalker_Flame_Loop"));

	// 화염 Niagara 컴포넌트 추가 스케일을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Flame", meta = (ClampMin = "0.01"))
	float FlameVFXComponentScale = 1.0f;

	// 사거리 방향 Niagara 배치 간격을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Flame", meta = (ClampMin = "1.0"))
	float ForwardSpacing = 180.0f;

	// 좌우 폭 방향 Niagara 배치 간격을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Flame", meta = (ClampMin = "1.0"))
	float LateralSpacing = 160.0f;

	// 소켓 하나당 생성 가능한 최대 Niagara 수를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Flame", meta = (ClampMin = "1"))
	int32 MaxVFXPerEmitter = 20;

	// 화염이 소켓에서 몇 cm 앞부터 보일지 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Flame", meta = (ClampMin = "0.0"))
	float StartOffset = 0.0f;

	// NS_Fire Flame Scale User Parameter 이름을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Flame")
	FName FlameScaleParameterName = FName(TEXT("User.Flame Scale"));

	// NS_Fire Spawn Rate User Parameter 이름을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Flame")
	FName SpawnRateParameterName = FName(TEXT("User.Spawn Rate"));

	// NS_Fire 개별 불꽃 크기 값을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Flame", meta = (ClampMin = "0.0"))
	float NiagaraFlameScale = 1.4f;

	// NS_Fire Spawn Rate 값을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Flame", meta = (ClampMin = "0.0"))
	float NiagaraSpawnRate = 80.0f;

	// Niagara 전방 축 보정 회전값을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Flame")
	FRotator RotationOffset = FRotator::ZeroRotator;

	// 화염 사운드 FadeOut 시간을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Cosmetic|Flame", meta = (ClampMin = "0.0"))
	float SoundFadeOutTime = 0.15f;

	// InstanceId별 활성 화염 코스메틱을 저장하는 변수
	UPROPERTY(Transient)
	TMap<int32, FNSActiveFlameCosmetic> ActiveFlames;

private:
	// 화염 시작 이벤트를 처리하는 함수
	void HandleStartEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData);

	// 화염 갱신 이벤트를 처리하는 함수
	void HandleUpdateEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData);

	// 화염 종료 이벤트를 처리하는 함수
	void HandleStopEvent(const FNSCosmeticEventNetData& EventData);

	// 현재 EventData 기준으로 화염 VFX 위치를 갱신하는 함수
	void UpdateFlameVFX(FNSActiveFlameCosmetic& ActiveFlame, AActor* OwnerActor, const FNSCosmeticEventNetData& EventData) const;
	
	// 화염 Niagara 컴포넌트를 즉시 비활성화하고 제거하는 함수
	void DestroyFlameVFXComponent(UNiagaraComponent* VFX) const;

	// EventData의 Cone 범위 안에 배치할 VFX Transform 목록을 생성하는 함수
	void BuildFlameVFXTransforms(const FNSCosmeticEventNetData& EventData, TArray<FTransform>& OutTransforms) const;

	// 특정 InstanceId의 화염 코스메틱을 정리하는 함수
	void StopFlameCosmetic(int32 InstanceId);
};
