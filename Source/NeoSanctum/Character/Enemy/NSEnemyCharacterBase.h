// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "NSEnemyCharacterBase.generated.h"

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
	 * 0번을 플레이어 진영
	 * 1번을 몬스터 진영으로 구성할 예정.
	 * EnemyCharacter가 타겟팅 혹은 피격되었을 때 진영을 알기 위한 TeamId 조회
	 */
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(1); }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY(Transient)
	TObjectPtr<UNSMonsterAttributeSet> AttributeSet;
};
