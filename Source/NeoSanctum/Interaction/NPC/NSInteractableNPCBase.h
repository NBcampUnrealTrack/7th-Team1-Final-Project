// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NeoSanctum/Interaction/Core/NSInteractable.h"
#include "NSInteractableNPCBase.generated.h"

class USphereComponent;
class UNSNPCInteractionWidgetBase;

UCLASS(Abstract)
class NEOSANCTUM_API ANSInteractableNPCBase : public ACharacter, public INSInteractable
{
	GENERATED_BODY()

public:
	ANSInteractableNPCBase();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual bool CanInteract_Implementation(APlayerController* Interactor) const override;
	virtual FText GetPromptText_Implementation() const override;
	virtual FVector GetPromptWorldLocation_Implementation() const override;

	virtual TSubclassOf<UNSNPCInteractionWidgetBase> GetInteractionWidgetClass() const { return nullptr; }

	FName GetNPCId() const { return NPCId; }
	
	// true면 OnInteract 서버에서 실행, false면 기존 로직 그대로 작동
	virtual bool ShouldHandleOnServer_Implementation() const override { return false; }
protected:
	// 상호작용 감지용 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USphereComponent> DetectionCollision;

	// 프롬프트 위젯이 뜰 위치 — 에디터에서 원하는 머리 위 지점으로 드래그
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USceneComponent> PromptAnchor;

	// 감지 반경
	UPROPERTY(EditAnywhere, Category = "Interaction")
	float DetectionRadius = 100.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FName NPCId;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FText PromptText;
};
