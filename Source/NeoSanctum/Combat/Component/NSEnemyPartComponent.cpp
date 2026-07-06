// Copyright 2026 One Team. All rights reserved.

#include "NSEnemyPartComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
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

	DOREPLIFETIME(UNSEnemyPartComponent, SpawnedParts);
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
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("EnemyPartComponent: No PartRows found. EnemyId=%s"),
			*EnemyData->EnemyId.ToString());

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

void UNSEnemyPartComponent::GetMuzzleTransformsByAttackId(
	FName AttackId,
	TArray<FTransform>& OutTransforms) const
{
	OutTransforms.Reset();

	if (AttackId.IsNone())
	{
		return;
	}

	TArray<const FNSEnemyPartRow*> PartRows;
	GetPartRowsByAttackId(AttackId, PartRows);

	for (const FNSEnemyPartRow* PartRow : PartRows)
	{
		if (!PartRow)
		{
			continue;
		}

		FTransform MuzzleTransform;
		if (TryGetMuzzleTransformFromPartRow(*PartRow, MuzzleTransform))
		{
			OutTransforms.Add(MuzzleTransform);
		}
	}
}

void UNSEnemyPartComponent::GetTraceSegmentsByAttackId(
	FName AttackId,
	float FallbackDistance,
	const FVector& FallbackDirection,
	TArray<FNSEnemyPartTraceSegment>& OutSegments) const
{
	OutSegments.Reset();

	if (AttackId.IsNone())
	{
		return;
	}

	TArray<const FNSEnemyPartRow*> PartRows;
	GetPartRowsByAttackId(AttackId, PartRows);

	for (const FNSEnemyPartRow* PartRow : PartRows)
	{
		if (!PartRow)
		{
			continue;
		}

		FVector Start;
		FVector End;

		if (TryGetTraceSegmentFromPartRow(
			*PartRow,
			FallbackDistance,
			FallbackDirection,
			Start,
			End))
		{
			FNSEnemyPartTraceSegment Segment;
			Segment.Start = Start;
			Segment.End = End;
			OutSegments.Add(Segment);
		}
	}
}

void UNSEnemyPartComponent::GetPartRowsByAttackIdAndAimRole(
	FName AttackId,
	ENSEnemyPartAimRole AimRole,
	TArray<const FNSEnemyPartRow*>& OutPartRows) const
{
	OutPartRows.Reset();

	if (AttackId.IsNone() || AimRole == ENSEnemyPartAimRole::None)
	{
		return;
	}

	TArray<const FNSEnemyPartRow*> PartRows;
	GetPartRowsByAttackId(AttackId, PartRows);

	for (const FNSEnemyPartRow* PartRow : PartRows)
	{
		if (PartRow && PartRow->AimRole == AimRole)
		{
			OutPartRows.Add(PartRow);
		}
	}
}

bool UNSEnemyPartComponent::TryGetAimLimitsByAttackId(
	FName AttackId,
	ENSEnemyPartAimRole AimRole,
	float& OutYawLimit,
	float& OutPitchLimit,
	float& OutAimSpeed) const
{
	OutYawLimit = 0.0f;
	OutPitchLimit = 0.0f;
	OutAimSpeed = 0.0f;

	TArray<const FNSEnemyPartRow*> PartRows;
	GetPartRowsByAttackIdAndAimRole(AttackId, AimRole, PartRows);

	bool bHasAimData = false;

	for (const FNSEnemyPartRow* PartRow : PartRows)
	{
		if (!PartRow)
		{
			continue;
		}

		const bool bHasAimTarget =
			!PartRow->AimBone.IsNone() ||
			!PartRow->AimControl.IsNone();

		if (!bHasAimTarget)
		{
			continue;
		}

		if (PartRow->YawLimit > 0.0f)
		{
			OutYawLimit = OutYawLimit > 0.0f
				              ? FMath::Min(OutYawLimit, PartRow->YawLimit)
				              : PartRow->YawLimit;
		}

		if (PartRow->PitchLimit > 0.0f)
		{
			OutPitchLimit = OutPitchLimit > 0.0f
				                ? FMath::Min(OutPitchLimit, PartRow->PitchLimit)
				                : PartRow->PitchLimit;
		}

		if (OutAimSpeed <= 0.0f && PartRow->AimSpeed > 0.0f)
		{
			OutAimSpeed = PartRow->AimSpeed;
		}

		bHasAimData = true;
	}

	return bHasAimData;
}

