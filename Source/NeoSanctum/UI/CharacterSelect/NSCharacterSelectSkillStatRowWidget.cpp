// Copyright 2026 One Team. All rights reserved.


#include "NSCharacterSelectSkillStatRowWidget.h"

#include "CommonTextBlock.h"
#include "Components/Widget.h"

void UNSCharacterSelectSkillStatRowWidget::SetStatData(
	const FText& InDisplayName, const FText& InValueText, bool bShowDivider)
{
	if (SkillStatNameText)
	{
		SkillStatNameText->SetText(InDisplayName);
	}

	if (SkillStatValueText)
	{
		SkillStatValueText->SetText(InValueText);
	}

	if (SkillStatDivider)
	{
		// 마지막 행 아래에는 구분선이 남지 않게 접어둠.
		SkillStatDivider->SetVisibility(bShowDivider ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
