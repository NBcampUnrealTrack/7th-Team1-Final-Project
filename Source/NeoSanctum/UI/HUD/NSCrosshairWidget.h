// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
#include "NSCrosshairWidget.generated.h"

class UImage;
class UWidgetAnimation;

/**
 * 플레이어의 조준점을 표시하는 HUD위젯
 */
UCLASS()
class NEOSANCTUM_API UNSCrosshairWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//조준점 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowCrosshair();
	//조준점 숨김
	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideCrosshair();
	//조준점 색상 변경
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetCrosshairColor(FLinearColor Newcolor);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void PlayAttackFeedback(ENSCrosshairAttackFeedbackType FeedbackType);

private:
	// GMS 크로스헤어 피드백 메시지 수신
	void HandleAttackFeedbackMessage(FGameplayTag Channel, const FNSCrosshairAttackFeedbackMessage& Message);

	//조준점 이미지
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CrosshairImage;

	// 공격 결과별 피드백 이미지
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> NormalHitFeedbackImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CriticalHitFeedbackImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> HeadShotFeedbackImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ShieldBarrierHitFeedbackImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> KillFeedbackImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> DestroyFeedbackImage;

	// 공격 결과별 피드백 애니메이션
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> NormalHitFeedbackAnimation;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> CriticalHitFeedbackAnimation;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> HeadShotFeedbackAnimation;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> ShieldBarrierHitFeedbackAnimation;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> KillFeedbackAnimation;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> DestroyFeedbackAnimation;

	FGameplayMessageListenerHandle AttackFeedbackListenerHandle;

	// 피드백 이미지를 모두 숨김
	void HideAttackFeedbackImages() const;
	// 피드백 타입에 맞는 이미지 반환
	UImage* GetAttackFeedbackImage(ENSCrosshairAttackFeedbackType FeedbackType) const;
	// 피드백 타입에 맞는 애니메이션 반환
	UWidgetAnimation* GetAttackFeedbackAnimation(ENSCrosshairAttackFeedbackType FeedbackType) const;
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
};
