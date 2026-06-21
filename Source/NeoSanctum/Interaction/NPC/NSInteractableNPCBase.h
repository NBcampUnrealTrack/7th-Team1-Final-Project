// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NeoSanctum/Interaction/Core/NSInteractable.h"
#include "NSInteractableNPCBase.generated.h"

class USphereComponent;

UCLASS(Abstract)
class NEOSANCTUM_API ANSInteractableNPCBase : public ACharacter, public INSInteractable
{
	GENERATED_BODY()

public:
	ANSInteractableNPCBase();

	virtual bool CanInteract_Implementation(APlayerController* Interactor) const override;
	virtual FText GetPromptText_Implementation() const override;
protected:
	// 상호작용 감지용 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USphereComponent> DetectionCollision;

	// 감지 반경
	UPROPERTY(EditAnywhere, Category = "Interaction")
	float DetectionRadius = 250.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FName NPCId;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FText PromptText;
};
