// Copyright 2026 One Team. All rights reserved.


#include "NSCloakComponent.h"



UNSCloakComponent::UNSCloakComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UNSCloakComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	GetWorld()->GetTimerManager().ClearTimer(CloakTimerHandle);
	RestoreOriginalMaterials();
}

void UNSCloakComponent::StartCloak()
{
	EnsureDynamicMaterials();
	if (CloakMIDs.Num() == 0) return;
	
	BeginTransition(CurrentCloakAmount, TargetCloakAmount, EnterDuration);
}

void UNSCloakComponent::StopCloak()
{
	if (CloakMIDs.Num() == 0) return;
	
	BeginTransition(CurrentCloakAmount, 0.f, ExitDuration);
}

void UNSCloakComponent::CollectCloakMeshes(TArray<UMeshComponent*>& OutMeshes) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;
	CollectMeshesFromActor(OwnerActor, OutMeshes);
	TArray<AActor*> OutActors;
	if (bIncludeAttachedActors)
	{
		OwnerActor->GetAttachedActors(OutActors);
	}
	
	for (AActor* Actor : OutActors)
	{
		CollectMeshesFromActor(Actor, OutMeshes);
	}
}

void UNSCloakComponent::CollectMeshesFromActor(AActor* TargetActor, TArray<UMeshComponent*>& OutMeshes) const
{
	if (!TargetActor) return;
	
	TArray<UMeshComponent*> MeshComponents;
	TargetActor->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (IsValid(MeshComponent) && 
			(MeshComponent->IsA<UStaticMeshComponent>() || 
			MeshComponent->IsA<USkeletalMeshComponent>()))
		{
			OutMeshes.AddUnique(MeshComponent);
		}
	}
}

void UNSCloakComponent::EnsureDynamicMaterials()
{
	if (CloakMIDs.Num() > 0) return;
	
	TArray<UMeshComponent*> MeshComponents;
	CollectCloakMeshes(MeshComponents);

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		for (int32 i = 0; i < MeshComponent->GetNumMaterials(); ++i)
		{
			UMaterialInterface* OriginalMaterial = MeshComponent->GetMaterial(i);
			UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateDynamicMaterialInstance(i);
			CachedMeshes.Add(MeshComponent);
			CachedSlotIndices.Add(i);
			OriginalMaterials.Add(OriginalMaterial);
			CloakMIDs.Add(DynamicMaterial);
		}
	}
}

void UNSCloakComponent::BeginTransition(float FromAmount, float ToAmount, float Duration)
{
	CloakStartAmount = FromAmount;
	CloakTargetAmount = ToAmount;
	CloakTransitionDuration = Duration;
	
	CloakStartTime = GetWorld()->GetTimeSeconds();
	GetWorld()->GetTimerManager().SetTimer(
		CloakTimerHandle,
		this,
		&UNSCloakComponent::UpdateCloak,
		0.016f,
		true);
}

void UNSCloakComponent::UpdateCloak()
{
	const float Elapsed = GetWorld()->GetTimeSeconds() - CloakStartTime;
	const float Alpha = FMath::Clamp(Elapsed / CloakTransitionDuration, 0.f, 1.f);
	const float CurveT = EvaluateCloakCurve(Alpha);
	
	CurrentCloakAmount = FMath::Lerp(CloakStartAmount, CloakTargetAmount, CurveT);
	ApplyCloakAmount(CurrentCloakAmount);
	
	if (Alpha >= 1.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(CloakTimerHandle);
		if (CloakTargetAmount <= 0.0f)
		{
			RestoreOriginalMaterials();
		}
	}
}

void UNSCloakComponent::ApplyCloakAmount(float Amount)
{
	for (const TObjectPtr<UMaterialInstanceDynamic>& CloakMID : CloakMIDs)
	{
		if (!IsValid(CloakMID)) continue;
		CloakMID->SetScalarParameterValue(CloakAmountParamName, Amount);
	}
}

float UNSCloakComponent::EvaluateCloakCurve(float NormalizedTime) const
{
	if (CloakCurve)
	{
		return CloakCurve->GetFloatValue(NormalizedTime);
	}
	
	return NormalizedTime;
}

void UNSCloakComponent::RestoreOriginalMaterials()
{
	for (int32 i = 0; i < CachedMeshes.Num(); ++i)
	{
		if (!IsValid(CachedMeshes[i]))
		{
			continue;
		}

		CachedMeshes[i]->SetMaterial(CachedSlotIndices[i], OriginalMaterials[i]);
	}
    
	CachedMeshes.Reset();
	CachedSlotIndices.Reset();
	OriginalMaterials.Reset();
	CloakMIDs.Reset();
}


