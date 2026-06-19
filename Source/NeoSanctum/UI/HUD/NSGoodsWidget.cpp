// Copyright 2026 One Team. All rights reserved.


#include "NSGoodsWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "CommonTextBlock.h"
#include "NeoSanctum/Data/UI/NSGoodsUIData.h"

void UNSGoodsWidget::SetRunInGoodsAmount(int32 NewGoodsAmount)
{
	//TODO(영웅): 런인 재화데이터 연동 필요
	
	//런 인 재화 저장
	CurrentRunInGoodsAmount = FMath::Max(NewGoodsAmount,0);
	
	//런 인 재화 텍스트 갱신
	if (RunInGoodsText)
	{
		RunInGoodsText->SetText(
			FText::AsNumber(CurrentRunInGoodsAmount));
	}
}

void UNSGoodsWidget::SetRunOutGoodsAmount(int32 NewGoodsAmount)
{
	//TODO(영웅): 런 아웃 재화 데이터 연동 필요
	
	//런 아웃 재화 저장
	CurrentRunOutGoodsAmount = FMath::Max(NewGoodsAmount,0);
	
	//런 아웃 재화 텍스트 갱신
	if (RunOutGoodsText)
	{
		RunOutGoodsText->SetText(
			FText::AsNumber(CurrentRunOutGoodsAmount));
	}
}

void UNSGoodsWidget::AddRunInGoodsAmount(int32 AddAmount)
{
	//런 인 획득 재화
	SetRunInGoodsAmount(CurrentRunInGoodsAmount+AddAmount);
}

void UNSGoodsWidget::AddRunOutGoodsAmount(int32 AddAmount)
{
	//런 아웃 획득 재화
	SetRunOutGoodsAmount(CurrentRunOutGoodsAmount+AddAmount);
}

void UNSGoodsWidget::UseRunInGoodsAmount(int32 UseAmount)
{
	//런 인 소모 재화
	SetRunInGoodsAmount(CurrentRunInGoodsAmount-UseAmount);
}

void UNSGoodsWidget::UseRunOutGoodsAmount(int32 UseAmount)
{
	//런 아웃 소모 재화
	SetRunOutGoodsAmount(CurrentRunOutGoodsAmount-UseAmount);
}

void UNSGoodsWidget::ResetRunInGoodsAmount()
{
	//런 시작시 런 내부 재화 초기화
	//TODO(영웅):런 시작시 런 내부 재화 초기화
	
	SetRunInGoodsAmount(0);
}

void UNSGoodsWidget::ApplyGoodsUIData()
{
	const FNSGoodsUIData* RunInGoodsData =
		RunInGoodsUIDataRow.GetRow<FNSGoodsUIData>(
			TEXT("ApplyGoodsUIData_RunIn"));
	
	if (RunInGoodsData && RunInGoodsIcon)
	{
		UTexture2D* LoadedIcon =
			RunInGoodsData->GoodsIcon.LoadSynchronous();
		
		if (LoadedIcon)
		{
			RunInGoodsIcon->SetBrushFromTexture(LoadedIcon);
		}
	}
	const FNSGoodsUIData* RunOutGoodsData =
		RunOutGoodsUIDataRow.GetRow<FNSGoodsUIData>(
			TEXT("ApplyGoodsUIData_RunOut"));
	
	if (RunOutGoodsData && RunOutGoodsIcon)
	{
		UTexture2D* LoadedIcon =
			RunOutGoodsData->GoodsIcon.LoadSynchronous();
		if (LoadedIcon)
		{
			RunOutGoodsIcon->SetBrushFromTexture(LoadedIcon);
		}
	}
}

void UNSGoodsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	ApplyGoodsUIData();
	
	//실제 값이 들어오기 전 기본 상태
	SetRunInGoodsAmount(0);
	SetRunOutGoodsAmount(0);
	SetRunSkillGoodsAmount(0);
}
void UNSGoodsWidget::SetRunSkillGoodsAmount(int32 NewGoodsAmount)
{
	if (!RunSkillGoodsText)
	{
		return;
	}

	RunSkillGoodsText->SetText(
		FText::AsNumber(FMath::Max(NewGoodsAmount, 0)));
}