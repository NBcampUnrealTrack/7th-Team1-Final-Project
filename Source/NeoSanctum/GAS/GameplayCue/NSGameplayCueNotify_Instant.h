// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "NeoSanctum/GAS/GameplayCue/NSGameplayCueTypes.h"
#include "NSGameplayCueNotify_Instant.generated.h"

class UAudioComponent;
class USceneComponent;

/**
 * 즉발형 GameplayCue 연출
 * 사운드는 NSSoundSubsystem의 SoundID 기반으로 재생
 * 재생모드를 설정해서 소켓에서 재생할지, 특정 Location에서 재생할지 선택 가능
 */
UCLASS(Blueprintable)
class NEOSANCTUM_API UNSGameplayCueNotify_Instant : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters
	) const override;
	
protected:
	USceneComponent* GetAttachComponent(
		AActor* MyTarget,
		FName ComponentName,
		FName SocketName,
		FName& OutSocketName
	) const;
	
	// 사운드 재생
	UAudioComponent* PlaySound(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters,
		FName SoundID,
		ENSGameplayCueSpawnMode SpawnMode,
		FName ComponentName,
		FName SocketName
	) const;

	// VFX 재생
	void SpawnVFX(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters,
		FName VFXID,
		ENSGameplayCueSpawnMode SpawnMode,
		FName ComponentName,
		FName SocketName
	) const;
	
protected:
	// 사운드 ID : DataTable상의 RowName
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|Sound")
	FName ExecuteSoundID = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|Sound")
	ENSGameplayCueSpawnMode SoundSpawnMode = ENSGameplayCueSpawnMode::Location;

	// 사운드를 붙힐 메쉬 : None으로 설정해도 소켓 이름을 설정했다면 메쉬를 모두 탐색함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|Sound",
		meta = (EditCondition = "SoundSpawnMode == ENSGameplayCueSpawnMode::Attached", EditConditionHides))
	FName SoundAttachComponentName = NAME_None;
	
	// 사운드를 붙힐 소켓 : None이면 그냥 MeshComponent root로 고정임
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|Sound",
		meta = (EditCondition = "SoundSpawnMode == ENSGameplayCueSpawnMode::Attached", EditConditionHides))
	FName SoundAttachSocketName = NAME_None;

	// VFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX")
	FName ExecuteVFXID = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX")
	ENSGameplayCueSpawnMode VFXSpawnMode = ENSGameplayCueSpawnMode::Location;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX", meta = (ClampMin = "0.0"))
	float VFXScaleMultiplier = 1.0f;

	// true인 GameplayCue만 RawMagnitude를 폭발 반경으로 사용하게 함.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX")
	bool bScaleVFXByEffectRadius = false;

	// 이 반경일 때 기존 이펙트 크기가 1배가 되도록 맞춰줌.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX",
		meta = (EditCondition = "bScaleVFXByEffectRadius", ClampMin = "1.0"))
	float VFXBaseRadius = 300.0f;

	// RawMagnitude를 전달할 Niagara User Float 이름 : None이면 컴포넌트 스케일 사용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX",
		meta = (EditCondition = "bScaleVFXByEffectRadius", EditConditionHides))
	FName VFXRadiusUserParameterName = NAME_None;

	// VFX를 붙힐 메쉬 : None으로 설정해도 소켓 이름을 설정했다면 메쉬를 모두 탐색함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX",
		meta = (EditCondition = "VFXSpawnMode == ENSGameplayCueSpawnMode::Attached", EditConditionHides))
	FName VFXAttachComponentName = NAME_None;
	
	// VFX를 붙힐 소켓 : None이면 그냥 MeshComponent root로 고정임
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX",
		meta = (EditCondition = "VFXSpawnMode == ENSGameplayCueSpawnMode::Attached", EditConditionHides))
	FName VFXAttachSocketName = NAME_None;
};
