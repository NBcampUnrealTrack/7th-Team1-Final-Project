// Copyright 2026 One Team. All rights reserved.


#include "NSBarrier.h"

#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NiagaraComponent.h"

ANSBarrier::ANSBarrier()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(false);

	ASC = CreateDefaultSubobject<UNSAbilitySystemComponent>(TEXT("ASC"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UNSBaseAttributeSet>(TEXT("AttributeSet"));

	BarrierCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("BarrierCollisionComponent"));
	SetRootComponent(BarrierCollisionComponent);
	CurrentRadius = DefaultRadius;
	BarrierCollisionComponent->InitSphereRadius(DefaultRadius);
	BarrierCollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BarrierCollisionComponent->SetGenerateOverlapEvents(false);

	BarrierNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BarrierNiagaraComponent"));
	BarrierNiagaraComponent->SetupAttachment(BarrierCollisionComponent);
	BarrierNiagaraComponent->SetAutoActivate(true);
}

UAbilitySystemComponent* ANSBarrier::GetAbilitySystemComponent() const
{
	return ASC;
}

void ANSBarrier::InitializeBarrier(
	APawn* InOwningPawn,
	AController* InOwningController,
	float InRadius,
	const TArray<FNSSetByCallerMagnitude>& InSetByCallerMagnitudes)
{
	OwningPawn = InOwningPawn;
	OwningController = InOwningController;
	SetByCallerMagnitudes = InSetByCallerMagnitudes;

	if (OwningPawn)
	{
		SetOwner(OwningPawn);
		SetInstigator(OwningPawn);
	}

	ApplyRadius(InRadius);
	InitializeAbilityActorInfo();
	ApplyInitialAttributeEffect();
}

void ANSBarrier::BeginPlay()
{
	Super::BeginPlay();

	InitializeAbilityActorInfo();
	ApplyRadius(CurrentRadius);
	ApplyInitialAttributeEffect();
}

void ANSBarrier::InitializeAbilityActorInfo()
{
	if (!ASC || bAbilityActorInfoInitialized)
	{
		return;
	}

	ASC->InitAbilityActorInfo(this, this);

	if (AttributeSet)
	{
		AttributeSet->OnOutOfHealth.AddUObject(this, &ThisClass::HandleOutOfHealth);
	}

	bAbilityActorInfoInitialized = true;
}

void ANSBarrier::ApplyInitialAttributeEffect()
{
	if (!HasAuthority() || !ASC || !InitialAttributeEffectClass || bInitialAttributeEffectApplied)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		InitialAttributeEffectClass,
		1.0f,
		EffectContext
	);

	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return;
	}

	for (const FNSSetByCallerMagnitude& SetByCallerMagnitude : SetByCallerMagnitudes)
	{
		if (!SetByCallerMagnitude.SetByCallerTag.IsValid())
		{
			continue;
		}

		SpecHandle.Data->SetSetByCallerMagnitude(
			SetByCallerMagnitude.SetByCallerTag,
			SetByCallerMagnitude.Magnitude
		);
	}

	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	bInitialAttributeEffectApplied = true;
}

void ANSBarrier::ApplyRadius(float InRadius)
{
	const float Radius = FMath::Max(InRadius, MinimumRadius);
	CurrentRadius = Radius;

	if (BarrierCollisionComponent)
	{
		BarrierCollisionComponent->SetSphereRadius(Radius);
	}

	if (BarrierNiagaraComponent)
	{
		const float VisualScale = Radius / DefaultRadius;
		BarrierNiagaraComponent->SetRelativeScale3D(FVector(VisualScale));
		BarrierNiagaraComponent->SetVariableFloat(TEXT("User.BarrierRadius"), Radius);
	}
}

void ANSBarrier::HandleOutOfHealth()
{
	// 연출이 들어오기 전까지는 즉시 파괴함.
	if (HasAuthority())
	{
		Destroy();
	}
}
