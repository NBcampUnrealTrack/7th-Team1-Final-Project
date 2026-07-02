// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NeoSanctum/Data/Augment/NSAugmentTypes.h"
#include "NSAugmentInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAugmentInventoryChanged);

class UGameplayEffect;
class UNSAugmentDefinition;
class UNSDataSubsystem;
class UAbilitySystemComponent;

UCLASS(ClassGroup=(NeoSanctum), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSAugmentInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSAugmentInventoryComponent();
	
	UPROPERTY(BlueprintAssignable, Category="NS|Augment")
	FOnAugmentInventoryChanged OnInventoryChanged;

	// 서버 권한, 증강 적용, 이미 있으면 Stack증가 없으면 GE/GA Grant
	void ApplyAugment(const FPrimaryAssetId& DefId);
	
	// 서버 권한, 인런 종료시 모든 GE/GA 제거
	void ClearAll();

	// 서버 권한, Seamless Travel 시 이전 PlayerState의 보유 증강 데이터를 이관 (핸들은 제외)
	void CopyRunStateFrom(const UNSAugmentInventoryComponent* Source);

	// 서버 권한, 새 ASC에 보유 증강의 GE/GA를 재적용하고 핸들을 갱신
	void ReapplyAll();
	
public:
	UFUNCTION(BlueprintPure, Category="NS|Augment")
	const TArray<FNSAugmentInstance>& GetOwned() const { return Owned; }
	
	UFUNCTION(BlueprintPure, Category = "NS|Augment")
	int32 GetStackCount(const FPrimaryAssetId& DefId) const;
	
	UFUNCTION(BlueprintPure, Category = "NS|Augment")
	int32 GetLegendaryCount() const;
	
	UFUNCTION(BlueprintPure, Category="NS|Augment")	
	bool IsLegendaryFull() const {return GetLegendaryCount() >= MaxLegendarySlots; }

	UFUNCTION(BlueprintPure, Category = "NS|Augment")
	int32 GetMaxLegendarySlots() const { return MaxLegendarySlots; }
	
	UAbilitySystemComponent* GetOwnerASC() const;
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NS|Augment", meta=(ClampMin="0"))
	int32 MaxLegendarySlots = 3;

	// Attribute 변경 증강들이 공유하는 GE.
	// GE에는 지원할 Attribute/Operation별 Modifier와 SetByCaller 태그가 미리 정의되어 있어야 함.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Augment|Effect")
	TSubclassOf<UGameplayEffect> SharedAttributeStackEffectClass;
	
private:
	UFUNCTION()
	void OnRep_Owned();
	
	// Attribute 변경 Row가 있는 증강에 공용 GE를 적용하고, DefinitionRows 기반 SetByCaller payload를 구성.
	void ApplyStackEffect(
		FNSAugmentInstance& Inst,
		const TArray<FNSAugmentDefinitionRow>& DefinitionRows,
		UAbilitySystemComponent* ASC,
		bool bAdjustCurrentByMaxDelta
	);
	
	static bool TryCalculateStackEffectMagnitude(
		const FNSAugmentDefinitionRow& DefinitionRow,
		int32 Stacks,
		float& OutMagnitude
	);

	// 기믹 Legendary GA 부여
	void GrantMechanicAbility(FNSAugmentInstance& Inst, UNSAugmentDefinition* Def, UAbilitySystemComponent* ASC);

	/**
 	 * Inventory가 보유하는 DefId에 대응하는 모든 증강 정의 Row를 찾습니다.
 	 *
 	 * 같은 Definition을 공유하는 여러 Modifier Row를 모두 반환하며,
 	 * 대표 메타데이터는 호출부에서 첫 Row를 기준으로 사용합니다.
 	 */
	bool TryFindDefinitionRows(
		UNSDataSubsystem* Data,
		const FPrimaryAssetId& DefId, 
		TArray<FNSAugmentDefinitionRow>& OutRows
	) const;
	
	UPROPERTY(ReplicatedUsing=OnRep_Owned)
	TArray<FNSAugmentInstance> Owned;
};
