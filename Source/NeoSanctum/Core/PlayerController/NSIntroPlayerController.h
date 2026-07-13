// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NSIntroPlayerController.generated.h"


class UMediaPlayer;
class UMediaSource;
class UMediaSoundComponent;
class UInputMappingContext;
class UInputAction;
class UNSPrologueWidget;
struct FInputActionInstance;


UCLASS()
class NEOSANCTUM_API ANSIntroPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ANSIntroPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	void StartPrologue();
	void StartPreload();
	// 재생 종료,스킵 공통 진입점
	void FinishPrologue();

	UFUNCTION()
	void HandleMediaEnd();
	
	void OnSkipOngoing(const FInputActionInstance& Instance);
	void OnSkipTriggered(const FInputActionInstance& Instance);
	void OnSkipReleased(const FInputActionInstance& Instance);
	
	// 마스터 볼륨을 프롤로그 사운드에 적용
	void ApplyPrologueVolume();

	UPROPERTY(EditDefaultsOnly, Category="Intro")
	TObjectPtr<UMediaPlayer> PrologueMediaPlayer;
	UPROPERTY(EditDefaultsOnly, Category="Intro")
	TObjectPtr<UMediaSource> PrologueMediaSource;
	UPROPERTY(VisibleAnywhere) 
	TObjectPtr<UMediaSoundComponent> PrologueMediaSound;
	UPROPERTY(EditDefaultsOnly, Category="Intro")
	TSubclassOf<UNSPrologueWidget> PrologueWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category="Intro|Input")
	TObjectPtr<UInputMappingContext> IntroMappingContext;
	UPROPERTY(EditDefaultsOnly, Category="Intro|Input") 
	TObjectPtr<UInputAction> SkipAction;
	UPROPERTY(EditDefaultsOnly, Category="Intro|Input")
	float SkipHoldSeconds = 2.0f;

	UPROPERTY()
	TObjectPtr<UNSPrologueWidget> PrologueWidget;
	
	bool bFinishing = false;
};
