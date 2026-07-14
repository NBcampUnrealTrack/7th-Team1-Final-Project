// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSPlayerAudioFlowComponent.generated.h"

class ANSRunGameState;

UCLASS(ClassGroup=(NS))
class NEOSANCTUM_API UNSPlayerAudioFlowComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSPlayerAudioFlowComponent();

	void HandleTitleLevelReady();
	void HandleOutRunLevelReady();
	void HandlePreClientTravel();
	void HandleClientRunDataReady();
	void BindRunGameState(ANSRunGameState* RunGameState);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleStagePhaseChanged();

	void PlayBGM(FName SoundID, float FadeOut = 1.0f, float FadeIn = 1.0f);
	void StopBGM(float FadeOut = 1.0f);
	void PlayCurrentLevelStageBGM();
	bool TryPlayPendingBGM();
	void ScheduleBGMRetry();
	void ClearPendingBGM();

	UFUNCTION()
	void RetryPendingBGM();

	bool ShouldHandleAudio() const;

	UPROPERTY()
	TWeakObjectPtr<ANSRunGameState> CachedRunGameState;

	FName CurrentBGMID = NAME_None;
	FName PendingBGMID = NAME_None;
	float PendingFadeOut = 1.0f;
	float PendingFadeIn = 1.0f;
	int32 PendingBGMRetryCount = 0;
	bool bPendingBGMStoppedCurrent = false;

	FTimerHandle BGMRetryTimerHandle;
};
