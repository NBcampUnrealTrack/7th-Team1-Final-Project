// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyDrone.h"

#include "NeoSanctum/AI/Components/NSFlyingLocomotionComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"

ANSEnemyDrone::ANSEnemyDrone()
{
	PrimaryActorTick.bCanEverTick = false;
	FlyingLocomotionComponent = CreateDefaultSubobject<UNSFlyingLocomotionComponent>("MovementComponent");
}

void ANSEnemyDrone::BeginPlay()
{
	Super::BeginPlay();
	
}

bool ANSEnemyDrone::IsInPool() const
{
	const UNSEnemyStateComponent* MotherShipStateComponent = GetStateComponent();
	return MotherShipStateComponent && MotherShipStateComponent->IsInactive();
}

void ANSEnemyDrone::PrepareForReuse(const FVector& Location, const FRotator& Rotation)
{
	if (!HasAuthority()) return;
	
	UNSEnemyStateComponent* MotherShipStateComponent = GetStateComponent();
	if (!MotherShipStateComponent) return;
	MotherShipStateComponent->ResetForReuse();
	ClearCurrentAttackRow();
	
	SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorTickEnabled(true); SetActorEnableCollision(true);
	
	if (FlyingLocomotionComponent)
	{
		FlyingLocomotionComponent->SetComponentTickEnabled(true);
	}
	
	ApplyAliveState();
	InitializeFromData(true);
	SpawnDefaultController();
}

void ANSEnemyDrone::DeactivateForPool()
{
	if (!HasAuthority()) return;
	
	
	if (UNSEnemyStateComponent* MotherShipStateComponent = GetStateComponent())
	{
		MotherShipStateComponent->SetInactive(true);
		MotherShipStateComponent->FinishHitReaction();
		MotherShipStateComponent->ResetHitGauge();
	}
	
	ClearCurrentAttackRow();
	
	if (FlyingLocomotionComponent)
	{
		FlyingLocomotionComponent->StopMovementImmediately();
		FlyingLocomotionComponent->SetComponentTickEnabled(false);
	}
	
	if(AAIController* DroneAIC = Cast<AAIController>(GetController()))
	{
		DroneAIC->UnPossess();
		DroneAIC->Destroy();
	}
	
	
	if (UAbilitySystemComponent* MotherShipASC = GetAbilitySystemComponent())
	{
		MotherShipASC->CancelAllAbilities();
		MotherShipASC->RemoveActiveEffects(FGameplayEffectQuery());
		MotherShipASC->ClearAllAbilities();
	}
	
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

