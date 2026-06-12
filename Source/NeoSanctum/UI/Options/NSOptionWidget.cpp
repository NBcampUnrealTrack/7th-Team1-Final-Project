// Copyright 2026 One Team. All rights reserved.


#include "NSOptionWidget.h"

#include "CommonButtonBase.h"
#include "Components/WidgetSwitcher.h"
#include "SoundSetting/SoundSettingWidget.h"

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
}

void UNSOptionWidget::OnClickedSoundCategoryButton()
{
	ShowOptionCategoryWidget(SoundSettingWidget);
}

void UNSOptionWidget::OnClickedGraphicCategoryButton()
{
	ShowOptionCategoryWidget(GraphicSettingWidget);
}

void UNSOptionWidget::ShowOptionCategoryWidget(UWidget* OptionWidget)
{
	if (!OptionSwitcher || !OptionWidget)
	{
		return;
	}

	OptionSwitcher->SetActiveWidget(OptionWidget);
}
