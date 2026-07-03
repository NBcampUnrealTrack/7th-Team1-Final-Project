// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_PlayVFXByID.generated.h"

UENUM(BlueprintType)
enum class ENSAnimNotifyVFXMode : uint8
{
	Attached UMETA(DisplayName = "Attached To Mesh"),
	AtLocation UMETA(DisplayName = "At Socket Location")
};

/**
 * DT_VFXDataTable의 Row Name을 VFXID로 사용하여
 * UNSVFXSubsystem을 통해 VFX를 재생하는 Anim Notify
 * AN_PlaySoundByID를 카피했기 때문에 완전히 동일한 구조임
 */
UCLASS(meta = (DisplayName = "NS Play VFX By ID"))
class NEOSANCTUM_API UAN_PlayVFXByID : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference
	) override;

	virtual FString GetNotifyName_Implementation() const override;

private:
	// DT_VFXDataTable의 Row Name
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (AllowPrivateAccess = "true"))
	FName VFXID = NAME_None;

	// 메시 부착 또는 Notify 발생 당시 위치에서 재생
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (AllowPrivateAccess = "true"))
	ENSAnimNotifyVFXMode VFXMode = ENSAnimNotifyVFXMode::Attached;

	// 비어 있으면 SkeletalMeshComponent의 원점 사용
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
};
