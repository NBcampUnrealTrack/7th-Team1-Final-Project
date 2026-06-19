// Copyright 2026 One Team. All rights reserved.


#include "NSInteractionComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"
#include "../../UI/Interaction/NSInteractionPromptWidget.h"
#include "Components/WidgetComponent.h"
#include "NeoSanctum/Interaction/Core/NSInteractable.h"

UNSInteractionComponent::UNSInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetSphereRadius(DetectionRadius);
	DetectionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	
	PromptWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PromptWidgetComponent"));
	PromptWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	PromptWidgetComponent->SetDrawSize(FVector2D(200.f, 100.f));
	PromptWidgetComponent->SetVisibility(false);
}

void UNSInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	
	DetectionSphere->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	DetectionSphere->SetSphereRadius(DetectionRadius);
	PromptWidgetComponent->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	
	if (PromptWidgetClass)
	{
		PromptWidgetComponent->SetWidgetClass(PromptWidgetClass);
	}
	
	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &UNSInteractionComponent::OnSphereBeginOverlap);
	DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &UNSInteractionComponent::OnSphereEndOverlap);
}

void UNSInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateActiveTarget();
}

void UNSInteractionComponent::UpdateActiveTarget()
{
	APlayerController* PC = GetOwnerController();
	AActor* Owner = GetOwner();
	if (!PC || !Owner)
	{
		HidePrompt();
		return;
	}
	const FVector OwnerLocation = Owner->GetActorLocation();
	AActor* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();
	
	// Remove를 해주기 위해서 역순으로 돔
	for (int32 i = Candidates.Num() - 1; i >= 0; --i)
	{
		AActor* Candidate = Candidates[i].Get();
		if (!Candidate)
		{
			// RemoveAt으로 돔
			Candidates.RemoveAt(i);
			continue;
		}
		if (!INSInteractable::Execute_CanInteract(Candidate, PC))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(OwnerLocation, Candidate->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Candidate;
		}
	}
	
	if (!Nearest)
	{
		HidePrompt();
		ActiveTarget = nullptr;
		return;
	}
	
	ActiveTarget = Nearest;
	ShowPromptFor(Nearest);
}

void UNSInteractionComponent::TryInteract()
{
	APlayerController* PC = GetOwnerController();
	if (!PC || !ActiveTarget)
	{
		return;
	}
	if (!INSInteractable::Execute_CanInteract(ActiveTarget, PC))
	{
		return;
	}
	INSInteractable::Execute_OnInteract(ActiveTarget, PC);
}

void UNSInteractionComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}
	if (!OtherActor->Implements<UNSInteractable>())
	{
		return;
	}
	
	APlayerController* PC = GetOwnerController();
	if (!PC)
	{
		return;
	}
	
	// 상호작용 불가시 후보에서 제외
	if (!INSInteractable::Execute_CanInteract(OtherActor, PC))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Interactor] 상호작용 불가 대상 감지 : %s"), *OtherActor->GetName());
		return;
	}
	
	Candidates.AddUnique(OtherActor);
}

void UNSInteractionComponent::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}
	Candidates.Remove(OtherActor);
}

void UNSInteractionComponent::ShowPromptFor(AActor* Target)
{
	if (!Target || !PromptWidgetComponent)
	{
		return;
	}
	
	const FVector TargetLocation = Target->GetActorLocation() + FVector(0.f, 0.f, PromptHeightOffset);
	PromptWidgetComponent->SetWorldLocation(TargetLocation);
	
	UNSInteractionPromptWidget* Widget = Cast<UNSInteractionPromptWidget>(PromptWidgetComponent->GetUserWidgetObject());
	if (Widget)
	{
		Widget->SetPromptText(InteractionKeyText, INSInteractable::Execute_GetPromptText(Target));
	}
	PromptWidgetComponent->SetVisibility(true);
}

void UNSInteractionComponent::HidePrompt()
{
	if (!PromptWidgetComponent)
	{
		return;
	}
	PromptWidgetComponent->SetVisibility(false);
}

APlayerController* UNSInteractionComponent::GetOwnerController() const
{
	 const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return nullptr;
	}
	return Cast<APlayerController>(OwnerPawn->GetController());
}
