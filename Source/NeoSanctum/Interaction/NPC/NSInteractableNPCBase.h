// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NeoSanctum/Interaction/Core/NSInteractable.h"
#include "NSInteractableNPCBase.generated.h"

UCLASS(Abstract)
class NEOSANCTUM_API ANSInteractableNPCBase : public ACharacter, public INSInteractable
{
	GENERATED_BODY()

public:
	ANSInteractableNPCBase();

	virtual bool CanInteract_Implementation(APlayerController* Interactor) const override;
	virtual FText GetPromptText_Implementation() const override;
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FName NPCId;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FText PromptText;
};
