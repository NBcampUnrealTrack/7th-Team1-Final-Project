// Copyright 2026 One Team. All rights reserved.

#include "NSBarrierBase.h"

#include "AbilitySystemComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NeoSanctum/Collision/NSCollisionProfiles.h"
#include "NeoSanctum/Combat/HitReaction/NSHitReactionComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/System/Component/NSDamageFlashComponent.h"
#include "Net/UnrealNetwork.h"

ANSBarrierBase::ANSBarrierBase()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(false);

	ASC = CreateDefaultSubobject<UNSAbilitySystemComponent>(TEXT("ASC"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UNSBaseAttributeSet>(TEXT("AttributeSet"));

	CurrentRadius = DefaultRadius;

	BarrierFlashMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrierFlashMeshComponent"));
	BarrierFlashMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BarrierFlashMeshComponent->SetGenerateOverlapEvents(false);
	BarrierFlashMeshComponent->SetCastShadow(false);

	HitReactionComponent = CreateDefaultSubobject<UNSHitReactionComponent>(TEXT("HitReactionComponent"));
	HitReactionComponent->SetTargetType(ENSHitFeedbackTargetType::Barrier);

	DamageFlashComponent = CreateDefaultSubobject<UNSDamageFlashComponent>(TEXT("DamageFlashComponent"));
}

UAbilitySystemComponent* ANSBarrierBase::GetAbilitySystemComponent() const
{
	return ASC;
}

void ANSBarrierBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANSBarrierBase, CurrentRadius);
}

void ANSBarrierBase::InitializeBarrier(
	APawn* InOwningPawn,
	AController* InOwningController,
	float InRadius,
	float InDuration,
	TSubclassOf<UGameplayEffect> InInitialAttributeEffectClass,
	const TArray<FNSSetByCallerMagnitude>& InSetByCallerMagnitudes)
{
	OwningPawn = InOwningPawn;
	OwningController = InOwningController;
	CurrentDuration = InDuration;
	InitialAttributeEffectClass = InInitialAttributeEffectClass;
	SetByCallerMagnitudes = InSetByCallerMagnitudes;

	if (OwningPawn)
	{
		SetOwner(OwningPawn);
		SetInstigator(OwningPawn);
	}

	ApplyRadius(InRadius);
	InitializeAbilityActorInfo();

	if (HasActorBegunPlay())
	{
		ApplyDuration(CurrentDuration);
		ApplyInitialAttributeEffect();
	}
}

float ANSBarrierBase::GetCurrentHealth() const
{
	return AttributeSet ? AttributeSet->GetHealth() : 0.0f;
}

void ANSBarrierBase::BeginPlay()
{
	Super::BeginPlay();

	InitializeAbilityActorInfo();
	ApplyRadius(CurrentRadius);
	ApplyDuration(CurrentDuration);
	ApplyInitialAttributeEffect();
}

void ANSBarrierBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DurationTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void ANSBarrierBase::OnRep_CurrentRadius()
{
	ApplyCollisionRadius(CurrentRadius);
	ApplyVisualRadius(CurrentRadius);
}

void ANSBarrierBase::InitializeBarrierCollisionComponent(UShapeComponent* InBarrierCollisionComponent)
{
	BarrierCollisionComponent = InBarrierCollisionComponent;
	SetRootComponent(BarrierCollisionComponent);

	if (BarrierCollisionComponent)
	{
		BarrierCollisionComponent->SetCollisionProfileName(NSCollisionProfiles::PlayerBarrier);
		BarrierCollisionComponent->SetGenerateOverlapEvents(false);
	}

	if (BarrierFlashMeshComponent)
	{
		BarrierFlashMeshComponent->SetupAttachment(BarrierCollisionComponent);
	}
}

void ANSBarrierBase::InitializeAbilityActorInfo()
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

void ANSBarrierBase::ApplyInitialAttributeEffect()
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

void ANSBarrierBase::ApplyRadius(float InRadius)
{
	const float Radius = FMath::Max(InRadius, MinimumRadius);
	CurrentRadius = Radius;

	ApplyCollisionRadius(Radius);
	ApplyVisualRadius(Radius);
}

void ANSBarrierBase::ApplyCollisionRadius(float Radius)
{
}

void ANSBarrierBase::ApplyVisualRadius(float Radius)
{
	if (BarrierFlashMeshComponent)
	{
		float BaseVisualRadius = DefaultRadius;
		if (const UStaticMesh* FlashMesh = BarrierFlashMeshComponent->GetStaticMesh())
		{
			const FVector MeshExtent = FlashMesh->GetBounds().BoxExtent;
			BaseVisualRadius = FMath::Max3(MeshExtent.X, MeshExtent.Y, MeshExtent.Z);
		}

		const float VisualScale = Radius / FMath::Max(BaseVisualRadius, KINDA_SMALL_NUMBER);
		BarrierFlashMeshComponent->SetRelativeScale3D(FVector(VisualScale));
	}
}

void ANSBarrierBase::ApplyDuration(float InDuration)
{
	GetWorldTimerManager().ClearTimer(DurationTimerHandle);
	CurrentDuration = FMath::Max(InDuration, 0.0f);

	if (!HasAuthority() || CurrentDuration <= 0.0f)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		DurationTimerHandle,
		this,
		&ThisClass::DestroyBarrier,
		CurrentDuration,
		false
	);
}

void ANSBarrierBase::HandleOutOfHealth()
{
	// 연출이 들어오기 전까지는 즉시 파괴함.
	if (HasAuthority())
	{
		DestroyBarrier();
	}
}

void ANSBarrierBase::DestroyBarrier()
{
	if (HasAuthority())
	{
		Destroy();
	}
}
