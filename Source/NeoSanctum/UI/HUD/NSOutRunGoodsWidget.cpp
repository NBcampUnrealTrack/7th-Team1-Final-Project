// Copyright 2026 One Team. All rights reserved.


#include "NSOutRunGoodsWidget.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/UI/NSGoodsUIData.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSProgressionSubsystem.h"

void UNSOutRunGoodsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyGoodsUIData();

	UNSProgressionSubsystem* ProgressionSubsystem =
		GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSProgressionSubsystem>()
		: nullptr;

	if (ProgressionSubsystem)
	{
		ProgressionSubsystem->OnCommonCurrencyChanged.RemoveAll(this);
		ProgressionSubsystem->OnCommonCurrencyChanged.AddUObject(
			this,
			&ThisClass::HandleCurrencyChanged);
	}

	RefreshGoods();
}

void UNSOutRunGoodsWidget::NativeDestruct()
{
	UNSProgressionSubsystem* ProgressionSubsystem =
		GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSProgressionSubsystem>()
		: nullptr;

	if (ProgressionSubsystem)
	{
		ProgressionSubsystem->OnCommonCurrencyChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UNSOutRunGoodsWidget::HandleCurrencyChanged(int64 CommonCurrency)
{
	SetCommonGoodsAmount(CommonCurrency);
}

void UNSOutRunGoodsWidget::ApplyGoodsUIData()
{
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem || !CommonGoodsIcon)
	{
		return;
	}

	const FGameplayTag CommonGoodsTag = FGameplayTag::RequestGameplayTag(FName(TEXT("UI.Goods.RunOut")));
	const FNSGoodsUIData* CommonGoodsData = DataSubsystem->FindCommonGoodsUIDataByTag(CommonGoodsTag);

	if (CommonGoodsData)
	{
		if (UTexture2D* LoadedIcon = CommonGoodsData->GoodsIcon.Get())
		{
			CommonGoodsIcon->SetBrushFromTexture(LoadedIcon);
		}
	}
}

void UNSOutRunGoodsWidget::RefreshGoods()
{
	const UNSProgressionSubsystem* ProgressionSubsystem =
		GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSProgressionSubsystem>()
		: nullptr;

	if (!ProgressionSubsystem)
	{
		return;
	}

	const int64 CommonCurrency =
		ProgressionSubsystem->GetCommonCurrency();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[OutRunGoods] Common=%lld"),
		CommonCurrency);

	SetCommonGoodsAmount(CommonCurrency);
}
void UNSOutRunGoodsWidget::SetCommonGoodsAmount(int64 NewAmount)
{
	if (!CommonGoodsText)
	{
		return;
	}

	CommonGoodsText->SetText(
		FText::AsNumber(
			FMath::Max<int64>(NewAmount, 0)));
}
