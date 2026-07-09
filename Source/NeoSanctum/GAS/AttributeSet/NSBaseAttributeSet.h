// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
#include "NSBaseAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

class ANSPlayerController;
DECLARE_MULTICAST_DELEGATE(FOnOutOfHealth);

struct FGameplayEffectModCallbackData;

/**
 * 전투 대상이 공통으로 사용하는 AttributeSet
 */
UCLASS()
class NEOSANCTUM_API UNSBaseAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "GAS|Attribute")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, Health);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "GAS|Attribute")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, MaxHealth);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseDamage, Category = "GAS|Attribute")
	FGameplayAttributeData BaseDamage;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, BaseDamage);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Defense, Category = "GAS|Attribute")
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, Defense);
	
	// 실제 CharacterMovement 반영은 Attribute 변경 콜백에서 처리
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "GAS|Attribute")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, MoveSpeed);
	
	// 최종 데미지를 잠시 담는 용이므로 Replicate하지 않음
	UPROPERTY(BlueprintReadOnly, Category = "GAS|Attribute")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, Damage);

	// 이번 Damage의 HitQuality를 잠시 담는 용이므로 Replicate하지 않음
	UPROPERTY(BlueprintReadOnly, Category = "GAS|Attribute")
	FGameplayAttributeData DamageHitQuality;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, DamageHitQuality);
	
public:
	FOnOutOfHealth OnOutOfHealth;
	
protected:
	// Health 적용 전 대상별 방어 처리를 수행
	virtual float HandlePreHealthDamage(float DamageAmount, const FGameplayEffectModCallbackData& Data);

	// 실제 Health 감소 후 공격자 로컬 피드백을 요청
	void NotifyAttackFeedbackAfterHealthDamage(
		const FGameplayEffectModCallbackData& Data,
		float PreviousHealth) const;

	// 실제 Health 감소 후 피격자 로컬 피드백을 요청
	virtual void NotifyHitTakenFeedbackAfterHealthDamage(
		const FGameplayEffectModCallbackData& Data,
		float PreviousHealth) const;

	// Damage Source가 플레이어 공격 결과 피드백을 허용하는지 확인
	bool ShouldTriggerPlayerAttackFeedback(const FGameplayEffectModCallbackData& Data) const;

	// Damage Source의 피격 리액션 공격 방식 조회
	ENSHitReactionAttackType ResolveHitReactionAttackType(const FGameplayEffectModCallbackData& Data) const;

	// Damage Meta Attribute에 담긴 HitQuality 조회
	ENSHitFeedbackQuality ResolveDamageHitQuality() const;

	// 실제 Health 감소 후 대상 액터의 월드 피격 리액션을 요청
	void NotifyHitReactionAfterHealthDamage(
		const FGameplayEffectModCallbackData& Data,
		float PreviousHealth) const;

	// 실제 Health 감소 후 데미지 숫자 표시를 요청
	void NotifyDamageNumberFeedbackAfterHealthDamage(
		const FGameplayEffectModCallbackData& Data,
		float PreviousHealth) const;

	// 플레이어 공격으로 피해를 받은 대상에 표시할 공격자 컨트롤러를 찾음.
	ANSPlayerController* ResolveDamageNumberViewerController(
		const FGameplayEffectModCallbackData& Data,
		const AActor* TargetActor) const;

	// HitResult, EffectContext Origin, Target 위치 순서로 표시 위치를 찾음.
	FVector ResolveDamageNumberWorldLocation(
		const FGameplayEffectModCallbackData& Data,
		const AActor* TargetActor) const;

	// 데미지 레이어 정보를 포함해서 월드 피격 리액션을 요청
	void NotifyHitReaction(
		const FGameplayEffectModCallbackData& Data,
		ENSHitReactionDamageLayer DamageLayer,
		float DamageAmount,
		bool bTargetDead) const;
	
private:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);
	
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
	
	UFUNCTION()
	void OnRep_BaseDamage(const FGameplayAttributeData& OldBaseDamage);
	
	UFUNCTION()
	void OnRep_Defense(const FGameplayAttributeData& OldDefense);
	
	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);
	
private:
	bool bOutOfHealth = false;
};
