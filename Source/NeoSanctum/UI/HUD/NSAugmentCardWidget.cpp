// Copyright 2026 One Team. All rights reserved.


#include "NSAugmentCardWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "VerseVM/VVMRuntimeError.h"

void UNSAugmentCardWidget::SetAugmentName(const FString& NewName)
{
	//증강 이름 텍스트 갱신
	if (!AugmentNameText)
	{
		return;
	}
	AugmentNameText->SetText(FText::FromString(NewName));
}

void UNSAugmentCardWidget::SetAugmentDescription(const FString& NewDescription)
{
	//증강 설명 텍스트 갱신
	if (!AugmentDescriptionText)
	{
		return;
	}
	AugmentDescriptionText->SetText(FText::FromString(NewDescription));
}

void UNSAugmentCardWidget::SetHighLighted(bool bHighLighted)
{
	//카드 선택 상태에 따라 테두리 색상 변경
	if (!CardBorder)
	{
		return;
	}
	
	if (bHighLighted)
	{
		//선택된 카드 강조 색상
		CardBorder->SetBrushColor(FLinearColor(1.0f,0.8f,0.2f,1.0f));
	}
	else
	{
		//기본 카드 색상
		CardBorder->SetBrushColor(FLinearColor(0.1f,0.1f,0.1f,1.0f));
	}
}

void UNSAugmentCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	
	
	//기본 상태는 하이라트 비활성화
	SetHighLighted(false);
}
