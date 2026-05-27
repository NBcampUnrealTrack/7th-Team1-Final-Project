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
	FGameplayTag EventTag;
};
