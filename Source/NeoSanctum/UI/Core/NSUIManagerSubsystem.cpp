// Copyright 2026 One Team. All rights reserved.


#include "NSUIManagerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "NeoSanctum/UI/HUD/NSHUDWidget.h"
#include "NeoSanctum/UI/HUD/NSAugmentationWidget.h"
#include "Engine/DataTable.h"
#include "UObject/ConstructorHelpers.h"
#include "NeoSanctum/Data/UI/NSUIWidgetData.h"
#include "NeoSanctum/UI/Result/NSRunResultWidget.h"

float UNSUIManagerSubsystem::GetRunResultTimeSeconds() const
{
	if (bRunResultTimeCached)
	{
		return CachedRunResultTimeSeconds;
	}

	const UWorld* World = GetWorld();
	if (!World || RunStartWorldTimeSeconds < 0.0f)
	{
		return 0.0f;
	}

	return FMath::Max(
		World->GetTimeSeconds() - RunStartWorldTimeSeconds,
		0.0f);
}

void UNSUIManagerSubsystem::CacheRunResultTime()
{
	if (bRunResultTimeCached)
	{
		return;
	}

	CachedRunResultTimeSeconds = GetRunResultTimeSeconds();
	bRunResultTimeCached = true;
}

UNSUIManagerSubsystem::UNSUIManagerSubsystem()
{
	static ConstructorHelpers::FObjectFinder<UDataTable>
	UIWidgetTableFinder(
		TEXT("/Game/NeoSanctum/Data/UI/DT_UIWidget.DT_UIWidget"));
	if (UIWidgetTableFinder.Succeeded())
	{
		UIWidgetDataTable = UIWidgetTableFinder.Object;
	}
}
TSubclassOf<UUserWidget>
UNSUIManagerSubsystem::GetWidgetClassFromTable(
	FName RowName) const
{
	if (!UIWidgetDataTable)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[UI] DT_UIWidget을 찾지 못했습니다."));
		return nullptr;
	}
	const FNSUIWidgetData* WidgetData =
		UIWidgetDataTable->FindRow<FNSUIWidgetData>(
			RowName,
			TEXT("GetWidgetClassFromTable"));
	if (!WidgetData)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[UI] Row를 찾지 못했습니다: %s"),
			*RowName.ToString());
		return nullptr;
	}
	return WidgetData->WidgetClass.LoadSynchronous();
}

void UNSUIManagerSubsystem::CreateHUD(APlayerController* OwningPlayer)
{
	if (!OwningPlayer)
	{
		return;
	}

	if (HUDWidget)
	{
		return;
	}

	TSubclassOf<UNSHUDWidget> WidgetClassToUse = nullptr;

	//데이터테이블에 없을경우 기존 HUD위젯으로
	TSubclassOf<UUserWidget> LoadedWidgetClass =
		GetWidgetClassFromTable(TEXT("HUD"));

	//데이터테이블에서 타입을 검증
	if (LoadedWidgetClass && LoadedWidgetClass->IsChildOf(UNSHUDWidget::StaticClass()))
	{
		WidgetClassToUse = *LoadedWidgetClass;
	}

	//데이터테이블에 없을시 에디터에서 지정한 위젯을 사용
	if (!WidgetClassToUse)
	{
		WidgetClassToUse = HUDWidgetClass;
	}

	//데이터테이블과 fallback에 없으면 종료
	if (!WidgetClassToUse)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UI Data] HUD 위젯 클래스를 찾지 못했습니다."));
		return;
	}

	HUDWidget = CreateWidget<UNSHUDWidget>(
		OwningPlayer,
		WidgetClassToUse);

	if (HUDWidget)
	{
		HUDWidget->AddToViewport();
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
	RunResultGoods = FMath::Max(NewGoodsAmount, 0);

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
		HUDWidget = nullptr;
	}
	bAugmentationPanelOpen = false;
}

void UNSUIManagerSubsystem::SelectAugmentCardByIndex(int32 CardIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("[증강] UIManager SelectAugmentCardByIndex 호출됨: %d"), CardIndex);

	if (!HUDWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[증강] 실패: HUDWidget이 nullptr 입니다"));
		return;
	}

	HUDWidget->SelectAugmentCardByIndex(CardIndex);
}

void UNSUIManagerSubsystem::RequestRerollAugment()
{
	if (!HUDWidget)
	{
		return;
	}

	HUDWidget->RequestRerollAugment();
}

void UNSUIManagerSubsystem::OpenAugmentationPanel()
{
	if (bAugmentationPanelOpen)
	{
		return;
	}
	if (!HUDWidget)
	{
		return;
	}
	bAugmentationPanelOpen = true;
	HUDWidget->OpenAugmentationPanel();
}

void UNSUIManagerSubsystem::CloseAugmentationPanel()
{
	if (!bAugmentationPanelOpen)
	{
		return;
	}
	bAugmentationPanelOpen = false;
	if (!HUDWidget)
	{
		return;
	}
	HUDWidget->CloseAugmentationPanel();
}

