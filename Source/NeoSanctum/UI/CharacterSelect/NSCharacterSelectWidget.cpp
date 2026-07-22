// Copyright 2026 One Team. All rights reserved.

#include "NSCharacterSelectWidget.h"
#include "CommonAnimatedSwitcher.h"
#include "NeoSanctum/UI/Common/NSButtonBase.h"
#include "CommonTextBlock.h"
#include "NSCharacterSelectSkillStatRowWidget.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"
#include "NeoSanctum/Data/UI/NSCharacterSelectData.h"
#include "NSCharacterSlotWidget.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/Character/NSCharacterBaseStatTypes.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "NeoSanctum/Data/UI/NSCharacterSkillUISet.h"
#include "NeoSanctum/Data/UI/NSSkillUIData.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"

namespace
{
	FText FormatSkillStatValue(float DisplayValue, ENSSkillStatValueFormat ValueFormat)
	{
		FNumberFormattingOptions NumberOptions;
		NumberOptions.MinimumFractionalDigits = 0;
		NumberOptions.MaximumFractionalDigits = 2;

		const FText NumberText = FText::AsNumber(DisplayValue, &NumberOptions);

		switch (ValueFormat)
		{
		case ENSSkillStatValueFormat::Percentage:
			return FText::Format(NSLOCTEXT("CharacterSelect", "SkillStatPercentage", "{0}%"), NumberText);

		case ENSSkillStatValueFormat::Seconds:
			return FText::Format(NSLOCTEXT("CharacterSelect", "SkillStatSeconds", "{0}초"), NumberText);

		case ENSSkillStatValueFormat::ShotsPerSecond:
			return FText::Format(NSLOCTEXT("CharacterSelect", "SkillStatShotsPerSecond", "초당 {0}발"), NumberText);

		case ENSSkillStatValueFormat::Meters:
			return FText::Format(NSLOCTEXT("CharacterSelect", "SkillStatMeters", "{0} m"), NumberText);

		case ENSSkillStatValueFormat::Number:
		default:
			return NumberText;
		}
	}
}

void UNSCharacterSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// UIOnly 입력 모드에서도 이 위젯이 ESC 키를 받을 수 있게 함.
	SetIsFocusable(true);
	
	CachedCharacters.Reset();
	CurrentIndex = 0;

	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		CachedCharacters = DataSubsystem->GetCachedCharacterSelectRows();
		UE_LOG(LogTemp, Warning, TEXT("[CharSelect] Construct Num=%d"), CachedCharacters.Num());
	}
	
	CurrentIndex = FindInitialCharacterIndex();
 
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

	if (CloseButton)
	{
		// 위젯이 다시 Construct되어도 닫기 이벤트가 중복되지 않게 정리함.
		CloseButton->OnClicked().RemoveAll(this);
		CloseButton->OnClicked().AddUObject(this, &ThisClass::HandleCloseButtonClicked);
	}

	BindSkillSlotEvents();
	HandleCharacterChanged();
}

void UNSCharacterSelectWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	// 위젯을 다시 열면 기본 공격 상세정보로 돌아감.
	PreviewSkillSlot(ENSCharacterSelectSkillSlot::BaseAttack);
}

