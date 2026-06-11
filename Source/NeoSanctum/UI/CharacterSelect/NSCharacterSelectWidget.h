// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CommonActivatableWidget.h"
#include "NeoSanctum/Data/UI/NSCharacterSelectData.h"
#include "NSCharacterSelectWidget.generated.h"

class UCommonAnimatedSwitcher;
class UCommonButtonBase;
class UTextBlock;
class UNSCharacterSlotWidget;
class UImage;

UCLASS()
class NEOSANCTUM_API UNSCharacterSelectWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UCommonButtonBase> NextButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UCommonButtonBase> PrevButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UCommonButtonBase> ConfirmButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> CharacterNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> CharacterDescriptionText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UCommonAnimatedSwitcher> CharacterSwitcher;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UImage> PreviewImage;	


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterSelect")
	TObjectPtr<UDataTable> CharacterDataTable;

	UFUNCTION(BlueprintCallable, Category = "CharacterSelect")
	void SelectNext();

	UFUNCTION(BlueprintCallable, Category = "CharacterSelect")
	void SelectPrev();

	UFUNCTION(BlueprintCallable, Category = "CharacterSelect")
	void ConfirmSelection();


private:
	void HandleCharacterChanged();
	void FadeAndSwitch();
	void OnFadeOutFinished();

	FTimerHandle FadeTimerHandle;
	int32 CurrentIndex = 0;

	TArray<FNSCharacterSelectData*> CachedCharacters;

	void ApplyPreviewImage(const FNSCharacterSelectData& Data);
};