UNSHUDWidget* UNSUIManagerSubsystem::GetHUDWidget() const
{
	return HUDWidget;
}

void UNSUIManagerSubsystem::CreateTitle(APlayerController* OwningPlayer)
{
		if (!OwningPlayer)
		{
			return;
		}

		if (TitleWidget)
		{
			return;
		}
	
		//데이터테이블에 없을경우 기존 Title위젯으로
		TSubclassOf<UUserWidget> WidgetClassToUse =
			GetWidgetClassFromTable(TEXT("Title"));

		//데이터테이블에 없을경우 기존에 에디터에서 지정한 위젯 불러옴 
		if (!WidgetClassToUse)
		{
			WidgetClassToUse = TitleWidgetClass;
		}

		//데이터테이블과 fallback 이 모두 없으면 종료
		if (!WidgetClassToUse)
		{
			UE_LOG(LogTemp, Warning, TEXT("[UI Data] Title 위젯 클래스를 찾지 못했습니다."));
			return;
		}

		TitleWidget = CreateWidget<UUserWidget>(
			OwningPlayer,
			WidgetClassToUse);

		if (TitleWidget)
		{
			TitleWidget->AddToViewport();
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

	TSubclassOf<UUserWidget> WidgetClassToUse =
		GetWidgetClassFromTable(TEXT("RunEnd"));

	if (!WidgetClassToUse)
	{
		WidgetClassToUse = RunEndWidgetClass;
	}

	if (!WidgetClassToUse)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunEnd UI] RunEnd 위젯 클래스를 찾을 수 없습니다."));
		return;
	}

	RunEndWidget = CreateWidget<UUserWidget>(
		OwningPlayer,
		WidgetClassToUse);

	if (!RunEndWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunEnd UI] RunEnd 위젯 생성에 실패했습니다."));
		return;
	}

	RunEndWidget->AddToViewport();

	RunEndWidget->SetVisibility(ESlateVisibility::Collapsed);

}

void UNSUIManagerSubsystem::ShowRunEnd()
{

	if (!RunEndWidget)
	{
		return;
	}

	RunEndWidget->SetVisibility(ESlateVisibility::Visible);

}

void UNSUIManagerSubsystem::HideRunEnd()
{
	if (RunEndWidget)
	{
		RunEndWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSUIManagerSubsystem::ClearRunEnd()
{
	if (RunEndWidget)
	{
		RunEndWidget->RemoveFromParent();
		RunEndWidget = nullptr;
	}
}

void UNSUIManagerSubsystem::OpenPartPanel()
{
	if (!IsValid(HUDWidget))
	{
		return;
	}
	
	HUDWidget->OpenPartPanel();
	bPartPanelOpen = true;
}

void UNSUIManagerSubsystem::ClosePartPanel()
{
	if (!IsValid(HUDWidget))
	{
		return;
	}
	
	HUDWidget->ClosePartPanel();
	bPartPanelOpen = false;
}

void UNSUIManagerSubsystem::ToggleRunBuildPanel()
{
	if (bRunBuildPanelOpen)
	{
		CloseRunBuildPanel();
		return;
	}
	OpenRunBuildPanel();
}

void UNSUIManagerSubsystem::OpenRunBuildPanel()
{
	if (!IsValid(HUDWidget))
	{
		return;
	}
	HUDWidget->OpenRunBuildPanel();
	
	bRunBuildPanelOpen = true;
	bAugmentationPanelOpen = true;
	bPartPanelOpen = true;
}

void UNSUIManagerSubsystem::CloseRunBuildPanel()
{
	if (!IsValid(HUDWidget))
	{
		return;
	}
	HUDWidget->CloseRunBuildPanel();
	
	bRunBuildPanelOpen = false;
	bAugmentationPanelOpen = false;
	bPartPanelOpen = false;
}

void UNSUIManagerSubsystem::ResetRunResultStats()
{
	RunResultGoods = 0;
	RunResultKillCount = 0;

	CachedRunResultTimeSeconds = 0.0f;
	bRunResultTimeCached = false;

	UWorld* World = GetWorld();
	RunStartWorldTimeSeconds = World ? World->GetTimeSeconds() : -1.0f;
}

void UNSUIManagerSubsystem::AddRunResultKillCount(int32 Amount)
{
	RunResultKillCount += FMath::Max(Amount, 0);
}

void UNSUIManagerSubsystem::UpdateRunEndResult(bool bCleared)
{
	UNSRunResultWidget* RunResultWidget =
		Cast<UNSRunResultWidget>(RunEndWidget);
	if (!RunResultWidget)
	{
		return;
	}

	RunResultWidget->SetRunResult(
		bCleared,
		RunResultGoods,
		GetRunResultTimeSeconds(),
		RunResultKillCount);
}

void UNSUIManagerSubsystem::UpdateRunEndVotes(int32 NextVotes, int32 HubVotes)
{
	UNSRunResultWidget* RunResultWidget =
	Cast<UNSRunResultWidget>(RunEndWidget);
	if (!RunResultWidget)
	{
		return;
	}

	RunResultWidget->SetVoteResult(NextVotes, HubVotes);
}

