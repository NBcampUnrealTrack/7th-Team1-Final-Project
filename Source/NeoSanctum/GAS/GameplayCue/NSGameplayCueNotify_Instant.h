// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "NSGameplayCueNotify_Instant.generated.h"

class UNiagaraSystem;
class UAudioComponent;
class USceneComponent;

/**
 * 즉발형 GameplayCue 연출
 * 사운드는 NSSoundSubsystem의 SoundID 기반으로 일단 PlaySoundAttached만 사용
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
	USceneComponent* GetAttachComponent(AActor* MyTarget, FName SocketName) const;
	
	// 사운드 재생
	UAudioComponent* PlayAttachedSound(AActor* MyTarget, FName SoundID, FName SocketName) const;
	// VFX 재생
	void SpawnAttachedVFX(AActor* MyTarget, UNiagaraSystem* NiagaraSystem, FName SocketName) const;
	
protected:
	// 사운드 ID : DataTable상의 RowName
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|Sound")
	FName ExecuteSoundID = NAME_None;

	// 사운드를 붙힐 소켓 : None이면 그냥 MeshComponent root로 고정임
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|Sound")
	FName SoundAttachSocketName = NAME_None;

	// VFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX")
	TObjectPtr<UNiagaraSystem> ExecuteVFX;

	// VFX를 붙힐 소켓 : None이면 그냥 MeshComponent root로 고정임
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CustomCue|VFX")
	FName VFXAttachSocketName = NAME_None;
};
