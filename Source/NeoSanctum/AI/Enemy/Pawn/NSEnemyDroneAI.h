// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/AI/Base/NSBaseDroneAI.h"
#include "NeoSanctum/Core/GameFlow/NSDifficultyType.h"
#include "NSEnemyDroneAI.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEnemyDroneDead, ANSEnemyDroneAI*);

class UNSEnemyDroneAttributeSet;
class UNSEnemyData;
class UGameplayAbility;
class UNSDamageFlashComponent;
class UMaterialInstanceDynamic;

UCLASS()
class NEOSANCTUM_API ANSEnemyDroneAI : public ANSBaseDroneAI
{
	GENERATED_BODY()

public:
	ANSEnemyDroneAI();

	virtual void PossessedBy(AController* NewController) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	void SetPendingEnemyData(const UNSEnemyData* InEnemyData);
	
	FOnEnemyDroneDead OnEnemyDroneDead;
	
protected:
	virtual void BeginPlay() override;
	virtual void InitializeFromData() override;

	void InitializeEnemyDroneData(bool bFullInit);
	
public:
	virtual void Tick(float DeltaTime) override;
	
	void Die();

	void PrepareForReuse(const FVector& SpawnLocation, const FRotator& SpawnRotation);
	void DeactivateForPool();
	
	void SetDifficultyScale(const FNSDifficultyScale& InScale) { CurrentDifficultyScale = InScale; }
	
	// (이용호 추가) 외부에서 생존 여부 확인용
	bool IsDead() const { return bIsDead; }
	bool IsInPool() const { return bIsInPool; }
	
protected:
	//(이용호 추가)
	void ApplyAliveVisual();
	void ApplyDeadVisual();
	void ApplyVisualData();
private:
	//(이용호 추가)
	void OnDissolveFinished();
	UPROPERTY(ReplicatedUsing = OnRep_bIsInPool)
	bool bIsInPool = false;
	// 기본값 = {1,1,1} (따로 값 호출안되면 기본 배율인 1배 적용)
	FNSDifficultyScale CurrentDifficultyScale;
	
	UFUNCTION()
	void OnRep_bIsInPool();
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	TObjectPtr<class UNSDissolveComponent> DissolveComponent;
	
#pragma region CachedData
	
public:
	void SetOwnerBoss(AActor* InOwnerBoss);
	AActor* GetOwnerBoss() const { return OwnerBoss;}
	
protected:
	UFUNCTION()
	void OnRep_CurrentEnemyData();
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Owner")
	TObjectPtr<AActor> OwnerBoss;
	
	UPROPERTY(ReplicatedUsing= OnRep_CurrentEnemyData)
	TObjectPtr<const UNSEnemyData> CurrentEnemyData;
	
#pragma endregion
	
#pragma region EnemyDroneGAS
	
public:
	FORCEINLINE UNSEnemyDroneAttributeSet* GetEnemyDroneAttributeSet() const { return EnemyDroneAttributeSet; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UNSEnemyDroneAttributeSet> EnemyDroneAttributeSet;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayAbility> DeathAbilityClass;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_bIsDead, Category = "GAS|Death")
	bool bIsDead = false;
	
	UFUNCTION()
	void OnRep_bIsDead();
#pragma endregion
	
#pragma region MID 적용

protected:
	// 몬스터의 외형 MID 또는 기존 Overlay에 피격 효과를 적용하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSDamageFlashComponent> DamageFlashComponent;
	
	// 현재 몬스터 외형에 적용된 런타임 MID 배열
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RuntimeVisualMaterials;

	// EnemyData를 바탕으로 외형 MID를 생성하고 피격 컴포넌트에 등록하는 함수
	void InitializeRuntimeMaterials();
#pragma endregion
	
};
