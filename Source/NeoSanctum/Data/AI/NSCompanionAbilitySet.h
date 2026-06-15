// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSCompanionAbilitySet.generated.h"

UCLASS(BlueprintType, const)
class NEOSANCTUM_API UNSCompanionAbilitySet : public UDataAsset
{
	GENERATED_BODY()
public:
	void GiveToAbilitySystem(UAbilitySystemComponent* ASC,
		FNSCompanionAbilitySet_GameplayAbility* OutGranted,
		UObject* SourceObject = nullptr) const;
	
protected:
	UPROPERTY(EditDefaultsOnly, meta=(TitleProperty=Ability))
	TArray<FNSCompanionAbilitySet_GameplayAbility> GrantedAbilities;
};
