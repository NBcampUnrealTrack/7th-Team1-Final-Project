// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
#include "NSHitTakenFeedbackWidget.generated.h"

class UImage;
class UWidgetAnimation;

/**
 * 로컬 플레이어의 피격 상태를 화면 가장자리와 비네트로 보여주는 HUD 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSHitTakenFeedbackWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// 피격 피드백 메시지를 실제 위젯 연출로 변환
	UFUNCTION(BlueprintCallable, Category = "HitFeedback")
	void HandleHitTakenFeedback(const FNSHitTakenFeedbackContext& Context);

	// 피격 타입에 맞는 이미지와 애니메이션을 재생
	UFUNCTION(BlueprintCallable, Category = "HitFeedback")
	void PlayHitTakenFeedback(const FNSHitTakenFeedbackContext& Context);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// GMS 피격 피드백 메시지 수신
	void HandleHitTakenFeedbackMessage(FGameplayTag Channel, const FNSHitTakenFeedbackMessage& Message);
	// GMS 피격 상태성 피드백 메시지 수신
	void HandleHitTakenFeedbackStateMessage(FGameplayTag Channel, const FNSHitTakenFeedbackStateMessage& Message);
	void HandleHitTakenFeedbackVitalsMessage(FGameplayTag Channel, const FNSHitTakenFeedbackVitalsMessage& Message);
	
private:
	// 연출용 이미지를 모두 숨김
	void HideHitTakenFeedbackImages() const;
	// 낮은 체력 비네트 표시 상태 갱신
	void UpdateLowHealthVignette(float HealthRatio, float ShieldRatio);
	// 상태성 피드백 표시 상태 변경
	void SetHitTakenFeedbackStateVisible(ENSHitTakenFeedbackStateType StateType, bool bVisible);
	// Shield Recharging 연출 표시 상태 변경
	void SetShieldRechargingFeedbackVisible(bool bVisible);
	
private:
	// 피격 타입에 맞는 이미지 반환
	UImage* GetHitTakenFeedbackImage(ENSHitTakenFeedbackType FeedbackType) const;
	// 피격 타입에 맞는 애니메이션 반환
	UWidgetAnimation* GetHitTakenFeedbackAnimation(ENSHitTakenFeedbackType FeedbackType) const;

private:
	// Shield 피격 화면 테두리 이미지
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ShieldHitFeedbackImage;

	// Health 피격 화면 테두리 이미지
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> HealthHitFeedbackImage;

	// 낮은 체력 상태 비네트 이미지
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> LowHealthVignetteImage;

	// Shield 재충전 상태 이미지
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ShieldRechargingFeedbackImage;

	// Shield 피격 애니메이션
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> ShieldHitFeedbackAnimation;

	// Health 피격 애니메이션
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> HealthHitFeedbackAnimation;

	// 낮은 체력 상태 비네트 애니메이션
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> LowHealthVignetteAnimation;

	// Shield 재충전 상태 애니메이션
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> ShieldRechargingFeedbackAnimation;

	// 낮은 체력 비네트를 표시하기 시작할 체력 비율
	UPROPERTY(EditDefaultsOnly, Category = "HitFeedback|Intensity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthThreshold = 0.3f;

	// 낮은 체력 비네트를 표시하기 시작할 Shield 비율
	UPROPERTY(EditDefaultsOnly, Category = "HitFeedback|Intensity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowShieldThresholdForLowHealth = 0.1f;

	// 낮은 체력 비네트의 고정 투명도
	UPROPERTY(EditDefaultsOnly, Category = "HitFeedback|Intensity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthVignetteOpacity = 1.0f;

private:
	// GMS 피격 피드백 리스너 핸들
	FGameplayMessageListenerHandle HitTakenFeedbackListenerHandle;
	// GMS 피격 상태성 피드백 리스너 핸들
	FGameplayMessageListenerHandle HitTakenFeedbackStateListenerHandle;
	FGameplayMessageListenerHandle HitTakenFeedbackVitalsListenerHandle;
	
	// 낮은 체력 비네트가 현재 활성화되어 있는지 여부
	bool bLowHealthVignetteActive = false;
	// Shield 재충전 연출이 현재 활성화되어 있는지 여부
	bool bShieldRechargingFeedbackActive = false;
};
