// Copyright 2026 One Team. All rights reserved.


#include "NSInteractionComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"
#include "../../UI/Interaction/NSInteractionPromptWidget.h"



UNSInteractionComponent::UNSInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	PromptWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PromptWidgetComponent"));
	PromptWidgetComponent->SetVisibility(false);
}

void UNSInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	//WidgetComponent에 프롬프트 위젯 클래스 설정
	if (PromptWidgetComponent && InteractionPromptWidgetClass)
	{
		PromptWidgetComponent->SetWidgetClass(InteractionPromptWidgetClass);
		PromptWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		PromptWidgetComponent->SetDrawSize(FVector2D(200.0f,100.f));
	}

	// 오너 액터에서 SphereComponent 찾아서 바인딩
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	SphereComponent = Owner->FindComponentByClass<USphereComponent>();
	if (!SphereComponent) { return; }

	SphereComponent->OnComponentBeginOverlap.AddDynamic(
		this,
		&UNSInteractionComponent::OnSphereBeginOverlap);
	SphereComponent->OnComponentEndOverlap.AddDynamic(
		this,
		&UNSInteractionComponent::OnSphereEndOverlap);
}

void UNSInteractionComponent::Interact(APlayerController* Interactor)
{
	if (!CanInteract()) { return; }

	UE_LOG(LogTemp, Warning, TEXT("OnInteracted 브로드캐스트 직전, 바인딩 수: %d"), OnInteracted.IsBound() ? 1 : 0);
	OnInteracted.Broadcast();
}

bool UNSInteractionComponent::CanInteract() const
{
	UE_LOG(LogTemp, Warning, TEXT("CanInteract: %d"), bCanInteract);
	return bCanInteract;
}

void UNSInteractionComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APlayerController* PC = Cast<APlayerController>(OtherActor->GetInstigatorController());
	if (!PC) { return; }

	//범위 진입시 감지
	CachedInteractor = PC;
	ShowPrompt(PC);
}

void UNSInteractionComponent::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerController* PC = Cast<APlayerController>(OtherActor->GetInstigatorController());
	if (!PC) { return; }

	//범위 이탈시 감지 제거
	CachedInteractor = nullptr;
	HidePrompt(PC);
}

void UNSInteractionComponent::ShowPrompt(APlayerController* PC)
{
	if (!PromptWidgetComponent)
	{
		return;
	}
	UNSInteractionPromptWidget* Widget = Cast<UNSInteractionPromptWidget>(PromptWidgetComponent->GetUserWidgetObject());
	if (Widget)
	{
		Widget->SetPromptText(InteractionKeyText, InteractionPromptText);
	}
	PromptWidgetComponent->SetVisibility(true);
}

void UNSInteractionComponent::HidePrompt(APlayerController* PC)
{
	if (!PromptWidgetComponent)
	{
		return;
	}
	PromptWidgetComponent->SetVisibility(false);
}
void UNSInteractionComponent::TickComponent
(float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
	)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PromptWidgetComponent || !PromptWidgetComponent->IsVisible()) { return; }

	// 카메라 방향으로 회전
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager) { return; }

	FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
	FVector WidgetLocation = PromptWidgetComponent->GetComponentLocation();
	FRotator LookAtRotation = FRotationMatrix::MakeFromX(CameraLocation - WidgetLocation).Rotator();
	PromptWidgetComponent->SetWorldRotation(LookAtRotation);
}

