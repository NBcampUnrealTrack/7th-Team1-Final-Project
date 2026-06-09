// Copyright 2026 One Team. All rights reserved.

#include "NSCharacterSelectWidget.h"
#include "CommonAnimatedSwitcher.h"
#include "CommonButtonBase.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "NeoSanctum/Data/UI/NSCharacterSelectData.h"
#include "GameFramework/PlayerController.h"
#include "NSCharacterSlotWidget.h"

void UNSCharacterSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CharacterDataTable)
	{
		CharacterDataTable->GetAllRows<FNSCharacterSelectData>(TEXT("CharacterSelect"), CachedCharacters);
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

void UNSCharacterSelectWidget::SetPreviewActor(ACharacter* InActor)
{
	PreviewActor = InActor;
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

	const FNSCharacterSelectData* Data = CachedCharacters[CurrentIndex];
	if (!Data) { return; }

	// DataAsset 로드 후 메시 교체
	UNSCharacterData* CharData = Data->CharacterData.LoadSynchronous();
	if (PreviewActor && CharData)
	{
		USkeletalMeshComponent* MeshComp = PreviewActor->GetMesh();
		if (MeshComp)
		{
			USkeletalMesh* Mesh = CharData->SkeletalMesh.LoadSynchronous();
			if (Mesh)
			{
				MeshComp->SetSkeletalMesh(Mesh);
			}
		}
	}

	if (CharacterNameText)
	{
		CharacterNameText->SetText(Data->CharacterName);
	}

	if (CharacterDescriptionText)
	{
		CharacterDescriptionText->SetText(Data->CharacterDescription);
	}

	if (CharacterSwitcher)
	{
		CharacterSwitcher->SetActiveWidgetIndex(CurrentIndex);

		UNSCharacterSlotWidget* CurrentSlot = Cast<UNSCharacterSlotWidget>(CharacterSwitcher->GetWidgetAtIndex(CurrentIndex));
		if (CurrentSlot)
		{
			CurrentSlot->SetCharacterData(*Data);
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

	const FNSCharacterSelectData* Data = CachedCharacters[CurrentIndex];
	if (!Data) { return; }

	if (CharacterNameText)
	{
		CharacterNameText->SetText(Data->CharacterName);
	}

	if (CharacterDescriptionText)
	{
		CharacterDescriptionText->SetText(Data->CharacterDescription);
	}

	if (CharacterSwitcher)
	{
		CharacterSwitcher->SetActiveWidgetIndex(CurrentIndex);
	}
}

void UNSCharacterSelectWidget::ConfirmSelection()
{
	// TODO(담당자): GameInstance 연동 후 구현
}