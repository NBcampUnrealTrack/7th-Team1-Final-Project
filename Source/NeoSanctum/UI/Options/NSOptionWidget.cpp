// Copyright 2026 One Team. All rights reserved.


#include "NSOptionWidget.h"

#include "NeoSanctum/UI/Common/NSButtonBase.h"
#include "Components/WidgetSwitcher.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "SoundSetting/SoundSettingWidget.h"
#include "GameplaySetting/NSGameplaySettingWidget.h"

void UNSOptionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (SoundCategoryButton)
	{
		SoundCategoryButton->OnClicked().AddUObject(this, &UNSOptionWidget::OnClickedSoundCategoryButton);
	}
	if (GraphicCategoryButton)
	{
		GraphicCategoryButton->OnClicked().AddUObject(this, &UNSOptionWidget::OnClickedGraphicCategoryButton);
	}
	if (CloseButton) 
	{
		CloseButton->OnClicked().AddUObject(this, &UNSOptionWidget::OnClickedCloseButton);
	}
	
	if (GameplayCategoryButton)
	{
		GameplayCategoryButton->OnClicked().AddUObject(
			this,
			&ThisClass::OnClickedGameplayCategoryButton);
	}
	ShowOptionCategoryWidget(SoundSettingWidget);
	UpdateCategorySelection(SoundCategoryButton);
}

void UNSOptionWidget::NativeDestruct()
{
	if (SoundCategoryButton)
	{
		SoundCategoryButton->OnClicked().RemoveAll(this);
	}

	if (GraphicCategoryButton)
	{
		GraphicCategoryButton->OnClicked().RemoveAll(this);
	}

	if (GameplayCategoryButton)
	{
		GameplayCategoryButton->OnClicked().RemoveAll(this);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked().RemoveAll(this);
	}

	Super::NativeDestruct();
}
void UNSOptionWidget::OnClickedSoundCategoryButton()
{
	ShowOptionCategoryWidget(SoundSettingWidget);
	UpdateCategorySelection(SoundCategoryButton);
}

void UNSOptionWidget::OnClickedGraphicCategoryButton()
{
	ShowOptionCategoryWidget(GraphicSettingWidget);
	UpdateCategorySelection(GraphicCategoryButton);
}

void UNSOptionWidget::OnClickedGameplayCategoryButton()
{
		ShowOptionCategoryWidget(GameplaySettingWidget);
		UpdateCategorySelection(GameplayCategoryButton);
}

void UNSOptionWidget::ShowOptionCategoryWidget(UWidget* OptionWidget)
{
	if (!OptionSwitcher || !OptionWidget)
	{
		return;
	}

	OptionSwitcher->SetActiveWidget(OptionWidget);
}

void UNSOptionWidget::UpdateCategorySelection(UNSButtonBase* SelectedButton)
{
	if (SoundCategoryButton)
	{
		SoundCategoryButton->SetIsSelected(
			SoundCategoryButton == SelectedButton,
			false);
	}

	if (GraphicCategoryButton)
	{
		GraphicCategoryButton->SetIsSelected(
			GraphicCategoryButton == SelectedButton,
			false);
	}

	if (GameplayCategoryButton)
	{
		GameplayCategoryButton->SetIsSelected(
			GameplayCategoryButton == SelectedButton,
			false);
	}
}

void UNSOptionWidget::OnClickedCloseButton()
{
	UGameInstance* GameInstance = GetGameInstance();
	UNSUIManagerSubsystem* UIManager = GameInstance
		? GameInstance->GetSubsystem<UNSUIManagerSubsystem>() : nullptr;
	if (UIManager)
	{
		UIManager->CloseOptionPanel(); 
	}
}
