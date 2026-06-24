// Copyright 2026 One Team. All rights reserved.


#include "NSUIManagerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "NeoSanctum/UI/HUD/NSHUDWidget.h"
#include "NeoSanctum/UI/HUD/NSAugmentationWidget.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "NeoSanctum/Data/UI/NSUIWidgetData.h"
#include "NeoSanctum/UI/Result/NSRunResultWidget.h"

UNSUIManagerSubsystem* UNSUIManagerSubsystem::Get(const UObject* WorldContext)
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContext);
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UNSUIManagerSubsystem>();
}

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

void UNSUIManagerSubsystem::UpdateRunResultCommonGoods(int32 NewAmount)
{
	RunResultCommonGoods = FMath::Max(NewAmount, 0);
}

void UNSUIManagerSubsystem::UpdateRunResultSkillGoods(int32 NewAmount)
{
	RunResultSkillGoods = FMath::Max(NewAmount, 0);
}

void UNSUIManagerSubsystem::ApplyCharacterSkillUISet(FName CharacterId)
{
	if (!IsValid(HUDWidget))
	{
		return;
	}
	//캐릭터 변경 요청을 HUD로 전달해 스킬 슬롯 UI를 갱신한다
	HUDWidget->ApplyCharacterSkillUISet(CharacterId);
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

void UNSUIManagerSubsystem::ClearTitle()
{
	if (TitleWidget)
	{
		TitleWidget->RemoveFromParent();
		TitleWidget = nullptr;
	}
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
	if (!HUDWidget)
	{
		return;
	}

	HUDWidget->SelectAugmentCardByIndex(CardIndex);
	//증강을 선택할시 패널이 자동으로 닫힘
	CloseAugmentationPanel();
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

void UNSUIManagerSubsystem::OpenPauseMenu(APlayerController* OwningPlayer)
{
	if (!OwningPlayer || bPauseMenuOpen) return;

	if (!PauseMenuWidget)
	{
		TSubclassOf<UUserWidget> WidgetClassToUse = GetWidgetClassFromTable(TEXT("PauseMenu"));
		if (!WidgetClassToUse)
		{
			WidgetClassToUse = PauseMenuWidgetClass;
		}
		
		if (!WidgetClassToUse)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PauseMenu] 위젯 클래스를 찾지 못했습니다."));
			return;
		}
		
		PauseMenuWidget = CreateWidget<UUserWidget>(OwningPlayer, WidgetClassToUse);
		if (!PauseMenuWidget)
		{
			return;
		}
		
		// HUD 위에 오도록 ZOrder 높게
		PauseMenuWidget->AddToViewport(10); 
	}

	PauseMenuWidget->SetVisibility(ESlateVisibility::Visible);
	bPauseMenuOpen = true;
	PauseMenuWidget->SetFocus();
}

void UNSUIManagerSubsystem::ClosePauseMenu()
{
	if (!bPauseMenuOpen)
	{
		return;
	}
	
	bPauseMenuOpen = false;
	
	if (PauseMenuWidget)
	{
		PauseMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSUIManagerSubsystem::ClearPauseMenu()
{
	if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
		PauseMenuWidget = nullptr;
	}
	
	bPauseMenuOpen = false;
}

void UNSUIManagerSubsystem::OpenOptionPanel(APlayerController* OwningPlayer)
{
	if (!OwningPlayer || bOptionPanelOpen)
	{
		return;
	}
	
	if (!OptionWidget)
	{
		TSubclassOf<UUserWidget> WidgetClassToUse = GetWidgetClassFromTable(TEXT("Option"));
		if (!WidgetClassToUse)
		{
			WidgetClassToUse = OptionWidgetClass;
		}
		
		if (!WidgetClassToUse)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Option] 위젯 클래스 없음")); return;
		}
		
		OptionWidget = CreateWidget<UUserWidget>(OwningPlayer, WidgetClassToUse);
		if (!OptionWidget)
		{
			return;
		}
		// PauseMenu보다 높게 나와야함
		OptionWidget->AddToViewport(20); 
	}
	
	OptionWidget->SetVisibility(ESlateVisibility::Visible);
	bOptionPanelOpen = true;
}

