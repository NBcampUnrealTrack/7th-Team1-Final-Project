// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NeoSanctum/Data/Augment/NSAugmentTypes.h"
#include "NSAugmentInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAugmentInventoryChanged);

class UNSAugmentDefitnition;
class UAbilitySystemComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
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
	
public:
	UFUNCTION(BlueprintPure, Category="NS|Augment")
	const TArray<FNSAugmentInstance>& GetOwned() const { return Owned; }
	
	UFUNCTION(BlueprintPure, Category = "NS|Augment")
	int32 GetStackCount(const FPrimaryAssetId& DefId) const;
	
	UFUNCTION(BlueprintPure, Category = "NS|Augment")
	int32 GetLegendaryCount() const;
	
	UFUNCTION(BlueprintPure, Category="NS|Augment")	
	bool IsLegendaryFull() const {return GetLegendaryCount() >= MaxLegendarySlots; }

	UAbilitySystemComponent* GetOwnerASC();
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NS|Augment", meta=(ClampMin="0"))
	int32 MaxLegendarySlots = 3;
	
private:
	UFUNCTION()
	void OnRep_Owned();
	
	// 스택형 GE 적용
	void ApplyStackEffect(FNSAugmentInstance& Inst, UNSAugmentDefinition* Def, UAbilitySystemComponent* ASC);

	// 기믹 Legendary GA 부여
	void GrantMechanicAbility(FNSAugmentInstance& Inst, UNSAugmentDefinition* Def, UAbilitySystemComponent* ASC);
	
	UPROPERTY(ReplicatedUsing=OnRep_Owned)
	TArray<FNSAugmentInstance> Owned;
};
