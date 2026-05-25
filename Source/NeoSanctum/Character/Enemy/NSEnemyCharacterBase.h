// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "NSEnemyCharacterBase.generated.h"

class UNSMonsterAttributeSet;

UCLASS(Abstract)
class NEOSANCTUM_API ANSEnemyCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ANSEnemyCharacterBase();
	virtual void BeginPlay() override;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return ASC; }


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;
	
	UPROPERTY(Transient)
	TObjectPtr<UNSMonsterAttributeSet> AttributeSet;
};
