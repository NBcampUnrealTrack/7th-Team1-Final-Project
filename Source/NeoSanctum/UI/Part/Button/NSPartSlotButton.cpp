// Copyright 2026 One Team. All rights reserved.


#include "NSPartSlotButton.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/AssetManager.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Progression/Part/NSPartUtils.h"

UNSPartSlotButton::UNSPartSlotButton()
	: bHasPart(false)
{
}

void UNSPartSlotButton::NativePreConstruct()
{
	Super::NativePreConstruct();

	RefreshEmptyState();
}

void UNSPartSlotButton::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(HoverHighlight))
	{
		HoverHighlight->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (IsValid(PressedHighlight))
	{
		PressedHighlight->SetVisibility(ESlateVisibility::Collapsed);
	}

	OnHovered().AddUObject(this, &ThisClass::HandleHovered);
	OnUnhovered().AddUObject(this, &ThisClass::HandleUnhovered);
	OnPressed().AddUObject(this, &ThisClass::HandlePressed);
	OnReleased().AddUObject(this, &ThisClass::HandleReleased);
}

void UNSPartSlotButton::NativeDestruct()
{
	OnHovered().RemoveAll(this);
	OnUnhovered().RemoveAll(this);
	OnPressed().RemoveAll(this);
	OnReleased().RemoveAll(this);

	Super::NativeDestruct();
}

