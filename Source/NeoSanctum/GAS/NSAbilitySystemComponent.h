// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "NeoSanctum/Type/NSSkillCooldownTypes.h"
#include "NSAbilitySystemComponent.generated.h"

class UGameplayAbility;
class UGameplayEffect;
struct FGameplayEffectRemovalInfo;

/**
 * InputTag 기반 Ability 입력 처리 담당을 위한 ASC.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
	
	// 입력 이벤트를 한 프레임 단위로 모아서 처리
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
	
	// 입력 상태를 강제로 비울 때 사용
	void ClearAbilityInput();

	// Avatar 교체 시 이전 캐릭터의 임시 GAS 상태를 제거
	void ResetTransientAvatarState();

	// 클라이언트에 남은 로컬 Buff State 태그만 제거
	void ClearLocalBuffStateTags();
	
	// 스킬 슬롯별 재충전 진입점
	void StartSkillRecharge(const FGameplayTag& SkillSlotTag, float Cooldown);

	// 슬롯 쿨다운 UI 데이터 조회
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill|Cooldown")
	bool GetSkillCooldownUIData(const FGameplayTag& SkillSlotTag, FSkillCooldownUIData& OutData) const;

	// 최대 충전 수가 바뀐 슬롯의 UI를 바로 갱신.
	void NotifySkillCountChangedForMaxStat(const FGameplayTag& MaxSkillCountStatTag) const;

public:
	bool TryGetBaseAbilityStat(
		const FGameplayTag& AbilityTag,
		const FGameplayTag& StatTag,
		float& OutValue
	) const;
	
	bool TryGetFinalAbilityStat(
		const FGameplayTag& AbilityTag,
		const FGameplayTag& StatTag,
		float& OutValue
	) const;
	
	bool IsAbilityStatModifiable(
		const FGameplayTag& AbilityTag,
		const FGameplayTag& StatTag
	) const;

	// CombatStatComponent에 TemporaryModifier 등록
	FGuid AddTemporaryCombatStatModifier(
		const FGameplayTag& TargetAbilityTag,
		const FGameplayTag& StatTag,
		ENSCombatStatModifierOperation Operation,
		float Value,
		float Duration
	) const;

	// CombatStatComponent의 TemporaryModifier 제거
	void RemoveTemporaryCombatStatModifier(FGuid Handle) const;
	
protected:
	virtual void BeginPlay() override;
	
private:
	void HandleAbilityFailed(const UGameplayAbility* FailedAbility, const FGameplayTagContainer& FailureTags);
	
	// 해당 슬롯의 스킬이 이미 재충전 중인지 판단
	bool IsSkillRechargeActive(const FGameplayTag& SkillSlotTag) const;
	// 해당 슬롯의 스킬이 이미 MaxCount인지 판단
	bool IsSkillCountFull(const FGameplayTag& SkillSlotTag) const;
	// 슬롯에 맞는 현재/최대 충전 수 조회
	bool TryGetSkillCountForSlot(const FGameplayTag& SkillSlotTag, int32& OutCurrentCount, int32& OutMaxCount) const;
	
	// 슬롯에 해당하는 Recharge GE를 찾아 반환하는 함수
	TSubclassOf<UGameplayEffect> GetRechargeGEClassForSlot(const FGameplayTag& SkillSlotTag) const;
	// 슬롯에 해당하는 SetByCaller Effect Tag 반환
	FGameplayTag GetRechargeEffectTagForSlot(const FGameplayTag& SkillSlotTag) const;
	
	// Recharge GE 제거 시 충전 완료 여부를 처리
	void HandleSkillRechargeEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo, FGameplayTag SkillSlotTag);
	// 충전 완료 후 Count 회복 및 다음 충전 시작
	void FinishSkillRecharge(const FGameplayTag& SkillSlotTag);
	// 슬롯 쿨다운 UI 변경 메시지 전송
	void BroadcastSkillCooldownUIData(const FGameplayTag& SkillSlotTag) const;
	// 슬롯에 맞는 SkillCount를 증가
	void AddSkillCountForSlot(const FGameplayTag& SkillSlotTag, float Amount);
	
	// 다음 충전에 사용할 Cooldown을 슬롯별로 저장
	void CacheSkillRechargeCooldown(const FGameplayTag& SkillSlotTag, float Cooldown);
	// 슬롯에 저장된 마지막 Recharge Cooldown 조회
	float GetCachedSkillRechargeCooldown(const FGameplayTag& SkillSlotTag) const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Recharge")
	TSubclassOf<UGameplayEffect> Skill1RechargeGEClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Recharge")
	TSubclassOf<UGameplayEffect> Skill2RechargeGEClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Recharge")
	TSubclassOf<UGameplayEffect> Skill3RechargeGEClass;

	// 연속 충전을 위해 슬롯별 마지막 Cooldown을 보관
	TMap<FGameplayTag, float> CachedSkillRechargeCooldowns;
	
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
};
