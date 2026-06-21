// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Interaction/NPC/NSInteractableNPCBase.h"
#include "NSPetNPC.generated.h"

UCLASS()
class NEOSANCTUM_API ANSPetNPC : public ANSInteractableNPCBase
{
	GENERATED_BODY()

public:
	virtual bool OnInteract_Implementation(APlayerController* Interactor) override;
};
