// Copyright 2026 One Team. All rights reserved.


#include "NSOutRunGoodsWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "CommonTextBlock.h"

void UNSOutRunGoodsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshGoods();
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
	UE_LOG(LogTemp, Log, TEXT("[OutRunGoods] Common=%lld Skill=%lld"),
		ProgressComponent->GetCommonCurrency(),
		ProgressComponent->GetJobCurrency());
	SetCommonGoodsAmount(ProgressComponent->GetCommonCurrency());
	SetSkillGoodsAmount(ProgressComponent->GetJobCurrency());
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

void UNSOutRunGoodsWidget::SetSkillGoodsAmount(int32 NewAmount)
{
	if (!SkillGoodsText)
	{
		return;
	}

	SkillGoodsText->SetText(
		FText::AsNumber(FMath::Max(NewAmount, 0)));
}