void UNSUIManagerSubsystem::CloseOptionPanel()
{
	if (!bOptionPanelOpen)
	{
		return;
	}
	
	bOptionPanelOpen = false;
	if (OptionWidget)
	{
		OptionWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSUIManagerSubsystem::ClearOptionPanel()
{
	if (OptionWidget)
	{
		OptionWidget->RemoveFromParent(); OptionWidget = nullptr;
	}
	
	bOptionPanelOpen = false;
}

void UNSUIManagerSubsystem::CreateLoadingScreen(APlayerController* OwningPlayer)
{
	if (!OwningPlayer)
	{
		return;
	}
	
	if (LoadingScreenWidget && !LoadingScreenWidget->IsInViewport())
	{
		LoadingScreenWidget = nullptr;
	}
	
	if (LoadingScreenWidget)
	{
		return;
	}
	
	TSubclassOf<UUserWidget> WidgetClassToUse = 
		GetWidgetClassFromTable(TEXT("LoadingScreen"));
	
	if (!WidgetClassToUse)
	{
		return;
	}
	
	LoadingScreenWidget = CreateWidget<UUserWidget>(OwningPlayer, WidgetClassToUse);
	if (LoadingScreenWidget)
	{
		LoadingScreenWidget->AddToViewport(1000);
		LoadingScreenWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSUIManagerSubsystem::ShowLoadingScreen()
{
	if (LoadingScreenWidget)
	{
		LoadingScreenWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void UNSUIManagerSubsystem::HideLoadingScreen()
{
	if (LoadingScreenWidget)
	{
		LoadingScreenWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSUIManagerSubsystem::ShowTravelLoadingScreen(APlayerController* OwningPlayer)
{
	bTravelLoadingScreenActive = true;
	CreateLoadingScreen(OwningPlayer);
	ShowLoadingScreen();
}

void UNSUIManagerSubsystem::RestoreTravelLoadingScreen(APlayerController* OwningPlayer)
{
	if (!bTravelLoadingScreenActive)
	{
		return;
	}

	CreateLoadingScreen(OwningPlayer);
	ShowLoadingScreen();
}

void UNSUIManagerSubsystem::HideTravelLoadingScreen()
{
	bTravelLoadingScreenActive = false;
	HideLoadingScreen();
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
	
	RunResultCommonGoods = 0;
	RunResultSkillGoods = 0;

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
		RunResultCommonGoods,
		RunResultSkillGoods,
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

void UNSUIManagerSubsystem::UpdateAmmo(int32 CurrentAmmo, int32 MaxAmmo)
{
	if (!HUDWidget)
	{
		return;
	}

	HUDWidget->UpdateAmmo(CurrentAmmo, MaxAmmo);
}

void UNSUIManagerSubsystem::SetReloading(bool bReloading)
{
	if (!HUDWidget)
	{
		return;
	}

	HUDWidget->SetReloading(bReloading);
}

void UNSUIManagerSubsystem::UpdateRunSkillGoods(int32 NewGoodsAmount)
{
	if (!IsValid(HUDWidget))
	{
		return;
	}

	HUDWidget->UpdateRunSkillGoods(NewGoodsAmount);
}

void UNSUIManagerSubsystem::ShowInRunGoods()
{
	UE_LOG(LogTemp, Log, TEXT("[Goods UI] UIManager ShowInRunGoods"));

	if (!IsValid(HUDWidget))
	{
		return;
	}

	HUDWidget->ShowInRunGoods();
}

void UNSUIManagerSubsystem::ShowOutRunGoods()
{
	UE_LOG(LogTemp, Log, TEXT("[Goods UI] UIManager ShowOutRunGoods"));

	if (!IsValid(HUDWidget))
	{
		return;
	}

	HUDWidget->ShowOutRunGoods();
}
void UNSUIManagerSubsystem::RefreshOutRunGoods()
{
	if (!IsValid(HUDWidget))
	{
		return;
	}

	HUDWidget->ShowOutRunGoods();
}