FReply UNSCharacterSelectWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		// UIOnly 입력 모드에서는 위젯이 ESC를 직접 받아서 닫아야 함.
		HandleCloseButtonClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UNSCharacterSelectWidget::HandleCloseButtonClicked()
{
	ANSPlayerController* PlayerController = Cast<ANSPlayerController>(GetOwningPlayer());

	if (!PlayerController)
	{
		NS_OBJ_LOG(LogNS, Warning,
			"[CharacterSelect] 닫기 실패: 소유 PlayerController가 유효하지 않습니다."
		);
		return;
	}

	// 캐릭터를 확정하지 않고 창과 입력 상태만 원래대로 되돌림.
	PlayerController->HideCharacterSelectWidget();
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
	RefreshSkillSection(Data);

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

	if (CharacterDescriptionText)
	{
		CharacterDescriptionText->SetText(Data.CharacterDescription);
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
	RefreshSkillSection(Data);

	if (CharacterNameText)
	{
		CharacterNameText->SetText(Data.CharacterName);
	}

	if (CharacterSwitcher && CurrentIndex < CharacterSwitcher->GetChildrenCount())
	{
		CharacterSwitcher->SetActiveWidgetIndex(CurrentIndex);
	}

	if (CharacterDescriptionText)
	{
		CharacterDescriptionText->SetText(Data.CharacterDescription);
	}
}

void UNSCharacterSelectWidget::BindSkillSlotEvents()
{
	UNSCharacterSelectSkillSlotWidget* SkillSlots[] =
	{
		BaseAttackSlot.Get(),
		Skill1Slot.Get(),
		Skill2Slot.Get(),
		Skill3Slot.Get()
	};

	for (UNSCharacterSelectSkillSlotWidget* SkillSlotWidget : SkillSlots)
	{
		if (!SkillSlotWidget)
		{
			continue;
		}

		// 위젯이 다시 Construct되어도 같은 이벤트가 중복되지 않게 정리.
		SkillSlotWidget->OnSlotHovered.RemoveAll(this);
		SkillSlotWidget->OnSlotClicked.RemoveAll(this);

		SkillSlotWidget->OnSlotHovered.AddUObject(this, &ThisClass::HandleSkillSlotHovered);
		SkillSlotWidget->OnSlotClicked.AddUObject(this, &ThisClass::HandleSkillSlotClicked);
	}

	// 호버가 끝나도 최근 상세정보를 유지하므로 Unhovered는 연결하지 않음.
}

void UNSCharacterSelectWidget::RefreshSkillSection(const FNSCharacterSelectData& Data)
{
	const UNSCharacterData* CharacterData = Data.CharacterData.Get();
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);

	if (!CharacterData || !CharacterData->CharacterTag.IsValid() || !DataSubsystem)
	{
		ClearSkillSection();
		return;
	}

	UDataTable* SkillSetTable = DataSubsystem->GetCommonCharacterSkillUISetTable();

	if (!SkillSetTable || SkillSetTable->GetRowStruct() != FNSCharacterSkillUISet::StaticStruct())
	{
		ClearSkillSection();
		return;
	}

	const FName SkillSetRowName = CharacterData->CharacterTag.GetTagName();

	const FNSCharacterSkillUISet* SkillSet =
		SkillSetTable->FindRow<FNSCharacterSkillUISet>(
			SkillSetRowName, TEXT("CharacterSelectSkillSet"), false);

	if (!SkillSet)
	{
		NS_OBJ_LOG(LogNS, Warning,
			"[CharacterSelect] 스킬 UI Set을 찾을 수 없습니다. CharacterTag={CharacterTag}",
			("CharacterTag", CharacterData->CharacterTag.ToString())
		);

		ClearSkillSection();
		return;
	}

	if (BaseAttackSlot)
	{
		BaseAttackSlot->SetupSlot(
			ENSCharacterSelectSkillSlot::BaseAttack,
			SkillSet->BaseAttackUIDataRow,
			SkillSet->BaseAttackInputDisplay
		);
	}

	if (Skill1Slot)
	{
		Skill1Slot->SetupSlot(
			ENSCharacterSelectSkillSlot::Skill1,
			SkillSet->Skill1UIDataRow,
			SkillSet->Skill1InputDisplay
		);
	}

	if (Skill2Slot)
	{
		Skill2Slot->SetupSlot(
			ENSCharacterSelectSkillSlot::Skill2,
			SkillSet->Skill2UIDataRow,
			SkillSet->Skill2InputDisplay
		);
	}

	if (Skill3Slot)
	{
		Skill3Slot->SetupSlot(
			ENSCharacterSelectSkillSlot::Skill3,
			SkillSet->Skill3UIDataRow,
			SkillSet->Skill3InputDisplay
		);
	}

	// 캐릭터가 바뀌면 항상 좌클릭 기본 공격부터 보여줌.
	PreviewSkillSlot(ENSCharacterSelectSkillSlot::BaseAttack);
}

