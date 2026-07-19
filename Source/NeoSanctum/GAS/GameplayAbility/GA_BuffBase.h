// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "TimerManager.h"
#include "GA_BuffBase.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;

UENUM(BlueprintType)
enum class ENSBuffTargetType : uint8
{
	Self,
	SingleTarget,
	Radius
};

UENUM(BlueprintType)
enum class ENSBuffApplyType : uint8
{
	GameplayEffect,
	CombatStatModifier
};

USTRUCT(BlueprintType)
struct FNSBuffTargetFilter
{
	GENERATED_BODY()

	// 자신 포함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|Target")
	bool bIncludeSelf = true;

	// 자신이 소환한 Turret 포함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|Target")
	bool bIncludeOwnedTurrets = false;

	// 다른 플레이어 포함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|Target")
	bool bIncludeOtherPlayers = false;
};

USTRUCT(BlueprintType)
struct FNSBuffCombatStatModifier
{
	GENERATED_BODY()

	// 수정할 대상 Ability 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|CombatStat")
	FGameplayTag TargetAbilityTag;

	// 수정할 대상 CombatStat 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|CombatStat")
	FGameplayTag TargetStatTag;

	// 버프 Ability에서 읽어올 수치 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|CombatStat")
	FGameplayTag SourceValueStatTag;

	// CombatStat 수정 연산
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|CombatStat")
	ENSCombatStatModifierOperation Operation = ENSCombatStatModifierOperation::Add;
};

USTRUCT(BlueprintType)
struct FNSBuffApplyEntry
{
	GENERATED_BODY()

	// 적용 대상 Actor 타입
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|Apply")
	TSubclassOf<AActor> TargetActorClass;

	// 버프 적용 방식
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|Apply")
	ENSBuffApplyType ApplyType = ENSBuffApplyType::GameplayEffect;

	// 대상에게 적용할 GameplayEffect
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|Apply",
		meta = (EditCondition = "ApplyType == ENSBuffApplyType::GameplayEffect", EditConditionHides))
	TSubclassOf<UGameplayEffect> EffectClass;

	// 대상에게 적용할 CombatStat 수정 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|Apply",
		meta = (EditCondition = "ApplyType == ENSBuffApplyType::CombatStatModifier", EditConditionHides))
	TArray<FNSBuffCombatStatModifier> CombatStatModifiers;
};

struct FNSActiveBuffRuntime
{
	TArray<FGuid> CombatStatModifierHandles;
	FTimerHandle PresentationTimerHandle;
};

/**
 * 버프를 줄 수 있는 Ability
 * 
 * 어떤 대상에게 어떤 GE를 적용할 것인지를 결정하는 간단한 Ability
 */
UCLASS()
class NEOSANCTUM_API UGA_BuffBase : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_BuffBase();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

protected:
	// 버프 대상 적용
	void ApplyBuffToTargets(const TArray<AActor*>& Targets, float Duration);
	bool TryApplyBuffToTarget(AActor* TargetActor, float Duration);
	const FNSBuffApplyEntry* FindBuffApplyEntryForTarget(const AActor* TargetActor) const;

	// CombatStatModifier 방식 적용
	bool ApplyCombatStatModifierBuff(
		AActor* TargetActor,
		const FNSBuffApplyEntry& ApplyEntry,
		const FGameplayTag& AbilityTag,
		float Duration
	);

	// GameplayEffect 방식 적용
	bool ApplyGameplayEffectBuff(AActor* TargetActor, const FNSBuffApplyEntry& ApplyEntry, float Duration);
	void ApplySetByCallerMappingsToSpec(FGameplayEffectSpecHandle& SpecHandle, const FGameplayTag& AbilityTag) const;

	// 버프 State 태그 관리
	bool HasBuffStateTag(const AActor* TargetActor) const;
	void AddTemporaryBuffPresentation(AActor* TargetActor, float Duration);

	// 버프 지속시간 조회
	bool TryGetBuffDuration(float& OutDuration) const;
	// 버프 범위 시각화 Cue 실행
	void ExecuteRangePulseGameplayCue(float Radius) const;

protected:
	// 버프 대상 수집
	void CollectBuffTargets(TArray<AActor*>& OutTargets) const;
	void CollectSelfTargets(TArray<AActor*>& OutTargets) const;
	void CollectRadiusTargets(TArray<AActor*>& OutTargets) const;
	void CollectSingleTargetTargets(TArray<AActor*>& OutTargets) const;

protected:
	// 필터에 통과 대상만 Target에 추가
	void AddFilteredTarget(AActor* TargetActor, TArray<AActor*>& OutTargets) const;
	// TargetFilter 조건 확인
	bool PassesTargetFilter(const AActor* TargetActor) const;
	// 대상이 ASC를 보유하고 있는지 여부 확인 : ASC가 없는 대상이라면 어차피 버프를 받는 것 자체가 불가능함
	bool HasTargetAbilitySystem(const AActor* TargetActor) const;
	
	// Radius 수집 범위 조회
	bool TryGetBuffRadius(float& OutRadius) const;

protected:
	// 버프 유지 중 대상에게 부여할 State 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Buff|State")
	FGameplayTag BuffStateTag;

	// 버프 유지 중 재생할 GameplayCue 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Buff|Cue")
	FGameplayTag BuffGameplayCueTag;

	// Radius 타입 버프 범위 시각화 Cue 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Buff|Cue",
		meta = (EditCondition = "TargetType == ENSBuffTargetType::Radius", EditConditionHides))
	FGameplayTag RangePulseGameplayCueTag;
	
	// 버프 대상 수집 방식
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Buff|Target")
	ENSBuffTargetType TargetType = ENSBuffTargetType::Self;

	// 버프 대상 필터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Buff|Target")
	FNSBuffTargetFilter TargetFilter;

	// 대상 타입별 버프 적용 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Buff|Apply")
	TArray<FNSBuffApplyEntry> BuffApplyEntries;

	// CombatStat 값을 GameplayEffect SetByCaller 값으로 넘기기 위한 매핑
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Buff|SetByCaller")
	TArray<FNSSetByCallerFromCombatStat> SetByCallerMappings;

	// 버프 지속시간을 조회할 CombatStat 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Buff|Duration")
	FGameplayTag DurationStatTag;

	// Radius 수집 범위를 조회할 CombatStat 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Buff|Target",
		meta = (EditCondition = "TargetType == ENSBuffTargetType::Radius", EditConditionHides))
	FGameplayTag RadiusStatTag;

	// 활성화된 CombatStat 버프를 중첩하지 않고 새 지속시간으로 갱신.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Buff|Reapply")
	bool bRefreshDurationOnReapply = false;

	// 대상별 Modifier 핸들과 연출 타이머를 보관.
	TMap<TWeakObjectPtr<UAbilitySystemComponent>, FNSActiveBuffRuntime> ActiveBuffRuntimeByTarget;
};
