// Copyright 2026 One Team. All rights reserved.

#include "NSReadyStartActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/UI/Interaction/NSReadyStartWidget.h"
#include "NeoSanctum/Core/Waypoint/NSOutRunGuideSubsystem.h"
#include "Blueprint/UserWidget.h"

ANSReadyStartActor::ANSReadyStartActor()
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

void ANSReadyStartActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터에서 DetectionRadius를 바꾸면 스피어 반경도 즉시 반영
	if (DetectionCollision)
	{
		DetectionCollision->SetSphereRadius(DetectionRadius);
	}
}

bool ANSReadyStartActor::CanInteract_Implementation(APlayerController* Interactor) const
{
	return IsValid(Interactor) && ReadyStartWidgetClass;
}

bool ANSReadyStartActor::OnInteract_Implementation(APlayerController* Interactor)
{
	if (!IsValid(Interactor) || !ReadyStartWidgetClass)
	{
		return false;
	}

	// 같은 플레이어가 이미 위젯을 열어둔 상태라면,
	// F 입력을 토글처럼 처리해서 위젯을 닫는다.
	if (TObjectPtr<UNSReadyStartWidget>* FoundWidget = OpenedWidgetsByPlayer.Find(Interactor))
	{
		if (IsValid(*FoundWidget) && (*FoundWidget)->IsInViewport())
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

	ReadyStartWidget->OnWidgetClosed.AddUObject(
		this,
		&ANSReadyStartActor::HandleReadyStartWidgetClosed);

	OpenedWidgetsByPlayer.Add(Interactor, ReadyStartWidget);

	ReadyStartWidget->AddToViewport();

	// 첫 상호작용 → 안내 완료 처리 (이 함수는 상호작용한 플레이어의 클라에서 실행됨)
	if (UNSOutRunGuideSubsystem* GuideSubsystem =
		GetWorld()->GetSubsystem<UNSOutRunGuideSubsystem>())
	{
		GuideSubsystem->NotifyReadyConsoleUsed();
	}

	return true;
}

FText ANSReadyStartActor::GetPromptText_Implementation() const
{
	return PromptText;
}

FVector ANSReadyStartActor::GetPromptWorldLocation_Implementation() const
{
	if (PromptAnchor)
	{
		return PromptAnchor->GetComponentLocation();
	}

	return GetActorLocation();
}
void ANSReadyStartActor::CloseOpenedWidget(APlayerController* Interactor)
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

void ANSReadyStartActor::HandleReadyStartWidgetClosed(
	UNSReadyStartWidget* ClosedWidget,
	APlayerController* Interactor)
{
	if (!Interactor)
	{
		return;
	}

	const TObjectPtr<UNSReadyStartWidget>* FoundWidget =
		OpenedWidgetsByPlayer.Find(Interactor);

	if (FoundWidget && FoundWidget->Get() == ClosedWidget)
	{
		OpenedWidgetsByPlayer.Remove(Interactor);
	}
}
