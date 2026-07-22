#include "NSGameplayCueNotify_CloakSwap.h"

#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"

ANSGameplayCueNotify_CloakSwap::ANSGameplayCueNotify_CloakSwap()
{
	bAutoDestroyOnRemove = true;
	bAllowMultipleOnActiveEvents = false;
	bAllowMultipleWhileActiveEvents = false;
}

bool ANSGameplayCueNotify_CloakSwap::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	Super::OnActive_Implementation(MyTarget, Parameters);

	USkeletalMeshComponent* Mesh = GetTargetMesh(MyTarget);
	if (!Mesh || !CloakMaterial || !GetWorld())
	{
		return true;
	}

	OriginalMaterials.Reset();
	CloakMIDs.Reset();

	const float Now = GetWorld()->GetTimeSeconds();
	const int32 NumMaterials = Mesh->GetNumMaterials();
	for (int32 i = 0; i < NumMaterials; ++i)
	{
		OriginalMaterials.Add(Mesh->GetMaterial(i));

		UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(i, CloakMaterial);
		if (MID)
		{
			MID->SetScalarParameterValue(StartTimeParam, Now);
			MID->SetScalarParameterValue(DirectionParam, 1.f);
			MID->SetScalarParameterValue(FadeTimeParam, FMath::Max(FadeInTime, 0.01f));
			CloakMIDs.Add(MID);
		}
	}

	return true;
}

bool ANSGameplayCueNotify_CloakSwap::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	USkeletalMeshComponent* Mesh = GetTargetMesh(MyTarget);
	if (Mesh && GetWorld())
	{
		const float Now = GetWorld()->GetTimeSeconds();
		for (UMaterialInstanceDynamic* MID : CloakMIDs)
		{
			if (!MID) continue;
			MID->SetScalarParameterValue(StartTimeParam, Now);
			MID->SetScalarParameterValue(DirectionParam, -1.f);
			MID->SetScalarParameterValue(FadeTimeParam, FMath::Max(FadeOutTime, 0.01f));
		}

		TWeakObjectPtr<USkeletalMeshComponent> WeakMesh = Mesh;
		TArray<TObjectPtr<UMaterialInterface>> OriginalsCopy = OriginalMaterials;
		FTimerHandle RevertHandle;
		GetWorld()->GetTimerManager().SetTimer(
			RevertHandle,
			FTimerDelegate::CreateWeakLambda(MyTarget, [WeakMesh, OriginalsCopy]()
			{
				if (USkeletalMeshComponent* M = WeakMesh.Get())
				{
					for (int32 i = 0; i < OriginalsCopy.Num(); ++i)
					{
						M->SetMaterial(i, OriginalsCopy[i]);
					}
				}
			}),
			FMath::Max(FadeOutTime, 0.01f),
			false);
	}

	OriginalMaterials.Reset();
	CloakMIDs.Reset();
	return Super::OnRemove_Implementation(MyTarget, Parameters);
}

USkeletalMeshComponent* ANSGameplayCueNotify_CloakSwap::GetTargetMesh(AActor* MyTarget) const
{
	if (const INSEnemyAgent* Agent = Cast<INSEnemyAgent>(MyTarget))
	{
		return Agent->GetEnemyMesh();
	}
	return MyTarget ? MyTarget->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}