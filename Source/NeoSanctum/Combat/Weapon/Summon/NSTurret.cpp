// Copyright 2026 One Team. All rights reserved.

#include "NSTurret.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SphereComponent.h"
#include "GenericTeamAgentInterface.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "NeoSanctum/Type/NSTeamTypes.h"

ANSTurret::ANSTurret()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BaseMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMeshComponent"));
	BaseMeshComponent->SetupAttachment(SceneRoot);

	HeadPivotComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HeadPivotComponent"));
	HeadPivotComponent->SetupAttachment(BaseMeshComponent);

	HeadMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMeshComponent"));
	HeadMeshComponent->SetupAttachment(HeadPivotComponent);

	DetectionSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphereComponent"));
	DetectionSphereComponent->SetupAttachment(SceneRoot);
	DetectionSphereComponent->InitSphereRadius(DetectionRadius);
	DetectionSphereComponent->SetGenerateOverlapEvents(true);
}

void ANSTurret::BeginPlay()
{
	Super::BeginPlay();

	if (DetectionSphereComponent)
	{
		DetectionSphereComponent->SetSphereRadius(DetectionRadius);
		DetectionSphereComponent->OnComponentBeginOverlap.AddDynamic(
			this,
			&ThisClass::OnDetectionSphereBeginOverlap
		);
		DetectionSphereComponent->OnComponentEndOverlap.AddDynamic(
			this,
			&ThisClass::OnDetectionSphereEndOverlap
		);

		InitializeTargets();
	}
}

void ANSTurret::OnDetectionSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (IsValidTargetActor(OtherActor))
	{
		TargetSet.Add(OtherActor);
	}
}

void ANSTurret::OnDetectionSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		TargetSet.Remove(OtherActor);
	}
}

bool ANSTurret::IsValidTargetActor(const AActor* TargetActor) const
{
	if (!IsValid(TargetActor) || TargetActor == this)
	{
		return false;
	}

	const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(TargetActor);
	if (!TeamAgent || TeamAgent->GetGenericTeamId() != FGenericTeamId(static_cast<uint8>(ETeamId::Enemy)))
	{
		return false;
	}

	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(TargetActor);
	const UAbilitySystemComponent* TargetASC =
		AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;

	if (!TargetASC)
	{
		return false;
	}

	if (TargetASC->HasMatchingGameplayTag(NSGameplayTags::State_Dead))
	{
		return false;
	}

	return true;
}

void ANSTurret::InitializeTargets()
{
	if (!DetectionSphereComponent)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	DetectionSphereComponent->GetOverlappingActors(OverlappingActors);

	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (IsValidTargetActor(OverlappingActor))
		{
			TargetSet.Add(OverlappingActor);
		}
	}
}
