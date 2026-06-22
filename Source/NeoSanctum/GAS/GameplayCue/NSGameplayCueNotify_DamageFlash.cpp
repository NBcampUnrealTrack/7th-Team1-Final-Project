// Copyright 2026 One Team. All rights reserved.


#include "NSGameplayCueNotify_DamageFlash.h"
#include "NeoSanctum/System/Component/NSDamageFlashComponent.h"

bool UNSGameplayCueNotify_DamageFlash::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{	
	Super::OnExecute_Implementation(MyTarget, Parameters);
	
	if (!MyTarget)
	{
		return false;
	}

	if (UNSDamageFlashComponent* Flash = MyTarget->FindComponentByClass<UNSDamageFlashComponent>())
	{
		if (Flash->TryPlayMaterialFlash())
		{
			return true;
		}
		
		Flash->PlayFlash();
	}
	
	return true;
}
