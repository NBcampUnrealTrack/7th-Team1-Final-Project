// Copyright 2026 One Team. All rights reserved.

#include "NSOptionCategoryButton.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

UNSOptionCategoryButton::UNSOptionCategoryButton(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSelectable = true;
	bToggleable = true;

	// 선택 상태를 먼저 변경한 뒤 OnClicked를 호출하도록 한다.
	bTriggerClickedAfterSelection = true;
}

void UNSOptionCategoryButton::NativePreConstruct()
{
	Super::NativePreConstruct();

	ApplySelectionVisual(GetSelected());
}

void UNSOptionCategoryButton::NativeOnSelected(bool bBroadcast)
{
	Super::NativeOnSelected(bBroadcast);

	ApplySelectionVisual(true);
}

void UNSOptionCategoryButton::NativeOnDeselected(bool bBroadcast)
{
	Super::NativeOnDeselected(bBroadcast);

	ApplyTexture(NormalTexture.Get());

	if (CategoryBackgroundImage)
	{
		CategoryBackgroundImage->SetColorAndOpacity(
			IsHovered()
				? FLinearColor(0.0f, 0.8f, 1.0f, 1.0f)
				: FLinearColor::White);
	}
}

void UNSOptionCategoryButton::NativeOnHovered()
{
	Super::NativeOnHovered();

	if (!GetSelected() && CategoryBackgroundImage)
	{
		CategoryBackgroundImage->SetColorAndOpacity(HoverTint);
	}
}

void UNSOptionCategoryButton::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();

	if (CategoryBackgroundImage)
	{
		CategoryBackgroundImage->SetColorAndOpacity(
			FLinearColor::White);
	}

	ApplySelectionVisual(GetSelected());
}
void UNSOptionCategoryButton::ApplySelectionVisual(bool bInSelected)
{
	if (CategoryBackgroundImage)
	{
		CategoryBackgroundImage->SetColorAndOpacity(
			FLinearColor::White);
	}
	ApplyTexture(
		bInSelected
			? SelectedTexture.Get()
			: NormalTexture.Get());
}
void UNSOptionCategoryButton::ApplyTexture(UTexture2D* Texture)
{
	if (!CategoryBackgroundImage || !Texture)
	{
		return;
	}

	CategoryBackgroundImage->SetBrushFromTexture(
		Texture,
		false);
}
