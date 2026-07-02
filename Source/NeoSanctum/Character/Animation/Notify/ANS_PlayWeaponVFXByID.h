// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_PlayWeaponVFXByID.generated.h"

class UNiagaraComponent;

/**
 * 장착 무기 소켓에 VFX를 붙여 NotifyState 구간 동안 유지하는 Anim Notify State
 */
UCLASS(meta = (DisplayName = "NS Play Weapon VFX By ID"))
class NEOSANCTUM_API UANS_PlayWeaponVFXByID : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference
	) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference
	) override;

	virtual FString GetNotifyName_Implementation() const override;

private:
	// DT_VFXDataTable의 Row Name
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (AllowPrivateAccess = "true"))
	FName VFXID = NAME_None;

	// 무기 Mesh의 VFX 부착 소켓
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (AllowPrivateAccess = "true"))
	FName SocketName = NAME_None;

	// 소켓 기준 위치 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (AllowPrivateAccess = "true"))
	FVector LocationOffset = FVector::ZeroVector;

	// 소켓 기준 회전 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (AllowPrivateAccess = "true"))
	FRotator RotationOffset = FRotator::ZeroRotator;

	// DataTable의 Scale에 추가로 곱해지는 값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", UIMin = "0.0", UIMax = "3.0"))
	float ScaleMultiplier = 1.0f;

	// 소켓을 찾지 못했을 때 무기 Root에 부착할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (AllowPrivateAccess = "true"))
	bool bFallbackToWeaponRoot = false;

private:
	USceneComponent* FindWeaponAttachComponent(USkeletalMeshComponent* MeshComp, FName& OutSocketName) const;
	void StopVFX(USkeletalMeshComponent* MeshComp);

private:
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TWeakObjectPtr<UNiagaraComponent>> ActiveVFXComponents;
};
