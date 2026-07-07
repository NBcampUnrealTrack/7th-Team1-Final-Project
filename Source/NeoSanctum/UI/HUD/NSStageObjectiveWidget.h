// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSStageObjectiveWidget.generated.h"

class ANSRunGameState;
class UCommonTextBlock;
class UWidget;

/**
 *  현재 스테이지 목표 진행도와 다음 행동을 표시하는 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSStageObjectiveWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	virtual void NativeTick(
		const FGeometry& MyGeometry,
		float InDeltaTime) override; 
	
private:
	void BindToRunGameState();
	void UnbindFromRunGameState();
	void UpdateTransitionCountdown();
	
	UFUNCTION()
	void RefreshStageObjective();
	
	UFUNCTION()
	void RefreshBossGate();
	
	UPROPERTY(meta =(BindWidget))
	TObjectPtr<UWidget> ObjectivePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> ObjectiveMessageText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> ObjectiveProgressText;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> TransitionPanel;
	
	UPROPERTY(meta =(BindWidget))
	TObjectPtr<UCommonTextBlock> TransitionMessageText;
	
	UPROPERTY(meta =(BindWidget))
	TObjectPtr<UCommonTextBlock> TransitionCountdownText;

	UPROPERTY()
	TObjectPtr<ANSRunGameState> CachedRunGameState;
};