bool UNSEnemyPartComponent::TryGetAnyMuzzleTransform(FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;

	const AActor* Owner = GetOwner();
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(Owner);
	const UNSEnemyData* EnemyData = EnemyAgent ? EnemyAgent->GetEnemyData() : nullptr;

	if (EnemyData)
	{
		for (const FNSEnemyPartRow* PartRow : EnemyData->GetPartRows())
		{
			if (PartRow && TryGetMuzzleTransformFromPartRow(*PartRow, OutTransform))
			{
				return true;
			}
		}
	}

	return false;
}

bool UNSEnemyPartComponent::TryGetMuzzleTransformByAttackId(
	FName AttackId,
	FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;

	if (AttackId.IsNone())
	{
		return false;
	}

	TArray<const FNSEnemyPartRow*> PartRows;
	GetPartRowsByAttackId(AttackId, PartRows);

	for (const FNSEnemyPartRow* PartRow : PartRows)
	{
		if (PartRow && TryGetMuzzleTransformFromPartRow(*PartRow, OutTransform))
		{
			return true;
		}
	}

	return false;
}

void UNSEnemyPartComponent::GetMuzzleTransformsByAttackId(FName AttackId, TArray<FTransform>& OutTransforms) const
{
	OutTransforms.Reset();

	if (AttackId.IsNone())
	{
		return;
	}

	TArray<const FNSEnemyPartRow*> PartRows;
	GetPartRowsByAttackId(AttackId, PartRows);

	for (const FNSEnemyPartRow* PartRow : PartRows)
	{
		FTransform MuzzleTransform;
		if (PartRow && TryGetMuzzleTransformFromPartRow(*PartRow, MuzzleTransform))
		{
			OutTransforms.Add(MuzzleTransform);
		}
	}
}

bool UNSEnemyPartComponent::TryGetTraceSegmentByAttackId(
	FName AttackId,
	float FallbackDistance,
	const FVector& FallbackDirection,
	FVector& OutStart,
	FVector& OutEnd) const
{
	OutStart = FVector::ZeroVector;
	OutEnd = FVector::ZeroVector;

	if (AttackId.IsNone())
	{
		return false;
	}

	TArray<const FNSEnemyPartRow*> PartRows;
	GetPartRowsByAttackId(AttackId, PartRows);

	for (const FNSEnemyPartRow* PartRow : PartRows)
	{
		if (PartRow &&
			TryGetTraceSegmentFromPartRow(
				*PartRow,
				FallbackDistance,
				FallbackDirection,
				OutStart,
				OutEnd))
		{
			return true;
		}
	}

	return false;
}

bool UNSEnemyPartComponent::TryGetLeftHandIKTransform(FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;

	const AActor* Owner = GetOwner();
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(Owner);
	const UNSEnemyData* EnemyData = EnemyAgent ? EnemyAgent->GetEnemyData() : nullptr;

	if (EnemyData)
	{
		for (const FNSEnemyPartRow* PartRow : EnemyData->GetPartRows())
		{
			if (PartRow && TryGetLeftHandIKTransformFromPartRow(*PartRow, OutTransform))
			{
				return true;
			}
		}
	}

	return false;
}

bool UNSEnemyPartComponent::ShouldUseLeftHandIKWhileEquipped() const
{
	const AActor* Owner = GetOwner();
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(Owner);
	const UNSEnemyData* EnemyData = EnemyAgent ? EnemyAgent->GetEnemyData() : nullptr;

	if (EnemyData)
	{
		for (const FNSEnemyPartRow* PartRow : EnemyData->GetPartRows())
		{
			if (PartRow && PartRow->bUseLeftHandIKWhileEquipped)
			{
				return true;
			}
		}
	}

	return false;
}

bool UNSEnemyPartComponent::TryGetAimMuzzleTransform(FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;

	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(GetOwner());
	const FNSEnemyAttackRow* CurrentAttackRow =
		EnemyAgent ? EnemyAgent->GetCurrentAttackRow() : nullptr;

	if (CurrentAttackRow &&
		TryGetMuzzleTransformByAttackId(CurrentAttackRow->AttackId, OutTransform))
	{
		return true;
	}

	return TryGetAnyMuzzleTransform(OutTransform);
}

