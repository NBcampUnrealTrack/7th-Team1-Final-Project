// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "NSEnemyCharacterBase.generated.h"

class UGameplayAbility;
class UNSMonsterAttributeSet;

UCLASS(Abstract)
class NEOSANCTUM_API ANSEnemyCharacterBase : public ACharacter, public IAbilitySystemInterface,
                                             public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ANSEnemyCharacterBase();
	virtual void BeginPlay() override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return ASC; }

	/*
	 * EnemyCharacter가 타겟팅 혹은 피격되었을 때 진영을 알기 위한 TeamId 조회
	 */
	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(static_cast<uint8>(ETeamId::Enemy));
	}

public:
	UFUNCTION(BlueprintCallable, Category = "GAS|Death")
	void HandleDeath();

	void Die();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY(Transient)
	TObjectPtr<UNSMonsterAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayAbility> AttackAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayAbility> DeathAbilityClass;

protected:
	bool bIsDead = false;
};
