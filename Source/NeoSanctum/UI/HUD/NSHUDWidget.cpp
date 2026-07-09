// Copyright 2026 One Team. All rights reserved.


#include "NSHUDWidget.h"
#include "NSHPShieldWidget.h"
#include "NSGoodsWidget.h"
#include "NSCrosshairWidget.h"
#include "NSAugmentationWidget.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/UI/Part/NSPartPanelWidget.h"
#include "NeoSanctum/UI/HUD/NSAmmoWidget.h"
#include "NeoSanctum/UI/HUD/NSOutRunGoodsWidget.h"
#include "NeoSanctum/UI/HUD/NSSkillSlotWidget.h"
#include "NeoSanctum/Data/UI/NSCharacterSkillUISet.h"
#include "NeoSanctum/UI/HUD/NSCharacterStatsWidget.h"
#include "NeoSanctum/UI/HUD/NSCharacterStatsBridgeSubsystem.h"
#include "NeoSanctum/UI/Minimap/NSMinimapWidget.h"


void UNSHUDWidget::UpdateHealthAndShield(
	float CurrentHealth,
	float MaxHealth,
	float CurrentShield,
	float MaxShield
	)
{
	//HP / Shield 위젯이 없으면 갱신X
	if (!HPShieldWidget)
	{
		return;
	}
	
	//HP/Shield UI 갱신
	HPShieldWidget->SetHealth(CurrentHealth, MaxHealth);
	HPShieldWidget->SetShield(CurrentShield, MaxShield);
}

void UNSHUDWidget::UpdateRunInGoods(int32 NewGoodsAmount)
{
	//TODO(영웅): 인런 재화 변경 값
	
	//런 인 재화 갱신
	if (!GoodsWidget)
	{
		return;
	}
	GoodsWidget->SetRunInGoodsAmount(NewGoodsAmount);
}

void UNSHUDWidget::UpdateRunOutGoods(int32 NewGoodsAmount)
{
	
	//런 아웃 재화 갱신
	if (!GoodsWidget)
	{
		return;
	}
	GoodsWidget->SetRunOutGoodsAmount(NewGoodsAmount);
}

void UNSHUDWidget::ResetRunInGoods()
{
	//TODO(영웅): 런 시작 지점 연결
	
	//런 인 재화 초기화
	if (!GoodsWidget)
	{
		return;
	}
	GoodsWidget->ResetRunInGoodsAmount();
}

void UNSHUDWidget::ShowCrosshair()
{
	//조준점이 필요할때 표사
	if (!CrosshairWidget)
	{
		return;
	}
	CrosshairWidget->ShowCrosshair();
}

void UNSHUDWidget::HideCrosshair()
{
	//조준점이 필요없는 상황에 숨긴다
	if (!CrosshairWidget)
	{
		return;
	}
	CrosshairWidget->HideCrosshair();
}

void UNSHUDWidget::SetCrosshairColor(FLinearColor NewColor)
{
	//상황에따른 조준점 색상 변경
	//TODO(영웅): 오버 크리티컬에 따라 색상 변경
	
	if (!CrosshairWidget)
	{
		return;
	}
	CrosshairWidget->SetCrosshairColor(NewColor);
}

void UNSHUDWidget::OpenAugmentationPanel()
{
	if (!AugmentationWidget)
	{
		return;
	}

	AugmentationWidget->OpenPanel();
	RefreshHudDimBackground();
}

void UNSHUDWidget::CloseAugmentationPanel()
{
	if (!AugmentationWidget)
	{
		return;
	}

	AugmentationWidget->ClosePanel();
	RefreshHudDimBackground();
}