void UNSEnemyPartComponent::GetSpawnedPartActorsByAttackId(
	FName AttackId,
	TArray<AActor*>& OutActors) const
{
	OutActors.Reset();

	if (AttackId.IsNone())
	{
		return;
	}

	TArray<const FNSEnemyPartRow*> PartRows;
	GetPartRowsByAttackId(AttackId, PartRows);

	for (const FNSEnemyPartRow* PartRow : PartRows)
	{
		if (!PartRow)
		{
			continue;
		}

		if (AActor* SpawnedActor = FindSpawnedPartActor(PartRow->PartId))
		{
			OutActors.AddUnique(SpawnedActor);
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

bool UNSEnemyPartComponent::TryGetMuzzleTransformFromPartRow(
	const FNSEnemyPartRow& PartRow,
	FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;

	if (PartRow.MuzzleSocket.IsNone())
	{
		return false;
	}

	return TryGetSocketTransformFromPartRow(PartRow, PartRow.MuzzleSocket, OutTransform);
}

bool UNSEnemyPartComponent::TryGetSocketTransformFromActor(
	AActor* Actor,
	FName SocketName,
	FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;

	if (!IsValid(Actor) || SocketName.IsNone())
	{
		return false;
	}

	TArray<USceneComponent*> SceneComponents;
	Actor->GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (IsValid(SceneComponent) && SceneComponent->DoesSocketExist(SocketName))
		{
			OutTransform = SceneComponent->GetSocketTransform(SocketName, RTS_World);
			return true;
		}
	}

	return false;
}

bool UNSEnemyPartComponent::TryGetSocketTransformFromOwnerMesh(
	FName SocketName,
	FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;

	if (SocketName.IsNone())
	{
		return false;
	}

	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(GetOwner());
	USkeletalMeshComponent* MeshComponent = EnemyAgent ? EnemyAgent->GetEnemyMesh() : nullptr;

	if (!IsValid(MeshComponent) || !MeshComponent->DoesSocketExist(SocketName))
	{
		return false;
	}

	OutTransform = MeshComponent->GetSocketTransform(SocketName, RTS_World);
	return true;
}

bool UNSEnemyPartComponent::TryGetTraceSegmentFromPartRow(
	const FNSEnemyPartRow& PartRow,
	float FallbackDistance,
	const FVector& FallbackDirection,
	FVector& OutStart,
	FVector& OutEnd) const
{
	OutStart = FVector::ZeroVector;
	OutEnd = FVector::ZeroVector;

	FTransform StartTransform;
	FTransform EndTransform;

	if (!PartRow.TraceStartSocket.IsNone() &&
		!PartRow.TraceEndSocket.IsNone() &&
		TryGetSocketTransformFromPartRow(PartRow, PartRow.TraceStartSocket, StartTransform) &&
		TryGetSocketTransformFromPartRow(PartRow, PartRow.TraceEndSocket, EndTransform))
	{
		OutStart = StartTransform.GetLocation();
		OutEnd = EndTransform.GetLocation();
		return true;
	}

	if (!PartRow.TraceSocket.IsNone() &&
		TryGetSocketTransformFromPartRow(PartRow, PartRow.TraceSocket, StartTransform))
	{
		const FVector TraceDirection = StartTransform.GetRotation().GetForwardVector().IsNearlyZero()
			                               ? FallbackDirection.GetSafeNormal()
			                               : StartTransform.GetRotation().GetForwardVector().GetSafeNormal();

		if (TraceDirection.IsNearlyZero())
		{
			return false;
		}

		OutStart = StartTransform.GetLocation();
		OutEnd = OutStart + TraceDirection * FMath::Max(FallbackDistance, 0.0f);
		return true;
	}

	return false;
}

bool UNSEnemyPartComponent::TryGetSocketTransformFromPartRow(
	const FNSEnemyPartRow& PartRow,
	FName SocketName,
	FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;

	if (SocketName.IsNone())
	{
		return false;
	}

	switch (PartRow.PartType)
	{
	case ENSEnemyPartType::SpawnedWeapon:
	case ENSEnemyPartType::SpawnedPart:
		if (TryGetSocketTransformFromSpawnedPart(PartRow, SocketName, OutTransform))
		{
			return true;
		}

		return TryGetSocketTransformFromOwnerMesh(SocketName, OutTransform);

	case ENSEnemyPartType::IntegratedWeapon:
	case ENSEnemyPartType::VisualOnly:
		return TryGetSocketTransformFromOwnerMesh(SocketName, OutTransform);

	default:
		return false;
	}
}

bool UNSEnemyPartComponent::TryGetLeftHandIKTransformFromPartRow(
	const FNSEnemyPartRow& PartRow,
	FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;

	if (PartRow.LeftHandIKSocket.IsNone())
	{
		return false;
	}

	return TryGetSocketTransformFromPartRow(PartRow, PartRow.LeftHandIKSocket, OutTransform);
}

bool UNSEnemyPartComponent::TryGetSocketTransformFromSpawnedPart(
	const FNSEnemyPartRow& PartRow,
	FName SocketName,
	FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;

	AActor* SpawnedActor = FindSpawnedPartActor(PartRow.PartId);
	if (!IsValid(SpawnedActor))
	{
		return false;
	}

	return TryGetSocketTransformFromActor(SpawnedActor, SocketName, OutTransform);
}
