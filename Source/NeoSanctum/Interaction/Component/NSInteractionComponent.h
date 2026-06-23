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

	// 로컬 컨트롤 폰에서만 감지 활성화. 비로컬/중복 호출은 무시되도록
	void EnableLocalInteraction();

protected:
	// 서버에서 CanInteract 재검증 후 Client_OnInteractApproved 호출
	UFUNCTION(Server, Reliable)
	void Server_RequestInteract(AActor* Target);

	// 서버 승인 후 클라이언트에서 UI 오픈
	UFUNCTION(Client, Reliable)
	void Client_OnInteractApproved(AActor* Target);

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
	bool IsOwnerLocallyControlled() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
	TObjectPtr<USphereComponent> DetectionSphere;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
	TObjectPtr<UWidgetComponent> PromptWidgetComponent;
	
	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	float DetectionRadius = 200.f;

	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	FText InteractionKeyText = FText::FromString(TEXT("F"));

	// 기본 심플 프롬프트 위젯
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Prompt")
	TSubclassOf<UNSInteractionPromptWidget> DefaultPromptWidgetClass;

	// 파츠 전용 프롬프트 위젯 —> 아이콘/이름 표시
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Prompt")
	TSubclassOf<UNSInteractionPromptWidget> PartPromptWidgetClass;

private:
	// 현재 WidgetComponent에 세팅된 클래스 캐싱
	TSubclassOf<UNSInteractionPromptWidget> ActivePromptWidgetClass;
	
private:
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> Candidates;
	
	UPROPERTY()
	TWeakObjectPtr<AActor> ActiveTarget;

	// 중복 셋업 방지
	bool bLocalInteractionEnabled = false;
};
