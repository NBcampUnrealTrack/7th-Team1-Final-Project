// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSInteractableNPCBase.h"
#include "NSPartUpgradeNPC.generated.h"

class UNSPartUpgradeWidget;

// 인런 파츠 NPC — 구매/리롤/등급업
UCLASS()
class NEOSANCTUM_API ANSPartUpgradeNPC : public ANSInteractableNPCBase
{
	GENERATED_BODY()

public:
	virtual TSubclassOf<UNSNPCInteractionWidgetBase> GetInteractionWidgetClass() const override;
	virtual bool CanInteract_Implementation(APlayerController* Interactor) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="NPC|UI")
	TSubclassOf<UNSPartUpgradeWidget> PartUpgradeWidgetClass;
};
