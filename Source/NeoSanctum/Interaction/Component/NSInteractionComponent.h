// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSInteractionComponent.generated.h"

class USphereComponent;
class UWidgetComponent;
class UNSInteractionPromptWidget;

UCLASS(ClassGroup=(NeoSanctum), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSInteractionComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UNSInteractionComponent();
	
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// PlayerController의 Interact 입력에서 호출
	void TryInteract();
	
protected:
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	// 후보 중 가장 가까운 유효 대상 갱신
	void UpdateActiveTarget();
	void ShowPromptFor(AActor* Target);
	void HidePrompt();
	
	APlayerController* GetOwnerController() const;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
	TObjectPtr<USphereComponent> DetectionSphere;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
	TObjectPtr<UWidgetComponent> PromptWidgetComponent;
	
	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	float DetectionRadius = 200.f;
	
	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	float PromptHeightOffset = 100.f;
	
	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	FText InteractionKeyText = FText::FromString(TEXT("F"));

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	TSubclassOf<UNSInteractionPromptWidget> PromptWidgetClass;
	
private:
	UPROPERTY()
	TArray<TObjectPtr<AActor>> Candidates;
	
	UPROPERTY()
	TObjectPtr<AActor> ActiveTarget;
};

