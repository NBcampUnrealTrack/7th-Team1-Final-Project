// Copyright 2026 One Team. All rights reserved.


#include "NSDissolveComponent.h"


UNSDissolveComponent::UNSDissolveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSDissolveComponent::StartDissolve(bool bDestroyAfterDissolve)
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld()) return;

	DynamicMaterials.Empty();

	TArray<UMeshComponent*> MeshComponents;
	CollectDissolveMeshes(MeshComponents);
	
	for (UMeshComponent* Mesh : MeshComponents)
	{
		if (!Mesh) continue;
		
		const int32 NumMaterials = Mesh->GetNumMaterials();
		for (int32 i = 0; i < NumMaterials; ++i)
		{
			if (UMaterialInstanceDynamic* DMI = Mesh->CreateDynamicMaterialInstance(i))
			{
				DynamicMaterials.Add(DMI);
			}
		}
	}
	
	if (DynamicMaterials.Num() > 0)
	{
		DissolveStartTime = GetWorld()->GetTimeSeconds();

		GetWorld()->GetTimerManager().SetTimer(
			DissolveTimerHandle, 
			this, 
			&UNSDissolveComponent::UpdateDissolveAlpha, 
			0.016f, // 60fps
			true
		);
		
		if (bDestroyAfterDissolve && Owner->HasAuthority())
		{
			Owner->SetLifeSpan(DissolveDuration);
		}
	}
}

void UNSDissolveComponent::ResetDissolve()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DissolveTimerHandle);
	}

	for (UMaterialInstanceDynamic* DMI : DynamicMaterials)
	{
		if (DMI)
		{
			DMI->SetScalarParameterValue(TEXT("DissolveMask"), -1.0f);
		}
	}
}

void UNSDissolveComponent::UpdateDissolveAlpha()
{
	if (!GetWorld()) return;

	float ElapsedTime = GetWorld()->GetTimeSeconds() - DissolveStartTime;
	float Alpha = FMath::Clamp(ElapsedTime / DissolveDuration, 0.0f, 1.0f);
	float CurrentMaskValue = FMath::Lerp(-1.0f, 1.0f, Alpha);

	for (UMaterialInstanceDynamic* DMI : DynamicMaterials)
	{
		if (DMI)
		{
			DMI->SetScalarParameterValue(TEXT("DissolveMask"), CurrentMaskValue);
		}
	}

	if (Alpha >= 1.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(DissolveTimerHandle);
		OnDissolveComplete.ExecuteIfBound();
	}
}

void UNSDissolveComponent::CollectDissolveMeshes(TArray<UMeshComponent*>& OutMeshes) const
{
	OutMeshes.Reset();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	CollectMeshesFromActor(Owner, OutMeshes);

	if (!bIncludeAttachedActors)
	{
		return;
	}

	TArray<AActor*> AttachedActors;
	Owner->GetAttachedActors(AttachedActors);

	for (AActor* AttachedActor : AttachedActors)
	{
		CollectMeshesFromActor(AttachedActor, OutMeshes);
	}
}

void UNSDissolveComponent::CollectMeshesFromActor(
	AActor* TargetActor,
	TArray<UMeshComponent*>& OutMeshes) const
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	TArray<UMeshComponent*> MeshComponents;
	TargetActor->GetComponents<UMeshComponent>(MeshComponents);

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (IsValid(MeshComponent))
		{
			OutMeshes.AddUnique(MeshComponent);
		}
	}
}