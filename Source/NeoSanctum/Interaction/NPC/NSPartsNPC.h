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
	virtual bool OnInteract_Implementation(APlayerController* Interactor) override;
protected:
	UPROPERTY(EditDefaultsOnly, Category="NPC|UI")
	TSubclassOf<UNSPartEquipWidget> PartEquipWidgetClass;
};
