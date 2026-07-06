// Copyright 2026 One Team. All rights reserved.


#include "NSBarrier.h"

#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NeoSanctum/Collision/NSCollisionProfiles.h"
#include "NeoSanctum/Combat/HitReaction/NSHitReactionComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/System/Component/NSDamageFlashComponent.h"

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
	BarrierCollisionComponent->SetCollisionProfileName(NSCollisionProfiles::PlayerBarrier);
	BarrierCollisionComponent->SetGenerateOverlapEvents(false);

	BarrierFlashMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrierFlashMeshComponent"));
	BarrierFlashMeshComponent->SetupAttachment(BarrierCollisionComponent);
	BarrierFlashMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BarrierFlashMeshComponent->SetGenerateOverlapEvents(false);
	BarrierFlashMeshComponent->SetCastShadow(false);

	HitReactionComponent = CreateDefaultSubobject<UNSHitReactionComponent>(TEXT("HitReactionComponent"));
	HitReactionComponent->SetTargetType(ENSHitFeedbackTargetType::Barrier);

	DamageFlashComponent = CreateDefaultSubobject<UNSDamageFlashComponent>(TEXT("DamageFlashComponent"));
}

UAbilitySystemComponent* ANSBarrier::GetAbilitySystemComponent() const
{
	return ASC;
}

void ANSBarrier::InitializeBarrier(
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

void ANSBarrier::BeginPlay()
{
	Super::BeginPlay();

	InitializeAbilityActorInfo();
	ApplyRadius(CurrentRadius);
	ApplyDuration(CurrentDuration);
	ApplyInitialAttributeEffect();
}

void ANSBarrier::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DurationTimerHandle);

	Super::EndPlay(EndPlayReason);
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

void ANSBarrier::ApplyDuration(float InDuration)
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

void ANSBarrier::HandleOutOfHealth()
{
	// 연출이 들어오기 전까지는 즉시 파괴함.
	if (HasAuthority())
	{
		DestroyBarrier();
	}
}

void ANSBarrier::DestroyBarrier()
{
	if (HasAuthority())
	{
		Destroy();
	}
}
