// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSInteractableNPCBase.h"
#include "NSCharacterSelectNPC.generated.h"

UCLASS()
class NEOSANCTUM_API ANSCharacterSelectNPC : public ANSInteractableNPCBase
{
	GENERATED_BODY()

public:
	virtual bool CanInteract_Implementation(APlayerController* Interactor) const override;
	virtual bool OnInteract_Implementation(APlayerController* Interactor) override;
};
