// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "NeoSanctum/GAS/GameplayCue/NSGameplayCueTypes.h"
#include "NSGameplayCueNotify_Instant.generated.h"

class UNiagaraSystem;
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
		UNiagaraSystem* NiagaraSystem,
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
	TObjectPtr<UNiagaraSystem> ExecuteVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX")
	ENSGameplayCueSpawnMode VFXSpawnMode = ENSGameplayCueSpawnMode::Location;

	// VFX를 붙힐 메쉬 : None으로 설정해도 소켓 이름을 설정했다면 메쉬를 모두 탐색함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX",
		meta = (EditCondition = "VFXSpawnMode == ENSGameplayCueSpawnMode::Attached", EditConditionHides))
	FName VFXAttachComponentName = NAME_None;
	
	// VFX를 붙힐 소켓 : None이면 그냥 MeshComponent root로 고정임
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX",
		meta = (EditCondition = "VFXSpawnMode == ENSGameplayCueSpawnMode::Attached", EditConditionHides))
	FName VFXAttachSocketName = NAME_None;
};
