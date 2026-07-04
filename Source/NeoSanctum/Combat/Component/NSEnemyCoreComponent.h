// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NeoSanctum/Core/GameFlow/NSDifficultyType.h"
#include "NSEnemyCoreComponent.generated.h"

class UNSEnemyData;
class UNSMonsterAttributeSet;
class UGameplayAbility;
class UAbilitySystemComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FNSEnemyDataChanged, UNSEnemyData*);

/**
 * EnemyData와 GAS 초기화를 Character/Pawn 공통으로 관리하는 컴포넌트입니다.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSEnemyCoreComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSEnemyCoreComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// EnemyData가 변경됐을 때 Owner가 시각 데이터 등을 갱신할 수 있도록 알리는 델리게이트
	FNSEnemyDataChanged OnEnemyDataChanged;

	// 서버에서 이 Enemy가 사용할 데이터 에셋을 설정하는 함수
	void SetEnemyData(UNSEnemyData* InEnemyData);

	// 현재 EnemyData를 반환하는 함수
	UNSEnemyData* GetEnemyData() const { return EnemyData; }

	// 몬스터별로 플레이어에게 주어질 경험치를 반환.
	float GetExperienceReward() const { return CachedExperienceReward; }

	// 난이도 배율을 설정하는 함수
	void SetDifficultyScale(const FNSDifficultyScale& InScale) { DifficultyScale = InScale; }

	// EnemyData를 기준으로 스탯과 기본 Ability/Effect를 초기화하는 함수
	void InitializeFromData(
		bool bFullInit,
		UNSMonsterAttributeSet* AttributeSet);

private:
	// 클라이언트에서 EnemyData가 복제됐을 때 호출되는 함수
	UFUNCTION()
	void OnRep_EnemyData();

	// Owner의 ASC를 반환하는 함수
	UAbilitySystemComponent* GetOwnerASC() const;

	// AttributeInitData를 기준으로 스탯을 초기화하는 함수
	void InitializeAttributes(UNSMonsterAttributeSet* AttributeSet);

	// DefaultEffect, DefaultAbility, AttackAbility를 부여하는 함수
	void GrantStartupAbilities();

private:
	// Enemy가 사용할 데이터 에셋
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing = OnRep_EnemyData, Category = "Enemy")
	TObjectPtr<UNSEnemyData> EnemyData;
	
	// 기본값 = {1,1,1} (따로 값 호출안되면 기본 배율인 1배 적용)
	// 현재 난이도 배율
	UPROPERTY(Transient)
	FNSDifficultyScale DifficultyScale;

	float CachedExperienceReward = 0.0f;
};
