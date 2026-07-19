// Copyright 2026 One Team. All rights reserved.

#include "NSPartPanelWidget.h"
#include "Components/PanelWidget.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Progression/Part/NSPartEquipComponent.h"
#include "NeoSanctum/Progression/Part/NSPartUtils.h"
#include "NeoSanctum/UI/Part/Button/NSPartSlotButton.h"
#include "NeoSanctum/UI/HUD/NSCharacterStatsBridgeSubsystem.h"

void UNSPartPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(this);
	if (!DataSS)
	{
		return;
	}

	if (!DataSS->GetAllSlotRows().IsEmpty())
	{
		BuildSlotButtons();
		BindPartEquipComponent();
		RefreshEquippedParts();
	}
	else
	{
		DataSS->OnOutGameDataReady.AddDynamic(this, &UNSPartPanelWidget::OnOutGameDataReady);
	}
}

void UNSPartPanelWidget::NativeDestruct()
{
	if (UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(this))
	{
		DataSS->OnOutGameDataReady.RemoveDynamic(this, &UNSPartPanelWidget::OnOutGameDataReady);
	}

	UnbindPartEquipComponent();

	Super::NativeDestruct();
}

void UNSPartPanelWidget::OnOutGameDataReady()
{
	BuildSlotButtons();
	BindPartEquipComponent();
	RefreshEquippedParts();
}

void UNSPartPanelWidget::RefreshEquippedParts()
{
	for (const auto& Pair : SlotButtonMap)
	{
		ApplySlot(Pair.Key, Pair.Value);
	}
}

UNSPartEquipComponent* UNSPartPanelWidget::GetPartEquipComponent() const
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	ANSPlayerState* NSPlayerState = PlayerController->GetPlayerState<ANSPlayerState>();
	if (!IsValid(NSPlayerState))
	{
		return nullptr;
	}

	return NSPlayerState->GetPartEquipComponent();
}

void UNSPartPanelWidget::BindPartEquipComponent()
{
	UNSPartEquipComponent* PartEquipComponent = GetPartEquipComponent();
	if (!IsValid(PartEquipComponent))
	{
		return;
	}

	CachedPartEquipComponent = PartEquipComponent;

	//파츠 컴포넌트는 일반 C++ 멀티캐스트 델리게이트를 사용하므로 AddUObject로 구독한다.
	PartEquipComponent->OnPartChanged.AddUObject(this, &UNSPartPanelWidget::HandlePartChanged);
}

void UNSPartPanelWidget::UnbindPartEquipComponent()
{
	if (!CachedPartEquipComponent.IsValid())
	{
		return;
	}

	CachedPartEquipComponent->OnPartChanged.RemoveAll(this);
	CachedPartEquipComponent.Reset();
}

void UNSPartPanelWidget::BuildSlotButtons()
{
	if (!SlotButtonContainer || !SlotButtonTemplate)
	{
		return;
	}

	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(this);
	if (!DataSS)
	{
		return;
	}

	// TMap 순회 순서는 보장되지 않으므로 DT의 SortOrder 기준으로 정렬 후 생성 (바디 > 암 > 레그)
	TArray<TPair<FGameplayTag, FNSPartSlotRow>> SortedRows;
	for (const auto& Pair : DataSS->GetAllSlotRows())
	{
		SortedRows.Add(Pair);
	}
	SortedRows.Sort([](const TPair<FGameplayTag, FNSPartSlotRow>& A, const TPair<FGameplayTag, FNSPartSlotRow>& B)
	{
		return A.Value.SortOrder < B.Value.SortOrder;
	});

	for (const auto& Pair : SortedRows)
	{
		UNSPartSlotButton* Btn = CreateWidget<UNSPartSlotButton>(this, SlotButtonTemplate);
		if (!Btn)
		{
			continue;
		}
		SlotButtonContainer->AddChild(Btn);
		SlotButtonMap.Add(Pair.Key, Btn);
	}
}

void UNSPartPanelWidget::ApplySlot(FGameplayTag PartSlot, UNSPartSlotButton* SlotButton)
{
	if (!IsValid(SlotButton))
	{
		return;
	}

	UNSPartEquipComponent* PartEquipComponent = GetPartEquipComponent();
	if (!IsValid(PartEquipComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PartPanel] ApplySlot: PartEquipComponent 없음 (Slot=%s)"), *PartSlot.ToString());
		SlotButton->ClearPart();
		return;
	}

	const FNSPartData* PartData = PartEquipComponent->GetEquippedPart(PartSlot);
	if (PartData == nullptr || !PartData->IsValid())
	{
		SlotButton->ClearPart();
		return;
	}

	// Definition 로드는 파츠 시스템 공통 유틸을 통해 처리해 UI와 시스템의 조회 방식을 맞춘다.
	UNSPartDefinition* PartDefinition = NSPartUtils::ResolvePartDefinition(this, *PartData);
	if (!IsValid(PartDefinition))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PartPanel] ApplySlot: Definition null → ClearPart (Slot=%s, Def=%s)"),
			*PartSlot.ToString(), *PartData->DefinitionPtr.ToString());
		SlotButton->ClearPart();
		return;
	}
	SlotButton->SetPart(*PartData, PartDefinition);
}

void UNSPartPanelWidget::HandlePartChanged(FGameplayTag PartSlot, const FNSPartData& PartData)
{
	TObjectPtr<UNSPartSlotButton>* Found = SlotButtonMap.Find(PartSlot);
	if (Found && IsValid(*Found))
	{
		ApplySlot(PartSlot, *Found);
	}
	
	if (UNSCharacterStatsBridgeSubsystem* StatsBridge =
	GetGameInstance()->GetSubsystem<UNSCharacterStatsBridgeSubsystem>())
	{
		StatsBridge->BroadcastCharacterStats(GetOwningPlayer());
	}
}
