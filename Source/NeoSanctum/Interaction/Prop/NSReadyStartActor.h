// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NeoSanctum/Interaction/Core/NSInteractable.h"
#include "NSReadyStartActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class USceneComponent;
class UNSReadyStartWidget;
/**
* 거점에서 플레이어가 상호작용하면 Ready 상태로 전환하고,
* 모든 플레이어가 Ready 상태라면 인런 시작을 요청하는 액터
*/
UCLASS()
class NEOSANCTUM_API ANSReadyStartActor : public AActor, public INSInteractable
{
	GENERATED_BODY()
	
public:	
	ANSReadyStartActor();
	
	virtual bool CanInteract_Implementation(APlayerController* Interactor) const override;
	virtual bool OnInteract_Implementation(APlayerController* Interactor) override;
	virtual FText GetPromptText_Implementation() const override;
	virtual FVector GetPromptWorldLocation_Implementation() const override;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	// 플레이어 InteractionComponent의 overlap 대상이 되는 감지 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> DetectionCollision;

	// 거점에 배치되는 출발 장치의 외형 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// 상호작용 프롬프트 위젯이 표시될 월드 위치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> PromptAnchor;

	// 에디터에서 조정 가능한 상호작용 감지 반경
	UPROPERTY(EditAnywhere, Category = "Interaction")
	float DetectionRadius = 120.0f;

	// 플레이어가 가까이 갔을 때 표시되는 프롬프트 문구
	UPROPERTY(EditAnywhere, Category = "Interaction")
	FText PromptText = NSLOCTEXT("ReadyStartActor", "PromptText", "Ready / Start");

	// 상호작용 시 열 Ready/Start 위젯
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UNSReadyStartWidget> ReadyStartWidgetClass;
	
	// 플레이어별로 열린 Ready/Start 위젯을 보관한다.
	// 액터는 월드에 하나지만 플레이어는 여러 명일 수 있으므로 PlayerController 기준으로 관리한다.
	UPROPERTY(Transient)
	TMap<TObjectPtr<APlayerController>, TObjectPtr<UNSReadyStartWidget>> OpenedWidgetsByPlayer;

	// 같은 플레이어가 F를 다시 눌렀을 때 열린 위젯을 닫고 입력 모드를 복구한다.
	void CloseOpenedWidget(APlayerController* Interactor);
	void HandleReadyStartWidgetClosed(
		UNSReadyStartWidget* ClosedWidget,
		APlayerController* Interactor);
};
