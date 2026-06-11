// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/WidgetComponent.h"
#include "NSInteractionComponent.generated.h"

class USphereComponent;
class UNSInteractionPromptWidget;

//상호작용이 가능한 액터가 구현해야하는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteracted);

/**
 * 상호작용 컴포넌트
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSInteractionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UNSInteractionComponent();
	
	//상호작용 실행 (PlayerController에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Interact(APlayerController* Interactor);
	//상호작용 가능 여부
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool CanInteract() const;
	//상호작용 프롬프트 텍스트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	FText InteractionPromptText;
	//상호작용 키 텍스트 (임시 M키)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	FText InteractionKeyText = FText::FromString(TEXT("M"));
	//상호작용 완료
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteracted OnInteracted;
	// 현재 범위 안에 있는 PlayerController 캐싱
	UPROPERTY()
	TObjectPtr<APlayerController> CachedInteractor;
	//월드 스페이스 프롬프트 위젯 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UWidgetComponent> PromptWidgetComponent;
	//상호작용 가능 여부
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	bool bCanInteract = true;
	//상호작용 프롬프트 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	TSubclassOf<UNSInteractionPromptWidget> InteractionPromptWidgetClass;

	
protected:
	virtual void BeginPlay() override;
	
private:
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	void ShowPrompt(APlayerController* PC);
	void HidePrompt(APlayerController* PC);
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType, 
		FActorComponentTickFunction* ThisTickFunction) override;
	
	//상호작용 범위 컴포넌트
	UPROPERTY()
	TObjectPtr<USphereComponent> SphereComponent;

	//현재 표시중인 프롬프트 위젯
	UPROPERTY()
	TObjectPtr<UNSInteractionPromptWidget> PromptWidget;

	
};
