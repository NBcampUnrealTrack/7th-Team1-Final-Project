// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSDifficultyTimerWidget.generated.h"

class UProgressBar;
class UCommonTextBlock;
class UNSGameFlowSubsystem;

/**
 * 몬스터의 강해지는 시간과 난이도를 나타내는 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSDifficultyTimerWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(
		const FGeometry& MyGeometry,
		float InDeltaTime) override;

private:
	void RefreshDifficultyTimer();

	UNSGameFlowSubsystem* GetGameFlowSubsystem() const;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> DifficultyProgressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DifficultyLevelText;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> StageNumberText;
};