void UNSCharacterSelectWidget::ClearSkillSection()
{
	if (BaseAttackSlot)
	{
		BaseAttackSlot->ResetSlot();
	}

	if (Skill1Slot)
	{
		Skill1Slot->ResetSlot();
	}

	if (Skill2Slot)
	{
		Skill2Slot->ResetSlot();
	}

	if (Skill3Slot)
	{
		Skill3Slot->ResetSlot();
	}

	PreviewedSkillSlot = ENSCharacterSelectSkillSlot::BaseAttack;
	bHasPreviewedSkillSlot = false;

	ClearSkillDetailPanel();
}

void UNSCharacterSelectWidget::HandleSkillSlotHovered(ENSCharacterSelectSkillSlot SlotType)
{
	if (SkillPreviewMode == ENSCharacterSelectSkillPreviewMode::Hover)
	{
		PreviewSkillSlot(SlotType);
	}
}

void UNSCharacterSelectWidget::HandleSkillSlotClicked(ENSCharacterSelectSkillSlot SlotType)
{
	if (SkillPreviewMode == ENSCharacterSelectSkillPreviewMode::Click)
	{
		PreviewSkillSlot(SlotType);
	}
}

void UNSCharacterSelectWidget::PreviewSkillSlot(ENSCharacterSelectSkillSlot SlotType)
{
	UNSCharacterSelectSkillSlotWidget* TargetSlot = GetSkillSlotWidget(SlotType);

	if (!TargetSlot)
	{
		bHasPreviewedSkillSlot = false;
		UpdateSkillSlotPreviewIndicators();
		ClearSkillDetailPanel();
		return;
	}

	const FDataTableRowHandle& SkillUIDataRow = TargetSlot->GetSkillUIDataRow();

	if (!SkillUIDataRow.DataTable || SkillUIDataRow.RowName.IsNone())
	{
		bHasPreviewedSkillSlot = false;
		UpdateSkillSlotPreviewIndicators();
		ClearSkillDetailPanel();
		return;
	}

	PreviewedSkillSlot = SlotType;
	bHasPreviewedSkillSlot = true;

	UpdateSkillSlotPreviewIndicators();
	UpdateSkillDetailPanel(SkillUIDataRow, TargetSlot->GetInputIconTexture());
}

void UNSCharacterSelectWidget::UpdateSkillSlotPreviewIndicators()
{
	if (BaseAttackSlot)
	{
		BaseAttackSlot->SetSlotPreviewed(
			bHasPreviewedSkillSlot && PreviewedSkillSlot == ENSCharacterSelectSkillSlot::BaseAttack);
	}

	if (Skill1Slot)
	{
		Skill1Slot->SetSlotPreviewed(
			bHasPreviewedSkillSlot && PreviewedSkillSlot == ENSCharacterSelectSkillSlot::Skill1);
	}

	if (Skill2Slot)
	{
		Skill2Slot->SetSlotPreviewed(
			bHasPreviewedSkillSlot && PreviewedSkillSlot == ENSCharacterSelectSkillSlot::Skill2);
	}

	if (Skill3Slot)
	{
		Skill3Slot->SetSlotPreviewed(
			bHasPreviewedSkillSlot && PreviewedSkillSlot == ENSCharacterSelectSkillSlot::Skill3);
	}
}

UNSCharacterSelectSkillSlotWidget* UNSCharacterSelectWidget::GetSkillSlotWidget(
	ENSCharacterSelectSkillSlot SlotType) const
{
	switch (SlotType)
	{
	case ENSCharacterSelectSkillSlot::BaseAttack:
		return BaseAttackSlot.Get();

	case ENSCharacterSelectSkillSlot::Skill1:
		return Skill1Slot.Get();

	case ENSCharacterSelectSkillSlot::Skill2:
		return Skill2Slot.Get();

	case ENSCharacterSelectSkillSlot::Skill3:
		return Skill3Slot.Get();

	default:
		return nullptr;
	}
}

