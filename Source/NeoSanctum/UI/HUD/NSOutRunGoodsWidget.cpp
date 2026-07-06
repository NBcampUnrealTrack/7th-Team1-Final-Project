// Copyright 2026 One Team. All rights reserved.


#include "NSOutRunGoodsWidget.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/UI/NSGoodsUIData.h"

void UNSOutRunGoodsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyGoodsUIData();
	RefreshGoods();

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		if (ANSPlayerState* NSPlayerState = PlayerController->GetPlayerState<ANSPlayerState>())
		{
			if (UNSPlayerProgressComponent* ProgressComponent = NSPlayerState->GetProgressComponent())
			{
				ProgressComponent->OnCurrencyChanged.AddUObject(this, &ThisClass::HandleCurrencyChanged);
			}
		}
	}
}

void UNSOutRunGoodsWidget::NativeDestruct()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		if (ANSPlayerState* NSPlayerState = PlayerController->GetPlayerState<ANSPlayerState>())
		{
			if (UNSPlayerProgressComponent* ProgressComponent = NSPlayerState->GetProgressComponent())
			{
				ProgressComponent->OnCurrencyChanged.RemoveAll(this);
			}
		}
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
	APlayerController* PlayerController = GetOwningPlayer();
	if (!IsValid(PlayerController))
	{
		return;
	}

	ANSPlayerState* NSPlayerState =
		PlayerController->GetPlayerState<ANSPlayerState>();
	if (!IsValid(NSPlayerState))
	{
		return;
	}

	UNSPlayerProgressComponent* ProgressComponent =
		NSPlayerState->GetProgressComponent();
	if (!IsValid(ProgressComponent))
	{
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("[OutRunGoods] Common=%lld"),	ProgressComponent->GetCommonCurrency());
	SetCommonGoodsAmount(ProgressComponent->GetCommonCurrency());
}

void UNSOutRunGoodsWidget::SetCommonGoodsAmount(int32 NewAmount)
{
	if (!CommonGoodsText)
	{
		return;
	}

	CommonGoodsText->SetText(
		FText::AsNumber(FMath::Max(NewAmount, 0)));
}
