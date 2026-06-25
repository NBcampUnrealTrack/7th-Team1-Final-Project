// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_GameplayEvent.generated.h"

UCLASS()
class NEOSANCTUM_API UAN_GameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

protected:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, Category = "GAS")
	FGameplayTagContainer EventTags;

	// 서버 권한 Actor에서 Event 전송
	UPROPERTY(EditAnywhere, Category = "GAS")
	bool bSendOnAuthority = true;

	// 로컬 조작 Actor에서 Event 전송
	UPROPERTY(EditAnywhere, Category = "GAS")
	bool bSendOnLocallyControlled = false;
};