void UNSCharacterSelectWidget::UpdateSkillDetailPanel(
	const FDataTableRowHandle& SkillUIDataRow, UTexture2D* InputIconTexture)
{
	const FNSSkillUIData* SkillUIData =
		SkillUIDataRow.GetRow<FNSSkillUIData>(TEXT("CharacterSelectSkillDetail"));

	if (!SkillUIData)
	{
		ClearSkillDetailPanel();
		return;
	}

	if (SkillDetailNameText)
	{
		SkillDetailNameText->SetText(SkillUIData->DisplayName);
	}

	if (SkillDetailDescriptionText)
	{
		SkillDetailDescriptionText->SetText(SkillUIData->Description);
	}

	if (SkillDetailIconImage)
	{
		// 아래 슬롯과 같은 텍스쳐 객체를 사용하므로 메모리가 중복되지 않음.
		UTexture2D* SkillTexture = SkillUIData->CharacterSelectIcon.Get();

		SkillDetailIconImage->SetBrushFromTexture(SkillTexture);
		SkillDetailIconImage->SetVisibility(
			SkillTexture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (SkillDetailInputIconImage)
	{
		SkillDetailInputIconImage->SetBrushFromTexture(InputIconTexture);
		SkillDetailInputIconImage->SetVisibility(
			InputIconTexture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	RefreshSkillDetailStats(*SkillUIData);
}

void UNSCharacterSelectWidget::RefreshSkillDetailStats(const FNSSkillUIData& SkillUIData)
{
	if (!SkillDetailStatsBox)
	{
		return;
	}

	// 이전에 보고 있던 스킬의 행이 남지 않게 먼저 비움.
	SkillDetailStatsBox->ClearChildren();

	if (!SkillStatRowWidgetClass)
	{
		return;
	}

	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	const int32 StatCount = SkillUIData.CharacterSelectStats.Num();

	for (int32 StatIndex = 0; StatIndex < StatCount; ++StatIndex)
	{
		const FNSSkillStatDisplayData& DisplayData = SkillUIData.CharacterSelectStats[StatIndex];

		const FNSAbilityBaseStatRow* StatRow =
			DataSubsystem ? DataSubsystem->FindAbilityBaseStatRow(SkillUIData.SkillTag, DisplayData.StatTag) : nullptr;

		FText ValueText = NSLOCTEXT("CharacterSelect", "MissingSkillStatValue", "-");

		if (StatRow)
		{
			const float DisplayValue = StatRow->BaseValue * DisplayData.DisplayScale;

			ValueText = FormatSkillStatValue(DisplayValue, DisplayData.ValueFormat);
		}
		else
		{
			const FString WarningKey = FString::Printf(
				TEXT("%s|%s"),
				*SkillUIData.SkillTag.ToString(),
				*DisplayData.StatTag.ToString()
			);

			if (!LoggedMissingSkillStatKeys.Contains(WarningKey))
			{
				LoggedMissingSkillStatKeys.Add(WarningKey);

				NS_OBJ_LOG(LogNS, Warning,
					"[CharacterSelect] 스킬 기본 스탯을 찾을 수 없습니다. "
					"AbilityTag={AbilityTag}, StatTag={StatTag}",
					("AbilityTag", SkillUIData.SkillTag.ToString()),
					("StatTag", DisplayData.StatTag.ToString())
				);
			}
		}

		UNSCharacterSelectSkillStatRowWidget* StatRowWidget =
			CreateWidget<UNSCharacterSelectSkillStatRowWidget>(this, SkillStatRowWidgetClass);

		if (!StatRowWidget)
		{
			continue;
		}

		// 마지막 스탯 아래에는 구분선을 표시하지 않음.
		const bool bShowDivider = StatIndex < StatCount - 1;

		StatRowWidget->SetStatData(DisplayData.DisplayName, ValueText, bShowDivider);

		SkillDetailStatsBox->AddChildToVerticalBox(StatRowWidget);
	}
}

void UNSCharacterSelectWidget::ClearSkillDetailPanel()
{
	const FText EmptyText = FText::GetEmpty();

	if (SkillDetailNameText)
	{
		SkillDetailNameText->SetText(EmptyText);
	}

	if (SkillDetailDescriptionText)
	{
		SkillDetailDescriptionText->SetText(EmptyText);
	}

	if (SkillDetailStatsBox)
	{
		// 위젯을 닫거나 데이터가 없을 때 이전 슽새 행을 모두 제거.
		SkillDetailStatsBox->ClearChildren();
	}

	if (SkillDetailIconImage)
	{
		SkillDetailIconImage->SetBrushFromTexture(nullptr);
		SkillDetailIconImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (SkillDetailInputIconImage)
	{
		SkillDetailInputIconImage->SetBrushFromTexture(nullptr);
		SkillDetailInputIconImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSCharacterSelectWidget::ConfirmSelection()
{
	UE_LOG(LogTemp, Warning, TEXT("[CharSelect] Confirm 진입 Num=%d Index=%d"),
	   CachedCharacters.Num(), CurrentIndex);
	
	if (!CachedCharacters.IsValidIndex(CurrentIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CharSelect] return: 인덱스 무효(목록 빔/범위 밖)"));
		return;
	}

	const FNSCharacterSelectData& SelectedData = CachedCharacters[CurrentIndex];

	UNSCharacterData* SelectedCharacterData = SelectedData.CharacterData.Get();
	if (!SelectedCharacterData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CharSelect] return: CharacterData.Get() null"));
		UE_LOG(LogTemp, Warning, TEXT("[CharacterSelect] 선택한 캐릭터 데이터가 로드되어 있지 않습니다."));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[CharSelect] Broadcast %s"), *SelectedCharacterData->GetName());
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

	// 라벨은 WBP에서 고정으로 보여주고, 여기서는 캐릭터마다 바뀌는 값만 전달합니다.
	if (MaxHealthText)
	{
		MaxHealthText->SetText(FText::AsNumber(StatRow->MaxHealth));
	}

	if (MaxShieldText)
	{
		MaxShieldText->SetText(FText::AsNumber(StatRow->MaxShield));
	}

	if (BaseDamageText)
	{
		BaseDamageText->SetText(FText::AsNumber(StatRow->BaseDamage));
	}

	if (DefenseText)
	{
		DefenseText->SetText(FText::AsNumber(StatRow->Defense));
	}

	if (MoveSpeedText)
	{
		MoveSpeedText->SetText(FText::AsNumber(StatRow->MoveSpeed));
	}

	if (CritChanceText)
	{
		CritChanceText->SetText(FText::Format(
			NSLOCTEXT("CharacterSelect", "CritChanceValueFormat", "{0}%"),
			FText::AsNumber(StatRow->CritChance)));
	}

	if (CritDamageText)
	{
		CritDamageText->SetText(FText::Format(
			NSLOCTEXT("CharacterSelect", "CritDamageValueFormat", "{0}%"),
			FText::AsNumber(StatRow->CritDamage)));
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

int32 UNSCharacterSelectWidget::FindInitialCharacterIndex() const
{
	if (CachedCharacters.IsEmpty())
	{
		return 0;
	}

	// 현재 선택된 캐릭터 id 확보 (서버 권위 값 우선)
	FPrimaryAssetId CurrentId;
	if (const APlayerController* PC = GetOwningPlayer())
	{
		if (const ANSPlayerState* PS = PC->GetPlayerState<ANSPlayerState>())
		{
			CurrentId = PS->GetCurrentCharacterDataId();
		}
	}
	
	// 현재 캐릭터 정보 없으면 첫 번째
	if (!CurrentId.IsValid())
	{
		return 0;  
	}

	// 목록에서 같은 캐릭터를 찾는다
	for (int32 Index = 0; Index < CachedCharacters.Num(); ++Index)
	{
		const TSoftObjectPtr<UNSCharacterData>& SoftData = CachedCharacters[Index].CharacterData;
		if (SoftData.IsNull())
		{
			continue;
		}

		// 소프트 레퍼런스는 로드 없이도 경로에서 PrimaryAssetId를 만들 수 있다
		const FPrimaryAssetId RowId(
			UNSDataSubsystem::CharacterAssetType,
			FName(*SoftData.GetAssetName()));

		if (RowId == CurrentId)
		{
			return Index;
		}
	}

	return 0; 
}
