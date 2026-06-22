// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_PlaySoundByID.generated.h"

UENUM(BlueprintType)
enum class ENSAnimNotifySoundMode : uint8
{
	Attached UMETA(DisplayName = "Attached To Mesh"),
	AtLocation UMETA(DisplayName = "At Socket Location")
};

/**
 * DT_SoundDataTable의 Row Name을 SoundID로 사용하여
 * UNSSoundSubsystem을 통해 사운드를 재생하는 Anim Notify
 */
UCLASS(meta = (DisplayName = "NS Play Sound By ID"))
class NEOSANCTUM_API UAN_PlaySoundByID : public UAnimNotify
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
	// DT_SoundDataTable의 Row Name
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	FName SoundID = NAME_None;

	// 메시 부착 또는 Notify 발생 당시 위치에서 재생
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	ENSAnimNotifySoundMode SoundMode = ENSAnimNotifySoundMode::Attached;

	// 비어 있으면 SkeletalMeshComponent의 원점 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	FName SocketName = NAME_None;

	// DataTable의 Pitch에 추가로 곱해지는 값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.01", UIMin = "0.5", UIMax = "2.0"))
	float PitchMultiplier = 1.0f;
};
