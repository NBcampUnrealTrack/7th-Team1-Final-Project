// Copyright 2026 One Team. All rights reserved.

#include "NSEnemyPartComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/Combat/Weapon/NSEnemyWeaponBase.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "Net/UnrealNetwork.h"

UNSEnemyPartComponent::UNSEnemyPartComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNSEnemyPartComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNSEnemyPartComponent, CurrentWeapon);
	DOREPLIFETIME(UNSEnemyPartComponent, SpawnedParts);
}

void UNSEnemyPartComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UNSEnemyStateComponent* StateComponent = GetOwner()->FindComponentByClass<UNSEnemyStateComponent>())
	{
		StateComponent->OnDeathStarted.AddUObject(this, &ThisClass::HandleOwnerDeathStarted);
	}
}

void UNSEnemyPartComponent::EquipParts()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	UnEquipParts();

	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(Owner);
	if (!EnemyAgent)
	{
		return;
	}

	UNSEnemyData* EnemyData = EnemyAgent->GetEnemyData();
	if (!EnemyData)
	{
		return;
	}

	const TArray<const FNSEnemyPartRow*>& PartRows = EnemyData->GetPartRows();

	if (PartRows.IsEmpty())
	{
		SpawnFallbackWeapon(EnemyData);
		return;
	}

	for (const FNSEnemyPartRow* PartRow : PartRows)
	{
		if (!PartRow || !IsSpawnedPartType(*PartRow))
		{
			continue;
		}

		AActor* SpawnedActor = SpawnPartActor(*PartRow);
		if (!SpawnedActor)
		{
			continue;
		}

		AttachPartActor(SpawnedActor, *PartRow);

		FNSSpawnedEnemyPart SpawnedPart;
		SpawnedPart.PartId = PartRow->PartId;
		SpawnedPart.Actor = SpawnedActor;
		SpawnedParts.Add(SpawnedPart);

		if (!CurrentWeapon)
		{
			CurrentWeapon = Cast<ANSEnemyWeaponBase>(SpawnedActor);
		}
	}
}

void UNSEnemyPartComponent::UnEquipParts()
{
	for (const FNSSpawnedEnemyPart& SpawnedPart : SpawnedParts)
	{
		if (IsValid(SpawnedPart.Actor))
		{
			SpawnedPart.Actor->Destroy();
		}
	}

	SpawnedParts.Reset();
	CurrentWeapon = nullptr;
}

AActor* UNSEnemyPartComponent::FindSpawnedPartActor(FName PartId) const
{
	if (PartId.IsNone())
	{
		return nullptr;
	}

	for (const FNSSpawnedEnemyPart& SpawnedPart : SpawnedParts)
	{
		if (SpawnedPart.PartId == PartId && IsValid(SpawnedPart.Actor))
		{
			return SpawnedPart.Actor;
		}
	}

	return nullptr;
}

void UNSEnemyPartComponent::GetPartRowsByAttackId(
	FName AttackId,
	TArray<const FNSEnemyPartRow*>& OutPartRows) const
{
	OutPartRows.Reset();

	const AActor* Owner = GetOwner();
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(Owner);
	if (!EnemyAgent)
	{
		return;
	}

	if (const UNSEnemyData* EnemyData = EnemyAgent->GetEnemyData())
	{
		EnemyData->GetPartRowsByAttackId(AttackId, OutPartRows);
	}
}

void UNSEnemyPartComponent::HandleOwnerDeathStarted()
{
	for (const FNSSpawnedEnemyPart& SpawnedPart : SpawnedParts)
	{
		if (ANSEnemyWeaponBase* Weapon = Cast<ANSEnemyWeaponBase>(SpawnedPart.Actor))
		{
			Weapon->StartDissolve();
		}
	}
}

AActor* UNSEnemyPartComponent::SpawnPartActor(const FNSEnemyPartRow& PartRow)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();

	if (!Owner || !World || !PartRow.ActorClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.Instigator = Cast<APawn>(Owner);

	return World->SpawnActor<AActor>(
		PartRow.ActorClass,
		FTransform::Identity,
		SpawnParams);
}

void UNSEnemyPartComponent::SpawnFallbackWeapon(UNSEnemyData* EnemyData)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();

	if (!Owner || !World || !EnemyData || !EnemyData->DefaultWeaponClass)
	{
		return;
	}

	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(Owner);
	USkeletalMeshComponent* MeshComponent = EnemyAgent ? EnemyAgent->GetEnemyMesh() : nullptr;
	if (!MeshComponent)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.Instigator = Cast<APawn>(Owner);

	CurrentWeapon = World->SpawnActor<ANSEnemyWeaponBase>(
		EnemyData->DefaultWeaponClass,
		FTransform::Identity,
		SpawnParams);

	if (!CurrentWeapon)
	{
		return;
	}

	const FWeaponConfig& Config = CurrentWeapon->GetWeaponConfig();

	CurrentWeapon->AttachToComponent(
		MeshComponent,
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		Config.EquipSocketName);

	CurrentWeapon->SetActorRelativeTransform(Config.RelativeTransform);

	FNSSpawnedEnemyPart SpawnedPart;
	SpawnedPart.PartId = TEXT("DefaultWeapon");
	SpawnedPart.Actor = CurrentWeapon;
	SpawnedParts.Add(SpawnedPart);
}

void UNSEnemyPartComponent::AttachPartActor(AActor* PartActor, const FNSEnemyPartRow& PartRow)
{
	if (!PartActor)
	{
		return;
	}

	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(GetOwner());
	USkeletalMeshComponent* MeshComponent = EnemyAgent ? EnemyAgent->GetEnemyMesh() : nullptr;
	if (!MeshComponent)
	{
		return;
	}

	PartActor->AttachToComponent(
		MeshComponent,
		MakeAttachmentRules(PartRow),
		PartRow.AttachSocket);

	PartActor->SetActorRelativeTransform(PartRow.RelativeTransform);
}

bool UNSEnemyPartComponent::IsSpawnedPartType(const FNSEnemyPartRow& PartRow) const
{
	return PartRow.PartType == ENSEnemyPartType::SpawnedWeapon ||
		PartRow.PartType == ENSEnemyPartType::SpawnedPart;
}

FAttachmentTransformRules UNSEnemyPartComponent::MakeAttachmentRules(const FNSEnemyPartRow& PartRow) const
{
	switch (PartRow.AttachRule)
	{
	case ENSEnemyPartAttachRule::KeepRelativeTransform:
		return FAttachmentTransformRules::KeepRelativeTransform;

	case ENSEnemyPartAttachRule::KeepWorldTransform:
		return FAttachmentTransformRules::KeepWorldTransform;

	case ENSEnemyPartAttachRule::SnapToTargetNotIncludingScale:
		return FAttachmentTransformRules::SnapToTargetNotIncludingScale;

	case ENSEnemyPartAttachRule::SnapToTargetIncludingScale:
		return FAttachmentTransformRules::SnapToTargetIncludingScale;

	default:
		return FAttachmentTransformRules::KeepRelativeTransform;
	}
}
