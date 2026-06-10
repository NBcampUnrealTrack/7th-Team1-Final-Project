// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "NSEnemyCharacterBase.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnEnemyDead);

class UNSEnemyData;
class UGameplayAbility;
class UNSMonsterAttributeSet;
class ANSEnemyWeaponBase;

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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	FORCEINLINE UNSEnemyData* GetEnemyData() const { return EnemyData; }

	ANSEnemyWeaponBase* GetCurrentWeapon() const;

	FOnEnemyDead OnEnemyDead;

public:
	void Die();

	// (이용호 추가) 외부에서 생존 여부 확인용
	bool IsDead() const { return bIsDead; }
	bool IsInPool() const { return bIsInPool; }
	
	// 스폰 시 데이터 주입용 (BeginPlay 전 호출)
	void SetEnemyData(UNSEnemyData* InEnemyData);
	
	void PrepareForReuse(const FVector& SpawnLocation, const FRotator& SpawnRotation);
	void DeactivateForPool();
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY(Transient)
	TObjectPtr<UNSMonsterAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "Character Data")
	TObjectPtr<UNSEnemyData> EnemyData;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayAbility> DeathAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	TObjectPtr<class UNSDissolveComponent> DissolveComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UNSEnemyWeaponComponent> WeaponComponent;

protected:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_bIsDead, Category = "GAS|Death")
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_bIsDead();
	
	//(이용호 추가)
	void ApplyAliveVisual();
	void ApplyDeadVisual();
	void InitializeFromData(bool bFullInit);

private:
	//(이용호 추가)
	void OnDissolveFinished();
	UPROPERTY(ReplicatedUsing = OnRep_bIsInPool)
	bool bIsInPool = false;
	
	UFUNCTION()
	void OnRep_bIsInPool();
};
