// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NeoSanctum/Interaction/Component/NSInteractionComponent.h"
#include "NSInteractableActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNSCharacterSelectWidget;

UCLASS()
class NEOSANCTUM_API ANSInteractableActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ANSInteractableActor();
	
	UPROPERTY()
	TObjectPtr<APlayerController> CachedInteractor;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNSCharacterSelectWidget> CharacterSelectWidgetClass;

protected:
	virtual void BeginPlay() override;
	
	//상호작용 로직
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnInteract(APlayerController* Interactor);
	//콜리전 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USphereComponent> SphereComponent;
	//상호작용 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UNSInteractionComponent> InteractionComponent;
	//엑터 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
	

private:	
	UFUNCTION()
	void HandleInteracted();
};
