// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Interaction/NPC/NSInteractableNPCBase.h"
#include "NSPetNPC.generated.h"

class UNSPetUpgradeWidget;

UCLASS()
class NEOSANCTUM_API ANSPetNPC : public ANSInteractableNPCBase
{
	GENERATED_BODY()

public:
	virtual bool OnInteract_Implementation(APlayerController* Interactor) override;

protected:
	// 펫 강화 UI 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category="NPC|UI")
	TSubclassOf<UNSPetUpgradeWidget> PetWidgetClass;
};
