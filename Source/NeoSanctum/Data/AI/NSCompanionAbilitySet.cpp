// Copyright 2026 One Team. All rights reserved.


#include "NSCompanionAbilitySet.h"

#include "AbilitySystemComponent.h"

void UNSCompanionAbilitySet::GiveToAbilitySystem(
	UAbilitySystemComponent* ASC,
	FNSCompanionAbilitySet_GrantedHandles* OutGranted,
	UObject* SourceObject) const
{
	check(ASC);
	
	if (!ASC->IsOwnerActorAuthoritative()) return;
	
	for (int32 i = 0; i < GrantedAbilities.Num(); ++i)
	{
		const FNSCompanionAbilitySet_GameplayAbility& AbilityToGrant = GrantedAbilities[i];
		
		if (!IsValid(AbilityToGrant.Ability))
		{
			UE_LOG(LogTemp, Warning, TEXT("GiveToAbilitySystem: Invalid Granted Ability"));
			continue;
		}
		
		FGameplayAbilitySpec Spec(
			AbilityToGrant.Ability,
			AbilityToGrant.AbilityLevel,
			INDEX_NONE,
			 SourceObject);
		
		const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
		
		if (OutGranted)
		{
			OutGranted->AddAbilitySpecHandle(Handle);
		}
	}
}
