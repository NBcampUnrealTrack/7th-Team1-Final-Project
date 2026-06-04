// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayAbilitySpec.h"
#include "Engine/StreamableManager.h"
#include "NEOSanctum/Data/Part/NSPartTypes.h"
#include "NSPartEquipComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UNSPartDefinition;

DECLARE_MULTICAST_DELEGATE_TwoParams(FNSOnPartChanged, ENSPartSlot, const FNSPartData&);

/**
 * PlayerState에 부착되는 파츠 컴포넌트
 * 슬롯별 파츠 장착/드랍/GAS 연동 관리
 */
UCLASS(ClassGroup=(NEOSANCTUM), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSPartEquipComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSPartEquipComponent();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// 서버 전용 직접 호출
	void EquipPart(const FNSPartData& NewPart);
	void ClearAll();

	bool HasEquippedPart(ENSPartSlot Slot) const;
	const FNSPartData* GetEquippedPart(ENSPartSlot Slot) const;

	UFUNCTION(Server, Reliable)
	void ServerRequestEquip(FNSPartData NewPart);

	UFUNCTION(Server, Reliable)
	void ServerRequestReroll(ENSPartSlot Slot);

	UFUNCTION(Server, Reliable)
	void ServerRequestUpgradeRarity(ENSPartSlot Slot);

public:
	FNSOnPartChanged OnPartChanged;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Part", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float UpgradeSuccessChance = 0.5f;

private:
	FNSPartData* FindPart(ENSPartSlot Slot);
	const FNSPartData* FindPart(ENSPartSlot Slot) const;

	void DropPartInSlot(ENSPartSlot Slot, const FVector& Location);
	void RemovePartEffects(ENSPartSlot Slot);

	void RemoveGEForSlot(ENSPartSlot Slot);
	void RemoveAbilitiesForSlot(ENSPartSlot Slot);

	void ApplyPartEffect(ENSPartSlot Slot);
	void Internal_ApplyGE(ENSPartSlot Slot, TSubclassOf<UGameplayEffect> GEClass);
	void OnEffectLoaded(ENSPartSlot Slot);

	void GrantAbilities(ENSPartSlot Slot);
	void OnAbilitiesLoaded(ENSPartSlot Slot);

	void RerollStat(ENSPartSlot Slot);
	void UpgradeRarity(ENSPartSlot Slot);
	float RollValueForRarity(const UNSPartDefinition* Def, ENSPartRarity Rarity) const;

	UAbilitySystemComponent* GetOwnerASC() const;

	// 장착 파츠의 Definition을 NSDataSubsystem 캐시에서 조회, 없으면 .Get() — nullptr 반환 가능
	UNSPartDefinition* ResolveDefinition(const FNSPartData& Part) const;

	UFUNCTION()
	void OnRep_EquippedParts();

private:
	UPROPERTY(ReplicatedUsing=OnRep_EquippedParts)
	TArray<FNSPartData> EquippedParts;

	// 런타임 핸들 (슬롯별)
	TMap<ENSPartSlot, FActiveGameplayEffectHandle> ActiveGEHandles;
	TMap<ENSPartSlot, TArray<FGameplayAbilitySpecHandle>> GrantedAbilityHandlesBySlot;
	TMap<ENSPartSlot, TSharedPtr<FStreamableHandle>> EffectLoadHandles;
	TMap<ENSPartSlot, TSharedPtr<FStreamableHandle>> AbilityLoadHandles;
};
