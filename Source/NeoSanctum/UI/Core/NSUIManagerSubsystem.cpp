// Copyright 2026 One Team. All rights reserved.


#include "NSUIManagerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "NeoSanctum/UI/HUD/NSHUDWidget.h"
#include "NeoSanctum/UI/HUD/NSAugmentationWidget.h"


void UNSUIManagerSubsystem::CreateHUD(APlayerController* OwningPlayer)
{
	// 테스트 로직으로 인해서 !HUDWidgetClass 검사 로직 제거
	if (!OwningPlayer)
	{
		return;
	}
	//TODO(영웅) : UI 설정 DataAsset 기반 HUD 클래스 관리 추가
	
	//이미 HUD가 있으면 중복 생성 X
	if (HUDWidget)
	{
		return;
	}
	
	FString HUDPath = TEXT("/Game/NeoSanctum/UI/HUD/WBP_HUD.WBP_HUD_C");
	UClass* LoadedHUDClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, *HUDPath);

	if (LoadedHUDClass)
	{
		HUDWidget = CreateWidget<UNSHUDWidget>(OwningPlayer, LoadedHUDClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
		}
	}
	
	/*
	HUDWidget = CreateWidget<UNSHUDWidget>(OwningPlayer, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToViewport();
	}
	*/
}

void UNSUIManagerSubsystem::ShowHUD()
{
	if (!HUDWidget)
	{
		return;
	}
	//HUD를 다시 보여줄때 기존 상태 유지
	HUDWidget->SetVisibility(ESlateVisibility::Visible);
}

void UNSUIManagerSubsystem::HideHUD()
{
	if (!HUDWidget)
	{
		return;
	}
	//HUD 숨김 처리 후 재사용
	HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void UNSUIManagerSubsystem::UpdateRunInGoods(int32 NewGoodsAmount)
{
	//TODO(영웅) 실제 런 내부 재화 데이터 연동
	if (!HUDWidget)
	{
		return;
	}
	HUDWidget->UpdateRunInGoods(NewGoodsAmount);
}

void UNSUIManagerSubsystem::UpdateRunOutGoods(int32 NewGoodsAmount)
{
	//TODO(영웅): 영구 재화 데이터 연동
	if (!HUDWidget)
	{
		return;
	}
	HUDWidget->UpdateRunOutGoods(NewGoodsAmount);
}

void UNSUIManagerSubsystem::ResetRunInGoods()
{
	//TODO(영웅): 런 시작시점에 호출
	if (!HUDWidget)
	{
		return;
	}
	HUDWidget->ResetRunInGoods();
}

void UNSUIManagerSubsystem::ShowCrosshair()
{
	//조준점 표시 요청을 HUD로 전달한다
	if (!HUDWidget)
	{
		return;
	}
	HUDWidget->ShowCrosshair();
}

void UNSUIManagerSubsystem::HideCrosshair()
{
	//조준점 숨김 요청을 HUD로 전달
	if (!HUDWidget)
	{
		return;
	}
	HUDWidget->HideCrosshair();
}

void UNSUIManagerSubsystem::SetCrosshairColor(FLinearColor NewColor)
{
	//TODO(영웅): 오버크리티컬에 따라 색상 변경
	if (!HUDWidget)
	{
		return;
	}
	HUDWidget->SetCrosshairColor(NewColor);
}

void UNSUIManagerSubsystem::UpdateHealthAndShield(
	float CurrentHealth,
	float MaxHealth,
	float CurrentShield,
	float MaxShield)
{
	//실제 체력/ 실드 값을 HUD로 전달
	if (!HUDWidget)
	{
		return;
	}
	HUDWidget->UpdateHealthAndShield(
		CurrentHealth,
		MaxHealth,
		CurrentShield,
		MaxShield);
}

void UNSUIManagerSubsystem::ClearHUD()
{
	if (HUDWidget)
	{
		HUDWidget->RemoveFromParent();
		HUDWidget=nullptr;
	}
}

void UNSUIManagerSubsystem::SelectAugmentCardByIndex(int32 CardIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("[증강] UIManager SelectAugmentCardByIndex 호출됨: %d"), CardIndex);

	if (!HUDWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[증강] 실패: HUDWidget이 nullptr 입니다"));
		return;
	}

	HUDWidget->SelectAugmentCardByIndex(CardIndex);
}

void UNSUIManagerSubsystem::ShowAugmentation()
{
	//증강 UI를 HUD에 요청
	if (!HUDWidget)
	{
		return;
	}
	//증강 UI표시는 HUD에서 관리
	HUDWidget->ShowAugmentation();
}

void UNSUIManagerSubsystem::HideAugmentation()
{
	//증강 UI 숨김을 HUD에 요청
	if (!HUDWidget)
	{
		return;
	}
	//증강 UI숨김 HUD에서 관리
	HUDWidget->HideAugmentation();	
}

UNSHUDWidget* UNSUIManagerSubsystem::GetHUDWidget() const
{
	return HUDWidget;
}

void UNSUIManagerSubsystem::CreateTitle(APlayerController* OwningPlayer)
{
	// 테스트 로직으로 인해서 !TitleWidgetClass 검사 로직 삭제
	if (!OwningPlayer)
	{
		return;
	}
	
	if (TitleWidget)
	{
		return; 
	}
	
	FString TitlePath = TEXT("/Game/NeoSanctum/UI/Menu/WBP_Title.WBP_Title_C");
	UClass* LoadedTitleClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, *TitlePath);

	if (LoadedTitleClass)
	{
		TitleWidget = CreateWidget<UUserWidget>(OwningPlayer, LoadedTitleClass);
		if (TitleWidget)
		{
			TitleWidget->AddToViewport();
		}
	}
	
	/*
	TitleWidget = CreateWidget<UUserWidget>(OwningPlayer, TitleWidgetClass);
	if (TitleWidget)
	{
		TitleWidget->AddToViewport();
	}
	*/
}

void UNSUIManagerSubsystem::ShowTitle()
{
	if (TitleWidget)
	{
		TitleWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void UNSUIManagerSubsystem::HideTitle()
{
	if (TitleWidget)
	{
		TitleWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSUIManagerSubsystem::CreateRunEnd(APlayerController* OwningPlayer)
{
	if (!OwningPlayer)
	{
		return;
	}
	
	if (RunEndWidget)
	{
		return; 
	}
	
	FString RunEndPath = TEXT("/Game/NeoSanctum/UI/WBP_TestRunEnd.WBP_TestRunEnd_C");
	UClass* LoadedRunEndClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, *RunEndPath);

	if (LoadedRunEndClass)
	{
		RunEndWidget = CreateWidget<UUserWidget>(OwningPlayer, LoadedRunEndClass);
		if (RunEndWidget)
		{
			RunEndWidget->AddToViewport();
		}
	}
}

void UNSUIManagerSubsystem::ShowRunEnd()
{
	if (RunEndWidget)
	{
		RunEndWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void UNSUIManagerSubsystem::HideRunEnd()
{
	if (RunEndWidget)
	{
		RunEndWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}
