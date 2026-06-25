// Copyright 2026 One Team. All rights reserved.


#include "NSCharacterSlotWidget.h"
#include "Components/TextBlock.h"

void UNSCharacterSlotWidget::SetCharacterData(const FNSCharacterSelectData& InData)
{
	CharacterData = InData;
	
	if (SlotNameText)
	{
		SlotNameText->SetText(CharacterData.CharacterName);
	}
}

void UNSCharacterSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
}
