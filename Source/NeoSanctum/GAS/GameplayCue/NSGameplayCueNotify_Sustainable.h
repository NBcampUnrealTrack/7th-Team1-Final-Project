// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "NeoSanctum/GAS/GameplayCue/NSGameplayCueTypes.h"
#include "NSGameplayCueNotify_Sustainable.generated.h"

class UAudioComponent;
class UNiagaraComponent;
class USceneComponent;

/**
 * 지속형 GameplayCue 연출
 * OnActive에서 시작, OnRemove에서 정리
 * 사운드는 NSSoundSubsystem의 SoundID 기반으로 일단 PlaySoundAttached만 사용
 */
UCLASS(Blueprintable)
class NEOSANCTUM_API ANSGameplayCueNotify_Sustainable : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	ANSGameplayCueNotify_Sustainable();

	virtual bool OnActive_Implementation(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters
	) override;

	virtual bool WhileActive_Implementation(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters
	) override;

	virtual bool OnRemove_Implementation(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters
	) override;
	
protected:
	USceneComponent* GetAttachComponent(
		AActor* MyTarget,
		FName ComponentName,
		FName SocketName,
		FName& OutSocketName
	) const;
	UAudioComponent* PlaySound(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters,
		FName SoundID,
		ENSGameplayCueSpawnMode SpawnMode,
		FName ComponentName,
		FName SocketName
	) const;
	UNiagaraComponent* SpawnVFX(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters,
		FName VFXID,
		ENSGameplayCueSpawnMode SpawnMode,
		FName ComponentName,
		FName SocketName
	) const;
	void LoopPresentation(AActor* MyTarget, const FGameplayCueParameters& Parameters);

protected:
	// 시작 시점에 1회 재생될 사운드 ID : DataTable상의 RowName
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|Sound")
	FName StartSoundID = NAME_None;

	// 종료 시점까지 계속 재생될 사운드 ID : DataTable상의 RowName
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|Sound")
	FName LoopSoundID = NAME_None;

	// 종료 시점에 1회 재생될 사운드 ID : DataTable상의 RowName
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|Sound")
	FName EndSoundID = NAME_None;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|Sound", meta = (ClampMin = "0.0"))
	float LoopSoundFadeOutTime = -1.f;
	
protected:
	// Cue가 Add되는 시점에 1회 재생할 VFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX")
	FName StartVFXID = NAME_None;

	// End 시점까지 계속 재생할 루프 VFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX")
	FName LoopVFXID = NAME_None;

	// Cue가 End되는 시점에 1회 재생할 VFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX")
	FName EndVFXID = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX")
	ENSGameplayCueSpawnMode VFXSpawnMode = ENSGameplayCueSpawnMode::Location;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX", meta = (ClampMin = "0.0"))
	float VFXScaleMultiplier = 1.0f;

	// VFX를 붙힐 메쉬 : None으로 설정해도 소켓 이름을 설정했다면 메쉬를 모두 탐색함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX",
		meta = (EditCondition = "VFXSpawnMode == ENSGameplayCueSpawnMode::Attached", EditConditionHides))
	FName VFXAttachComponentName = NAME_None;

	// VFX를 붙힐 소켓 : None이면 그냥 MeshComponent root로 고정임
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX",
		meta = (EditCondition = "VFXSpawnMode == ENSGameplayCueSpawnMode::Attached", EditConditionHides))
	FName VFXAttachSocketName = NAME_None;
	
protected:
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> LoopAudioComponent;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> LoopVFXComponent;
};
