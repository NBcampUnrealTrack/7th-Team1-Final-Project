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
#include "NeoSanctum/Data/Character/NSCharacterBaseStatTypes.h"

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
	APlayerController* OwningPlayer = GetOwningPlayer();
	APlayerCameraManager* CameraManager =
		OwningPlayer ? OwningPlayer->PlayerCameraManager : nullptr;
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
	UpdateBaseStatTexts(Data);
	
	if (CharacterNameText)
	{
		CharacterNameText->SetText(Data.CharacterName);
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

	APlayerController* OwningPlayer = GetOwningPlayer();
	APlayerCameraManager* CameraManager =
		OwningPlayer ? OwningPlayer->PlayerCameraManager : nullptr;
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
	UpdateBaseStatTexts(Data);

	if (CharacterNameText)
	{
		CharacterNameText->SetText(Data.CharacterName);
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

void UNSCharacterSelectWidget::UpdateBaseStatTexts(const FNSCharacterSelectData& Data)
{
	const UNSCharacterData* CharacterData = Data.CharacterData.Get();
	if (!CharacterData || !CharacterData->CharacterTag.IsValid())
	{
		ClearBaseStatTexts();
		return;
	}

	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	const FNSCharacterBaseStatRow* StatRow =
		DataSubsystem
			? DataSubsystem->FindCharacterBaseStatRow(CharacterData->CharacterTag)
			: nullptr;

	if (!StatRow)
	{
		ClearBaseStatTexts();
		return;
	}

	if (MaxHealthText)
	{
		MaxHealthText->SetText(FText::Format(
			NSLOCTEXT("CharacterSelect", "MaxHealthFormat", "체력: {0}"),
			FText::AsNumber(StatRow->MaxHealth)));
	}

	if (BaseDamageText)
	{
		BaseDamageText->SetText(FText::Format(
			NSLOCTEXT("CharacterSelect", "BaseDamageFormat", "공격력: {0}"),
			FText::AsNumber(StatRow->BaseDamage)));
	}

	if (DefenseText)
	{
		DefenseText->SetText(FText::Format(
			NSLOCTEXT("CharacterSelect", "DefenseFormat", "방어력: {0}"),
			FText::AsNumber(StatRow->Defense)));
	}

	if (MoveSpeedText)
	{
		MoveSpeedText->SetText(FText::Format(
			NSLOCTEXT("CharacterSelect", "MoveSpeedFormat", "이동속도: {0}"),
			FText::AsNumber(StatRow->MoveSpeed)));
	}

	if (CritChanceText)
	{
		CritChanceText->SetText(FText::Format(
			NSLOCTEXT("CharacterSelect", "CritChanceFormat", "치명타 확률: {0}%"),
			FText::AsNumber(StatRow->CritChance)));
	}

	if (CritDamageText)
	{
		CritDamageText->SetText(FText::Format(
			NSLOCTEXT("CharacterSelect", "CritDamageFormat", "치명타 피해: {0}%"),
			FText::AsNumber(StatRow->CritDamage)));
	}

	if (MaxShieldText)
	{
		MaxShieldText->SetText(FText::Format(
			NSLOCTEXT("CharacterSelect", "MaxShieldFormat", "보호막: {0}"),
			FText::AsNumber(StatRow->MaxShield)));
	}
}

void UNSCharacterSelectWidget::ClearBaseStatTexts()
{
	const FText EmptyText = FText::GetEmpty();

	if (MaxHealthText)
	{
		MaxHealthText->SetText(EmptyText);
	}

	if (BaseDamageText)
	{
		BaseDamageText->SetText(EmptyText);
	}

	if (DefenseText)
	{
		DefenseText->SetText(EmptyText);
	}

	if (MoveSpeedText)
	{
		MoveSpeedText->SetText(EmptyText);
	}

	if (CritChanceText)
	{
		CritChanceText->SetText(EmptyText);
	}

	if (CritDamageText)
	{
		CritDamageText->SetText(EmptyText);
	}

	if (MaxShieldText)
	{
		MaxShieldText->SetText(EmptyText);
	}
}
