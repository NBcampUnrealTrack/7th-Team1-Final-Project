#include "NSGameplayCueNotify_Cloak.h"

#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"

ANSGameplayCueNotify_Cloak::ANSGameplayCueNotify_Cloak()
{
	bAutoDestroyOnRemove = true;
	bAllowMultipleOnActiveEvents = false;
	bAllowMultipleWhileActiveEvents = false;
}

bool ANSGameplayCueNotify_Cloak::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	Super::OnActive_Implementation(MyTarget, Parameters);
	ApplyCloakParams(MyTarget, 1.f, FadeInTime);
	return true;
}

bool ANSGameplayCueNotify_Cloak::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	ApplyCloakParams(MyTarget, -1.f, FadeOutTime);
	CloakMIDs.Reset();
	return Super::OnRemove_Implementation(MyTarget, Parameters);
}

void ANSGameplayCueNotify_Cloak::ApplyCloakParams(AActor* MyTarget, float Direction, float FadeTime)
{
	USkeletalMeshComponent* Mesh = GetTargetMesh(MyTarget);
	if (!Mesh || !GetWorld())
	{
		return;
	}

	if (CloakMIDs.Num() == 0)
	{
		const int32 NumMaterials = Mesh->GetNumMaterials();
		for (int32 i = 0; i < NumMaterials; ++i)
		{
			if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(i))
			{
				CloakMIDs.Add(MID);
			}
		}
	}

	const float Now = GetWorld()->GetTimeSeconds();
	for (UMaterialInstanceDynamic* MID : CloakMIDs)
	{
		if (!MID) continue;
		MID->SetScalarParameterValue(StartTimeParam, Now);
		MID->SetScalarParameterValue(DirectionParam, Direction);
		MID->SetScalarParameterValue(FadeTimeParam, FMath::Max(FadeTime, 0.01f));
	}
}

USkeletalMeshComponent* ANSGameplayCueNotify_Cloak::GetTargetMesh(AActor* MyTarget) const
{
	if (const INSEnemyAgent* Agent = Cast<INSEnemyAgent>(MyTarget))
	{
		return Agent->GetEnemyMesh();
	}
	return MyTarget ? MyTarget->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}