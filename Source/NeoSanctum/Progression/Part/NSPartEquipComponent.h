// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayAbilitySpec.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NSPartEquipComponent.generated.h"

class UGameplayEffect;

DECLARE_MULTICAST_DELEGATE_OneParam(FNSOnPartChanged, const FNSPartData&);

/**
 * PlayerState에 부착
 * 장착/리롤/등급업/드랍 관리
 */
UCLASS(ClassGroup=(NEOSANCTUM), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSPartEquipComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSPartEquipComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 서버 전용 직접 호출 (런 시작 시 SaveGame → EquipPart 등)
	void EquipPart(const FNSPartData& NewPart);
	void RerollStat();
	void UpgradeRarity();
	void DropCurrentPart(const FVector& Location);
	void ClearAll();

	const FNSPartData& GetEquippedPart() const { return EquippedPart; }
	bool HasEquippedPart() const { return EquippedPart.IsValid(); }

	// 클라이언트 → 서버 요청
	UFUNCTION(Server, Reliable)
	void ServerRequestEquip(FNSPartData NewPart);

	UFUNCTION(Server, Reliable)
	void ServerRequestReroll();

	UFUNCTION(Server, Reliable)
	void ServerRequestUpgradeRarity();

public:
	// UI 구독용 델리게이트 (클라이언트에서 OnRep 시 브로드캐스트)
	FNSOnPartChanged OnPartChanged;

	// 등급 업그레이드 성공 확률 -> 추후 변경
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Part", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float UpgradeSuccessChance = 0.5f;

private:
	void RemoveActiveGE();
	void RemoveGrantedAbilities();
	void ApplyPartEffect();
	void GrantAbilities();
	float RollValueForRarity(const UNSPartDefinition* Def, ENSPartRarity Rarity) const;
	UAbilitySystemComponent* GetOwnerASC() const;

	// 장착된 파츠의 Definition을 NSDataSubsystem 캐시에서 조회, 없으면 .Get()으로 직접조회 -> nullptr반환
	UNSPartDefinition* GetEquippedDefinition() const;

	UFUNCTION()
	void OnRep_EquippedPart();

private:
	UPROPERTY(ReplicatedUsing=OnRep_EquippedPart)
	FNSPartData EquippedPart;

	FActiveGameplayEffectHandle ActiveGEHandle;
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
};