void UNSPartSlotButton::HandleHovered()
{
	if (IsValid(HoverHighlight))
	{
		HoverHighlight->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UNSPartSlotButton::HandleUnhovered()
{
	if (IsValid(HoverHighlight))
	{
		HoverHighlight->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSPartSlotButton::HandlePressed()
{
	if (IsValid(PressedHighlight))
	{
		PressedHighlight->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UNSPartSlotButton::HandleReleased()
{
	// 선택된 상태(bIsHighlighted)면 눌림 이미지를 계속 유지하고, 아니면 원래대로 해제
	if (IsValid(PressedHighlight) && !bIsHighlighted)
	{
		PressedHighlight->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSPartSlotButton::SetPart(const FNSPartData& InPartData, const UNSPartDefinition* InPartDefinition)
{
	if (!IsValid(InPartDefinition))
	{
		ClearPart();
		return;
	}

	bHasPart = true;

	if (IsValid(PartIconImage))
	{
		if (UTexture2D* LoadedIcon = InPartDefinition->Icon.Get())
		{
			PartIconImage->SetBrushFromTexture(LoadedIcon);
			PartIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			// 위젯이 화면에 처음 페인트되기 전에 브러시가 바뀌는 경우 반영이 안 되고 남을 수 있어 강제 재도장
			PartIconImage->InvalidateLayoutAndVolatility();
		}
		else if (!InPartDefinition->Icon.IsNull())
		{
			TWeakObjectPtr<UNSPartSlotButton> WeakThis(this);
			TSoftObjectPtr<UTexture2D> SoftIcon = InPartDefinition->Icon;
			IconLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
				SoftIcon.ToSoftObjectPath(),
				[WeakThis, SoftIcon]()
				{
					if (!WeakThis.IsValid())
					{
						return;
					}
					if (UTexture2D* Tex = SoftIcon.Get())
					{
						WeakThis->PartIconImage->SetBrushFromTexture(Tex);
						WeakThis->PartIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
						WeakThis->PartIconImage->InvalidateLayoutAndVolatility();
					}
					WeakThis->IconLoadHandle.Reset();
				});
		}
	}

	if (IsValid(PartNameText))
	{
		PartNameText->SetText(InPartDefinition->PartName);
		PartNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (IsValid(PartRarityText))
	{
		// 등급 텍스트가 WBP에 별도로 존재하면 등급을 분리 표시 (아웃런 상점 레이아웃)
		PartRarityText->SetText(GetRarityText(InPartData.CurrentRarity));
		PartRarityText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (IsValid(PartValueText))
	{
		// 소수점은 버리고 정수로만 표시. 반올림이면 3.8이 4로 보여 실제보다 좋아 보이므로 내림 고정
		FNumberFormattingOptions Options;
		Options.MaximumFractionalDigits = 0;
		Options.MinimumFractionalDigits = 0;
		Options.RoundingMode = ERoundingMode::ToNegativeInfinity;

		// 등급 텍스트가 따로 있으면 "스탯이름 수치"로 표시, 없으면 기존 포맷("등급 수치") 유지
		if (IsValid(PartRarityText))
		{
			// 어떤 스탯이 오르는지 스탯 표시 DT에서 이름을 조회해 수치 앞에 붙임 (예: "이동속도 10")
			FText StatName;
			const FGameplayTag StatTag = NSPartUtils::GetPartStatTag(this, InPartData);
			if (const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(this))
			{
				if (const FNSStatDisplayInfoRow* StatInfo = DataSS->FindStatDisplayInfoRow(StatTag))
				{
					StatName = StatInfo->DisplayName;
				}
			}

			// DT에 이름이 등록 안 된 스탯이면 기존처럼 수치만 표시 (데이터 누락이 UI를 깨지 않게)
			PartValueText->SetText(StatName.IsEmpty()
				? FText::AsNumber(InPartData.CurrentValue, &Options)
				: FText::Format(
					NSLOCTEXT("PartSlotButton", "PartStatValueFormat", "{0} {1}"),
					StatName,
					FText::AsNumber(InPartData.CurrentValue, &Options)
				));
		}
		else
		{
			PartValueText->SetText(FText::Format(
				NSLOCTEXT("PartSlotButton", "PartValueFormat", "{0} {1}"),
				GetRarityText(InPartData.CurrentRarity),
				FText::AsNumber(InPartData.CurrentValue, &Options)
			));
		}
		PartValueText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

}

void UNSPartSlotButton::ClearPart()
{
	if (IconLoadHandle.IsValid())
	{
		IconLoadHandle->CancelHandle();
		IconLoadHandle.Reset();
	}

	bHasPart = false;

	if (IsValid(PartIconImage))
	{
		PartIconImage->SetBrushFromTexture(nullptr);
		PartIconImage->SetVisibility(ESlateVisibility::Hidden);
	}

	if (IsValid(PartNameText))
	{
		PartNameText->SetText(FText::GetEmpty());
		PartNameText->SetVisibility(ESlateVisibility::Hidden);
	}

	if (IsValid(PartValueText))
	{
		PartValueText->SetText(FText::GetEmpty());
		PartValueText->SetVisibility(ESlateVisibility::Hidden);
	}

	if (IsValid(PartRarityText))
	{
		PartRarityText->SetText(FText::GetEmpty());
		PartRarityText->SetVisibility(ESlateVisibility::Hidden);
	}

	RefreshEmptyState();
}

bool UNSPartSlotButton::IsEmpty() const
{
	return !bHasPart;
}

void UNSPartSlotButton::SetHighlighted(bool bHighlighted)
{
	bIsHighlighted = bHighlighted;

	if (IsValid(SelectedHighlight))
	{
		SelectedHighlight->SetVisibility(bHighlighted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// 선택된 항목은 눌림 이미지를 고정으로 보여주고, 선택 해제되면 원래 상태로 되돌림
	if (IsValid(PressedHighlight))
	{
		PressedHighlight->SetVisibility(bHighlighted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UNSPartSlotButton::RefreshEmptyState()
{
	if (bHasPart)
	{
		return;
	}

}

FText UNSPartSlotButton::GetRarityText(ENSPartRarity Rarity) const
{
	switch (Rarity)
	{
	case ENSPartRarity::Common:
		return NSLOCTEXT("PartSlotButton", "CommonRarity", "Common");
	case ENSPartRarity::Rare:
		return NSLOCTEXT("PartSlotButton", "RareRarity", "Rare");
	case ENSPartRarity::Epic:
		return NSLOCTEXT("PartSlotButton", "EpicRarity", "Epic");
	case ENSPartRarity::Legendary:
		return NSLOCTEXT("PartSlotButton", "LegendaryRarity", "Legendary");
	default:
		break;
	}

	return FText::GetEmpty();
}