// Copyright 2026 One Team. All rights reserved.

#include "NSReadyStartNPC.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/UI/Interaction/NSReadyStartWidget.h"
#include "Blueprint/UserWidget.h"

ANSReadyStartNPC::ANSReadyStartNPC()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	DetectionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionCollision"));
	SetRootComponent(DetectionCollision);
	DetectionCollision->SetSphereRadius(DetectionRadius);
	DetectionCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(DetectionCollision);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PromptAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("PromptAnchor"));
	PromptAnchor->SetupAttachment(DetectionCollision);
	PromptAnchor->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
}

void ANSReadyStartNPC::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터에서 DetectionRadius를 바꾸면 스피어 반경도 즉시 반영
	if (DetectionCollision)
	{
		DetectionCollision->SetSphereRadius(DetectionRadius);
	}
}

bool ANSReadyStartNPC::CanInteract_Implementation(APlayerController* Interactor) const
{
	return IsValid(Interactor) && ReadyStartWidgetClass;
}

bool ANSReadyStartNPC::OnInteract_Implementation(APlayerController* Interactor)
{
	if (!IsValid(Interactor) || !ReadyStartWidgetClass)
	{
		return false;
	}

	// 같은 플레이어가 이미 위젯을 열어둔 상태라면,
	// F 입력을 토글처럼 처리해서 위젯을 닫는다.
	if (TObjectPtr<UNSReadyStartWidget>* FoundWidget = OpenedWidgetsByPlayer.Find(Interactor))
	{
		if (IsValid(*FoundWidget))
		{
			CloseOpenedWidget(Interactor);
			return true;
		}

		// 위젯이 이미 제거된 경우 stale entry만 정리한다.
		OpenedWidgetsByPlayer.Remove(Interactor);
	}

	UNSReadyStartWidget* ReadyStartWidget =
		CreateWidget<UNSReadyStartWidget>(Interactor, ReadyStartWidgetClass);

	if (!IsValid(ReadyStartWidget))
	{
		return false;
	}

	OpenedWidgetsByPlayer.Add(Interactor, ReadyStartWidget);

	ReadyStartWidget->AddToViewport();

	Interactor->SetShowMouseCursor(true);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(ReadyStartWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	Interactor->SetInputMode(InputMode);

	return true;
}

FText ANSReadyStartNPC::GetPromptText_Implementation() const
{
	return PromptText;
}

FVector ANSReadyStartNPC::GetPromptWorldLocation_Implementation() const
{
	if (PromptAnchor)
	{
		return PromptAnchor->GetComponentLocation();
	}

	return GetActorLocation();
}
void ANSReadyStartNPC::CloseOpenedWidget(APlayerController* Interactor)
{
	if (!Interactor)
	{
		return;
	}

	if (TObjectPtr<UNSReadyStartWidget>* FoundWidget = OpenedWidgetsByPlayer.Find(Interactor))
	{
		if (IsValid(*FoundWidget))
		{
			(*FoundWidget)->RemoveFromParent();
		}

		OpenedWidgetsByPlayer.Remove(Interactor);
	}

	Interactor->SetShowMouseCursor(false);

	FInputModeGameOnly InputMode;
	Interactor->SetInputMode(InputMode);
}