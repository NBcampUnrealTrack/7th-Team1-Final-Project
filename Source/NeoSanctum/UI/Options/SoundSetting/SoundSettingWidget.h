// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SoundSettingWidget.generated.h"

class UCommonTextBlock;
class UButton;
class UTextBlock;
class USlider;
/**
 * 
 */
UCLASS()
class NEOSANCTUM_API USoundSettingWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	
protected:
	UFUNCTION()
	void OnMasterVolumeChanged(float Value);
	
	UFUNCTION()
	void OnBGMVolumeChanged(float Value);
	
	UFUNCTION()
	void OnSFXVolumeChanged(float Value);
	
	UFUNCTION()
	void OnUIVolumeChanged(float Value);
	
protected:
	void UpdateVolumeText(UTextBlock* Text, float Value);
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MasterVolumeSlider;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> BGMVolumeSlider;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> SFXVolumeSlider;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> UIVolumeSlider;
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> MasterVolumeText;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> BGMVolumeText;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> SFXVolumeText;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> UIVolumeText;
};
