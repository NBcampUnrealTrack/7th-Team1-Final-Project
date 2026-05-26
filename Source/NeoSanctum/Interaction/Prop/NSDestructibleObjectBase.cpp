// Copyright 2026 One Team. All rights reserved.


#include "NSDestructibleObjectBase.h"
#include "AbilitySystemComponent.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSDestructibleAttributeSet.h"


ANSDestructibleObjectBase::ANSDestructibleObjectBase()
{
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	DestructibleAttrSet = CreateDefaultSubobject<UNSDestructibleAttributeSet>(TEXT("DestructibleAttributeSet"));

	GCComp = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryCollection"));
	RootComponent = GCComp;
}

void ANSDestructibleObjectBase::BeginPlay()
{
	Super::BeginPlay();
	
	LastHitLocation = GetActorLocation();

	ASC->InitAbilityActorInfo(this, this);
	DestructibleAttrSet->InitHealth(MaxHealth);
	DestructibleAttrSet->InitMaxHealth(MaxHealth);

	ASC->GetGameplayAttributeValueChangeDelegate(
		UNSDestructibleAttributeSet::GetHealthAttribute())
		.AddUObject(this, &ANSDestructibleObjectBase::OnHealthChanged);

	ASC->OnGameplayEffectAppliedDelegateToSelf.AddUObject(
		this, &ANSDestructibleObjectBase::OnGEApplied);
}

UAbilitySystemComponent* ANSDestructibleObjectBase::GetAbilitySystemComponent() const
{
	return ASC;
}

void ANSDestructibleObjectBase::TriggerDestruction()
{
	if (!HasAuthority()) return;

	Multicast_PlayDestruction(LastHitLocation);
}

void ANSDestructibleObjectBase::Multicast_PlayDestruction_Implementation(FVector HitLocation)
{
	GCComp->SetCollisionProfileName(TEXT("DestructedFragment"));

	GCComp->SetSimulatePhysics(true);

	FVector ImpulseDir = (GetActorLocation() - HitLocation).GetSafeNormal();
	GCComp->AddImpulseAtLocation(ImpulseDir * ImpulseStrength, HitLocation);
}

void ANSDestructibleObjectBase::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!bIsDestroyed && Data.NewValue <= 0.f)
	{
		bIsDestroyed = true;
		TriggerDestruction();
	}
}

void ANSDestructibleObjectBase::OnGEApplied(UAbilitySystemComponent* Source, const FGameplayEffectSpec& Spec,
	FActiveGameplayEffectHandle Handle)
{
	if (const FHitResult* Hit = Spec.GetContext().GetHitResult())
	{
		LastHitLocation = Hit->ImpactPoint;
	}
}
