// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/System/Minimap/NSMinimapIconComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/System/Minimap/NSMinimapSubsystem.h"

UNSMinimapIconComponent::UNSMinimapIconComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSMinimapIconComponent::SetIconRowName(FName NewIconRowName)
{
	IconRowName = NewIconRowName;
}

void UNSMinimapIconComponent::SetShowOnMinimap(bool bNewShowOnMinimap)
{
	bShowOnMinimap = bNewShowOnMinimap;
}

void UNSMinimapIconComponent::SetHideWhenOwnerHealthZero(bool bNewHideWhenOwnerHealthZero)
{
	bHideWhenOwnerHealthZero = bNewHideWhenOwnerHealthZero;
}

bool UNSMinimapIconComponent::ShouldShowOnMinimap() const
{
	if (!bShowOnMinimap || IconRowName.IsNone())
	{
		return false;
	}

	const AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return false;
	}

	if (!bHideWhenOwnerHealthZero)
	{
		return true;
	}

	const IAbilitySystemInterface* AbilitySystemOwner = Cast<IAbilitySystemInterface>(Owner);
	const UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemOwner
		? AbilitySystemOwner->GetAbilitySystemComponent()
		: nullptr;
	if (!AbilitySystemComponent)
	{
		return true;
	}

	return AbilitySystemComponent->GetNumericAttribute(UNSBaseAttributeSet::GetHealthAttribute()) > 0.0f;
}

void UNSMinimapIconComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UNSMinimapSubsystem* MinimapSubsystem = World->GetSubsystem<UNSMinimapSubsystem>())
		{
			MinimapSubsystem->RegisterIconComponent(this);
		}
	}
}

void UNSMinimapIconComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UNSMinimapSubsystem* MinimapSubsystem = World->GetSubsystem<UNSMinimapSubsystem>())
		{
			MinimapSubsystem->UnregisterIconComponent(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

FVector UNSMinimapIconComponent::GetIconWorldLocation() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorLocation() + WorldLocationOffset : WorldLocationOffset;
}
