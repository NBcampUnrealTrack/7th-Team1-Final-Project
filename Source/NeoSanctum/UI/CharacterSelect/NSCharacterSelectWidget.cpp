// Copyright 2026 One Team. All rights reserved.

#include "NSCharacterSelectWidget.h"
#include "CommonAnimatedSwitcher.h"
#include "CommonButtonBase.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"
#include "NeoSanctum/Data/UI/NSCharacterSelectData.h"
#include "NSCharacterSlotWidget.h"
#include "Components/Image.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"

void UNSCharacterSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	CachedCharacters.Reset();
	CurrentIndex = 0;

	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		CachedCharacters = DataSubsystem->GetCachedCharacterSelectRows();
	}

	if (NextButton)
	{
		NextButton->OnClicked().AddUObject(this, &UNSCharacterSelectWidget::SelectNext);
	}

	if (PrevButton)
	{
		PrevButton->OnClicked().AddUObject(this, &UNSCharacterSelectWidget::SelectPrev);
	}

	if (ConfirmButton)
	{
		ConfirmButton->OnClicked().AddUObject(this, &UNSCharacterSelectWidget::ConfirmSelection);
	}
	HandleCharacterChanged();
}


void UNSCharacterSelectWidget::SelectNext()
{
	if (CachedCharacters.IsEmpty()) { return; }

	CurrentIndex = (CurrentIndex + 1) % CachedCharacters.Num();
	FadeAndSwitch();
}

void UNSCharacterSelectWidget::SelectPrev()
{
	if (CachedCharacters.IsEmpty()) { return; }

	CurrentIndex = (CurrentIndex - 1 + CachedCharacters.Num()) % CachedCharacters.Num();
	FadeAndSwitch();
}

void UNSCharacterSelectWidget::FadeAndSwitch()
{
	APlayerCameraManager* CameraManager = GetOwningPlayer()->PlayerCameraManager;
	if (!CameraManager) { return; }

	CameraManager->StartCameraFade(0.0f, 1.0f, 0.3f, FLinearColor::Black, false, true);
	
	GetWorld()->GetTimerManager().SetTimer(FadeTimerHandle, this, &UNSCharacterSelectWidget::OnFadeOutFinished, 0.3f, false);
}

void UNSCharacterSelectWidget::OnFadeOutFinished()
{
	if (CachedCharacters.IsEmpty()) { return; }

	if (!CachedCharacters.IsValidIndex(CurrentIndex))
	{
		return;
	}
	const FNSCharacterSelectData& Data = CachedCharacters[CurrentIndex];
	
	ApplyPreviewImage(Data);
	
	if (CharacterNameText)
	{
		CharacterNameText->SetText(Data.CharacterName);
	}

	if (CharacterDescriptionText)
	{
		CharacterDescriptionText->SetText(Data.CharacterDescription);
	}

	if (CharacterSwitcher)
	{
		CharacterSwitcher->SetActiveWidgetIndex(CurrentIndex);

		UNSCharacterSlotWidget* CurrentSlot = 
			Cast<UNSCharacterSlotWidget>(CharacterSwitcher->GetWidgetAtIndex(CurrentIndex));
		if (CurrentSlot)
		{
			CurrentSlot->SetCharacterData(Data);
		}
	}

	APlayerCameraManager* CameraManager = GetOwningPlayer()->PlayerCameraManager;
	if (CameraManager)
	{
		CameraManager->StartCameraFade(1.0f, 0.0f, 0.3f, FLinearColor::Black, false, false);
	}
}

void UNSCharacterSelectWidget::HandleCharacterChanged()
{
	if (CachedCharacters.IsEmpty()) { return; }

	if (!CachedCharacters.IsValidIndex(CurrentIndex))
	{
		return;
	}

	const FNSCharacterSelectData& Data = CachedCharacters[CurrentIndex];

	ApplyPreviewImage(Data);

	if (CharacterNameText)
	{
		CharacterNameText->SetText(Data.CharacterName);
	}

	if (CharacterDescriptionText)
	{
		CharacterDescriptionText->SetText(Data.CharacterDescription);
	}

	if (CharacterSwitcher && CurrentIndex < CharacterSwitcher->GetChildrenCount())
	{
		CharacterSwitcher->SetActiveWidgetIndex(CurrentIndex);
	}
}

void UNSCharacterSelectWidget::ConfirmSelection()
{
	if (!CachedCharacters.IsValidIndex(CurrentIndex))
	{
		return;
	}

	const FNSCharacterSelectData& SelectedData = CachedCharacters[CurrentIndex];

	UNSCharacterData* SelectedCharacterData = SelectedData.CharacterData.Get();
	if (!SelectedCharacterData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CharacterSelect] 선택한 캐릭터 데이터가 로드되어 있지 않습니다."));
		return;
	}

	OnCharacterSelectionConfirmed.Broadcast(SelectedCharacterData);
}
void UNSCharacterSelectWidget::ApplyPreviewImage(const FNSCharacterSelectData& Data)
{
	if (!PreviewImage)
	{
		return;
	}
	
	UTexture2D* Texture = Data.PreviewTexture.Get();
	if (!Texture)
	{
		PreviewImage->SetBrushFromTexture(nullptr);
		PreviewImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	PreviewImage->SetBrushFromTexture(Texture);
	PreviewImage->SetVisibility(ESlateVisibility::Visible);
}
