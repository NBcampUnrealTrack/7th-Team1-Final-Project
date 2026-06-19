// // Copyright 2026 One Team. All rights reserved.
//
//
// #include "NSInteractableActor.h"
// #include "Components/SphereComponent.h"
// #include "NeoSanctum/Interaction/Component/NSInteractionComponent.h"
// #include "NeoSanctum/UI/CharacterSelect/NSCharacterSelectWidget.h"
//
// ANSInteractableActor::ANSInteractableActor()
// {
// 	PrimaryActorTick.bCanEverTick = false;
// 	
// 	//메시 컴포넌트 생성
// 	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
// 	SetRootComponent(MeshComponent);
// 	//상호작용 범위 컴포넌트 생성
// 	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
// 	SphereComponent->SetupAttachment(RootComponent);
// 	SphereComponent->SetSphereRadius(200.f);
// 	//상호작용 컴포넌트 생성
// 	InteractionComponent = CreateDefaultSubobject<UNSInteractionComponent>(TEXT("InteractionComponent"));
// 	//WidgetComponent 위치를 엑터 위쪽으로 설정
// 	if (InteractionComponent && InteractionComponent->PromptWidgetComponent)
// 	{
// 		InteractionComponent->PromptWidgetComponent->SetupAttachment(RootComponent);
// 		InteractionComponent->PromptWidgetComponent->SetRelativeLocation(FVector(0.f,0.f,100.f));
// 	}
//
// }
//
// void ANSInteractableActor::BeginPlay()
// {
// 	Super::BeginPlay();
// 	
// 	//상호작용 완료 델리게이트 바인딩
// 	if (InteractionComponent)
// 	{
// 		InteractionComponent->OnInteracted.AddDynamic(
// 			this,
// 			&ANSInteractableActor::HandleInteracted);
// 	}
// }
//
// void ANSInteractableActor::HandleInteracted()
// {
// 	APlayerController* PC = InteractionComponent->CachedInteractor.Get();
// 	OnInteract(PC);
// }
//
// void ANSInteractableActor::OnInteract_Implementation(APlayerController* Interactor)
// {
// 	if (!Interactor || !CharacterSelectWidgetClass)
// 	{
// 		return;
// 	}
//
// 	UNSCharacterSelectWidget* Widget =
// 		CreateWidget<UNSCharacterSelectWidget>(Interactor, CharacterSelectWidgetClass);
//
// 	if (!Widget)
// 	{
// 		return;
// 	}
//
// 	Widget->AddToViewport();
//
// 	Interactor->SetShowMouseCursor(true);
//
// 	FInputModeUIOnly InputMode;
// 	InputMode.SetWidgetToFocus(Widget->TakeWidget());
// 	Interactor->SetInputMode(InputMode);
// }
//
