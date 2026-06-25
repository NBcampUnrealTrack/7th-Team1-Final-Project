// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSInteractableNPCBase.h"
#include "NSPartsNPC.generated.h"

class UNSPartEquipWidget;

UCLASS()
class NEOSANCTUM_API ANSPartsNPC : public ANSInteractableNPCBase
{
	GENERATED_BODY()

public:
	virtual TSubclassOf<UNSNPCInteractionWidgetBase> GetInteractionWidgetClass() const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="NPC|UI")
	TSubclassOf<UNSPartEquipWidget> PartEquipWidgetClass;
};
