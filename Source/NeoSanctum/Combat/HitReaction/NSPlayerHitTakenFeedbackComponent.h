// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
#include "NSPlayerHitTakenFeedbackComponent.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

// 플레이어가 피해를 받았을 때 로컬 피격 피드백 메시지를 발행하는 컴포넌트
UCLASS()
class NEOSANCTUM_API UNSPlayerHitTakenFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSPlayerHitTakenFeedbackComponent();

	// 피격 Context를 기반으로 로컬 피격 피드백을 재생
	UFUNCTION(BlueprintCallable, Category = "HitFeedback")
	void HandleHitTakenFeedback(const FNSHitTakenFeedbackContext& Context);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Shield Recharging 상태 태그 변화 감지 시작
	void TryBindShieldRechargingTagEvent();
	// Health/Shield 비율 변화 감지 시작
	void TryBindVitalsAttributeEvents();
	// Shield Recharging 상태 태그 변화 감지 해제
	void UnbindShieldRechargingTagEvent();
	// Health/Shield 비율 변화 감지 해제
	void UnbindVitalsAttributeEvents();
	// Shield Recharging 태그 변화에 따라 상태성 피드백 메시지 발행
	void HandleShieldRechargingTagChanged(const FGameplayTag Tag, int32 NewCount);
	// Health/Shield Attribute 변화에 따라 생존 비율 메시지 발행
	void HandleVitalsAttributeChanged(const FOnAttributeChangeData& Data);
	// 상태성 피드백 메시지를 GMS로 발행
	void BroadcastHitTakenFeedbackState(ENSHitTakenFeedbackStateType StateType, bool bActive) const;
	// 현재 Health/Shield 비율을 GMS로 발행
	void BroadcastHitTakenFeedbackVitals() const;
	// 로컬 플레이어에게만 피드백을 재생할지 확인
	bool ShouldPlayLocalFeedback() const;

	// Shield Recharging 태그를 감지할 로컬 플레이어 ASC
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	// Shield Recharging 태그 이벤트 핸들
	FDelegateHandle ShieldRechargingTagDelegateHandle;
	bool bVitalsAttributeEventsBound = false;
};
