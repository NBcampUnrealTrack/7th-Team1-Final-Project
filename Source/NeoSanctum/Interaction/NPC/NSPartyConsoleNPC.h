// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSInteractableNPCBase.h"
#include "NSPartyConsoleNPC.generated.h"

UCLASS()
class NEOSANCTUM_API ANSPartyConsoleNPC : public ANSInteractableNPCBase
{
	GENERATED_BODY()

public:
	virtual TSubclassOf<UNSNPCInteractionWidgetBase> GetInteractionWidgetClass() const override
	{
		return PartyConsoleWidgetClass;
	}

	// 콘솔은 항상 상호작용 가능
	virtual bool CanInteract_Implementation(APlayerController* Interactor) const override
	{
		return Interactor != nullptr;
	}

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Party")
	TSubclassOf<UNSNPCInteractionWidgetBase> PartyConsoleWidgetClass;
};
