// Copyright 2026 One Team. All rights reserved.


#include "NSGameplayCueNotify_Cloak.h"

#include "NeoSanctum/AI/Enemy/Components/NSCloakComponent.h"

bool ANSGameplayCueNotify_Cloak::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (!MyTarget) return false;
	Super::OnActive_Implementation(MyTarget, Parameters);
	if (UNSCloakComponent* CloakComponent = MyTarget->FindComponentByClass<UNSCloakComponent>())
	{
		CloakComponent->StartCloak();
	}
	
	return true;
}

bool ANSGameplayCueNotify_Cloak::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (!MyTarget) return false;
	if (UNSCloakComponent* CloakComponent = MyTarget->FindComponentByClass<UNSCloakComponent>())
	{
		CloakComponent->StopCloak();
	}
	
	Super::OnRemove_Implementation(MyTarget, Parameters);
	return true;
}
