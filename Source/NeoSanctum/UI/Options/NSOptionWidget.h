// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSOptionWidget.generated.h"

class USoundSettingWidget;
class UNSButtonBase;
class UWidgetSwitcher;
class UNSGameplaySettingWidget;

/**
 * 옵션창 Widget
 * WidgetSwitcher를 활용해서 특정 위젯만 활성화하게함
 */
UCLASS()
class NEOSANCTUM_API UNSOptionWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
private:
	UFUNCTION()
	void OnClickedSoundCategoryButton();
	
	UFUNCTION()
	void OnClickedGraphicCategoryButton();
	
	UFUNCTION()
	void OnClickedGameplayCategoryButton();
	
private:
	void ShowOptionCategoryWidget(UWidget* OptionWidget);
	
	// 선택한 카테고리 버튼만 선택 상태로 유지한다.
	void UpdateCategorySelection(UNSButtonBase* SelectedButton);
	
private:
	// 활성화 위젯을 변경하는 Switcher
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> OptionSwitcher;
	
private:
	// 사운드 카테고리를 선택하기 위한 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNSButtonBase> SoundCategoryButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNSButtonBase> GraphicCategoryButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNSButtonBase> GameplayCategoryButton;
	
	// TODO : 앞으로 다양한 Option Button들이 생기면 여기에 추가
	
	// (이용호 추가) 옵션창 닫는 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNSButtonBase> CloseButton;
	
private:
	// 사용하는 사운드 세팅 위젯
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USoundSettingWidget> SoundSettingWidget;
	
	// 테스트용 임시 그래픽 세팅 위젯
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> GraphicSettingWidget;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNSGameplaySettingWidget> GameplaySettingWidget;
	
	// TODO : 앞으로 Graphic, Game 등 다양한 세팅 위젯들이 추가될 때 이 곳에 추가
	
	// 옵션 창 종료
	UFUNCTION()
	void OnClickedCloseButton();
};