void UNSHUDWidget::OpenPartPanel()
{
	if (!IsValid(PartPanelWidget))
	{
		return;
	}
	//패널을 열때 장착상태를 최신상태로 표시
	PartPanelWidget->RefreshEquippedParts();
	PartPanelWidget->SetVisibility(ESlateVisibility::Visible);
	
	if (CharacterStatsWidget)
	{
		CharacterStatsWidget->SetVisibility(ESlateVisibility::Visible);
	}
	
	if (UNSCharacterStatsBridgeSubsystem* StatsBridge =
		GetGameInstance()->GetSubsystem<UNSCharacterStatsBridgeSubsystem>())
	{
		StatsBridge->BroadcastCharacterStats(GetOwningPlayer());
	}

	if (AugmentationWidget)
	{
		AugmentationWidget->SetOwnedAugmentListVisible(true);
		AugmentationWidget->RefreshOwnedAugmentList();
	}
	
	if (MinimapWidget)
	{
		MinimapWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	RefreshHudDimBackground();
}

void UNSHUDWidget::ClosePartPanel()
{
	if (!IsValid(PartPanelWidget))
	{
		return;
	}

	PartPanelWidget->SetVisibility(ESlateVisibility::Collapsed);

	if (AugmentationWidget && !AugmentationWidget->IsPanelOpen())
	{
		AugmentationWidget->SetOwnedAugmentListVisible(false);
	}
	
	if (CharacterStatsWidget)
	{
		CharacterStatsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	if (UNSCharacterStatsBridgeSubsystem* StatsBridge =
	GetGameInstance()->GetSubsystem<UNSCharacterStatsBridgeSubsystem>())
	{
		StatsBridge->StopBroadcastCharacterStats();
	}
	
	if (MinimapWidget)
	{
		MinimapWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	RefreshHudDimBackground();
}

void UNSHUDWidget::OpenRunBuildPanel()
{
	OpenAugmentationPanel();
	OpenPartPanel();
}
void UNSHUDWidget::CloseRunBuildPanel()
{
	CloseAugmentationPanel();
	ClosePartPanel();
}

void UNSHUDWidget::UpdateExperience(float CurrentExperience, float RequiredExperience)
{
	if (!HPShieldWidget)
	{
		return;
	}
	HPShieldWidget->SetExperience(
		CurrentExperience,
		RequiredExperience);
}

void UNSHUDWidget::SelectAugmentCardByIndex(int32 CardIndex)
{

	if (!AugmentationWidget)
	{
		return;
	}

	AugmentationWidget->SelectCardByIndex(CardIndex);
}

void UNSHUDWidget::RequestRerollAugment()
{
	if (!AugmentationWidget)
	{
		return;
	}

	AugmentationWidget->RequestRerollAugment();
}

void UNSHUDWidget::RefreshHudDimBackground()
{
	if (!HudDimBackground)
	{
		return;
	}

	const bool bIsAugmentationOpen =
		AugmentationWidget &&
		AugmentationWidget->IsPanelOpen();

	const bool bIsPartPanelOpen =
		IsValid(PartPanelWidget) &&
		PartPanelWidget->GetVisibility() != ESlateVisibility::Collapsed &&
		PartPanelWidget->GetVisibility() != ESlateVisibility::Hidden;

	HudDimBackground->SetVisibility(
		(bIsAugmentationOpen)
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
}

void UNSHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	
	if (GoodsWidget)
	{
		GoodsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (OutRunGoodsWidget)
	{
		OutRunGoodsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSHUDWidget::UpdateAmmo(int32 CurrentAmmo, int32 MaxAmmo)
{
	if (!AmmoWidget)
	{
		return;
	}

	AmmoWidget->SetAmmo(CurrentAmmo, MaxAmmo);
}

void UNSHUDWidget::SetReloading(bool bReloading)
{
	if (!AmmoWidget)
	{
		return;
	}

	AmmoWidget->SetReloading(bReloading);
}

void UNSHUDWidget::ShowInRunGoods()
{
	UE_LOG(LogTemp, Log, TEXT("[Goods UI] ShowInRunGoods"));
	if (GoodsWidget)
	{
		GoodsWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (OutRunGoodsWidget)
	{
		OutRunGoodsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSHUDWidget::ShowOutRunGoods()
{
	UE_LOG(LogTemp, Log, TEXT("[Goods UI] ShowOutRunGoods"));
	if (GoodsWidget)
	{
		GoodsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (OutRunGoodsWidget)
	{
		OutRunGoodsWidget->RefreshGoods();
		OutRunGoodsWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UNSHUDWidget::ApplyCharacterSkillUISet(FName CharacterId)
{
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	const UDataTable* CharacterSkillUISetTable =
		DataSubsystem ? DataSubsystem->GetCommonCharacterSkillUISetTable() : nullptr;

	if (!CharacterSkillUISetTable)
	{
		return;
	}

	const FNSCharacterSkillUISet* SkillUISet =
		CharacterSkillUISetTable->FindRow<FNSCharacterSkillUISet>(
			CharacterId,
			TEXT("ApplyCharacterSkillUISet"));

	if (!SkillUISet)
	{
		return;
	}

	if (SkillSlot1Widget)
	{
		SkillSlot1Widget->SetSkillUIData(
			SkillUISet->Skill1UIDataRow,
			SkillUISet->Skill1InputDisplay);
	}

	if (SkillSlot2Widget)
	{
		SkillSlot2Widget->SetSkillUIData(
			SkillUISet->Skill2UIDataRow,
			SkillUISet->Skill2InputDisplay);
	}

	if (SkillSlot3Widget)
	{
		SkillSlot3Widget->SetSkillUIData(
			SkillUISet->Skill3UIDataRow,
			SkillUISet->Skill3InputDisplay);
	}
}
