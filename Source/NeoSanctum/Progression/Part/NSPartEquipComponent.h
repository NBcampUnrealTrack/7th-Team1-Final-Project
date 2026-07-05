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
class ANSDroppedPart;
class UNSCurrencyComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FNSOnPartChanged, FGameplayTag, const FNSPartData&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FNSOnUpgradeResult, FGameplayTag, ENSPartUpgradeResult);
DECLARE_MULTICAST_DELEGATE(FNSOnShopStockChanged);

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
	// 서버 전용 직접 호출. DropLocationOverride 지정 시 기존 파츠를 그 위치에 드랍(줍기 교체용), 미지정 시 캐릭터 발밑
	void EquipPart(const FNSPartData& NewPart, TOptional<FVector> DropLocationOverride = TOptional<FVector>());
	void ClearAll();

	// Seamless Travel: 이전 PlayerState의 런타임 파츠 데이터 이관 (핸들 제외)
	void CopyRunStateFrom(const UNSPartEquipComponent* Source);
	// 새 ASC 기준으로 보유 파츠 GE/GA 재적용
	void ReapplyAll();
	// 이관된 런타임 파츠 유무 확인 (있으면 ApplyEquippedPart 대신 ReapplyAll)
	bool HasAnyEquipped() const { return EquippedParts.Num() > 0; }

	bool HasEquippedPart(FGameplayTag Slot) const;
	const FNSPartData* GetEquippedPart(FGameplayTag Slot) const;

	UFUNCTION(Server, Reliable)
	void Server_RequestEquip(FNSPartData NewPart);

	UFUNCTION(Server, Reliable)
	void Server_RequestReroll(FGameplayTag Slot);

	UFUNCTION(Server, Reliable)
	void Server_RequestUpgradeRarity(FGameplayTag Slot);

	// 클라 → 서버 줍기 요청 (상호작용 OnInteract에서 호출, 서버 TryPickup 경유)
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Part")
	void Server_RequestPickup(ANSDroppedPart* TargetPart);

	// 리롤 비용
	UFUNCTION(BlueprintPure, Category = "Part")
	int64 GetRerollCost(FGameplayTag Slot) const;

	// 현재 등급 → 다음 등급 업그레이드 비용
	UFUNCTION(BlueprintPure, Category = "Part")
	int64 GetUpgradeCost(FGameplayTag Slot) const;

	// 등급업 성공 확률 (0~1)
	UFUNCTION(BlueprintPure, Category = "Part")
	float GetUpgradeChance(FGameplayTag Slot) const;

	// 리롤/등급업/구매 결과 연출용, 수치 갱신은 OnRep_EquippedParts가 담당
	UFUNCTION(Client, Reliable)
	void Client_NotifyUpgradeResult(FGameplayTag Slot, ENSPartUpgradeResult Result);

	// 인런 상점 재고 생성 요청
	UFUNCTION(Server, Reliable)
	void Server_RequestGenerateStock();

	// 재고 구매 요청, 성공 시 즉시 장착
	UFUNCTION(Server, Reliable)
	void Server_RequestPurchase(int32 StockIndex);

	const TArray<FNSPartData>& GetShopStock() const { return ShopStock; }

	// 등급별 상점 구매 가격
	UFUNCTION(BlueprintPure, Category = "Part")
	int64 GetShopPrice(ENSPartRarity Rarity) const;

	FNSOnShopStockChanged OnShopStockChanged;

public:
	FNSOnPartChanged OnPartChanged;
	FNSOnUpgradeResult OnUpgradeResult;

	// 교체 시 바닥에 스폰할 드랍 액터 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Part")
	TSubclassOf<ANSDroppedPart> DroppedPartClass;

	// 상점 재고 -> 부위당 생성 개수, 에디터 수정가능
	UPROPERTY(EditDefaultsOnly, Category = "Part|Shop", meta = (ClampMin = "1"))
	int32 StockCountPerSlot = 3;

private:
	FNSPartData* FindPart(FGameplayTag Slot);
	const FNSPartData* FindPart(FGameplayTag Slot) const;

	void DropPartInSlot(FGameplayTag Slot, TOptional<FVector> LocationOverride);
	void SpawnDroppedPart(const FNSPartData& Part, const FVector& Location);
	void RemovePartEffects(FGameplayTag Slot);

	void RemoveGEForSlot(FGameplayTag Slot);
	void RemoveAbilitiesForSlot(FGameplayTag Slot);

	void ApplyPartEffect(FGameplayTag Slot);
	void Internal_ApplyGE(FGameplayTag Slot, TSubclassOf<UGameplayEffect> GEClass);
	void OnEffectLoaded(FGameplayTag Slot);

	void GrantAbilities(FGameplayTag Slot);
	void OnAbilitiesLoaded(FGameplayTag Slot);

	void RerollStat(FGameplayTag Slot);
	void UpgradeRarity(FGameplayTag Slot);
	float RollValueForRarity(const UNSPartDefinition* Def, ENSPartRarity Rarity) const;

	void GenerateShopStock();
	ENSPartRarity RollShopRarity() const;

	UAbilitySystemComponent* GetOwnerASC() const;
	UNSCurrencyComponent* GetCurrencyComponent() const;

	UFUNCTION()
	void OnRep_EquippedParts();

	UFUNCTION()
	void OnRep_ShopStock();

private:
	UPROPERTY(ReplicatedUsing=OnRep_EquippedParts)
	TArray<FNSPartData> EquippedParts;

	// 인런 개인 상점 재고. 스테이지 내 고정, Seamless Travel 시 이관하지 않음(다음 스테이지에서 재생성)
	UPROPERTY(ReplicatedUsing=OnRep_ShopStock)
	TArray<FNSPartData> ShopStock;

	// 서버 전용. 스테이지 내 재고 1회 생성 보장 — 매진과 미생성 구분용
	bool bShopStockGenerated = false;

	// 런타임 핸들 (슬롯별)
	TMap<FGameplayTag, FActiveGameplayEffectHandle> ActiveGEHandles;
	TMap<FGameplayTag, TArray<FGameplayAbilitySpecHandle>> GrantedAbilityHandlesBySlot;
	TMap<FGameplayTag, TSharedPtr<FStreamableHandle>> EffectLoadHandles;
	TMap<FGameplayTag, TSharedPtr<FStreamableHandle>> AbilityLoadHandles;
};
