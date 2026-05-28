// Copyright 2026 One Team. All rights reserved.


#include "NSDissolveComponent.h"


UNSDissolveComponent::UNSDissolveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSDissolveComponent::StartDissolve()
{
AActor* Owner = GetOwner();
	if (!Owner || !GetWorld()) return;

	DynamicMaterials.Empty();
	
	TArray<UMeshComponent*> MeshComponents;
	Owner->GetComponents<UMeshComponent>(MeshComponents);
	
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
		
		if (Owner->HasAuthority())
		{
			Owner->SetLifeSpan(DissolveDuration);
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
	}
}
