// Copyright 2026 One Team. All rights reserved.


#include "NSUIManagerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "NeoSanctum/UI/HUD/NSHUDWidget.h"
#include "NeoSanctum/UI/HUD/NSAugmentationWidget.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Data/UI/NSUIWidgetData.h"
#include "NeoSanctum/UI/Result/NSRunResultWidget.h"
#include "NeoSanctum/UI/Spectator/NSSpectatorWidget.h"

UNSUIManagerSubsystem* UNSUIManagerSubsystem::Get(const UObject* WorldContext)
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContext);
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UNSUIManagerSubsystem>();
}

void UNSUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &ThisClass::HandleCommonDataReady);

		if (DataSubsystem->IsCommonReady())
		{
			HandleCommonDataReady();
			return;
		}

		DataSubsystem->OnCommonDataReady.AddDynamic(this, &ThisClass::HandleCommonDataReady);
	}
	
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
	  this,
	  &UNSUIManagerSubsystem::HandlePostLoadMap);
}

void UNSUIManagerSubsystem::Deinitialize()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGameViewportClient* VC = GI->GetGameViewportClient())
		{
			if (LoadingScreenSlate.IsValid())
			{
				VC->RemoveViewportWidgetContent(LoadingScreenSlate.ToSharedRef());
			}
		}
	}
	LoadingScreenSlate.Reset();
	LoadingScreenWidget = nullptr;
	
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	
	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &ThisClass::HandleCommonDataReady);
	}

	WidgetClassCache.Reset();
	UIWidgetDataTable = nullptr;

	Super::Deinitialize();
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

void UNSUIManagerSubsystem::ApplyCharacterSkillUISet(FName CharacterId)
{
	if (!IsValid(HUDWidget))
	{
		return;
	}
	//캐릭터 변경 요청을 HUD로 전달해 스킬 슬롯 UI를 갱신한다
	HUDWidget->ApplyCharacterSkillUISet(CharacterId);
}

void UNSUIManagerSubsystem::CreateSpectator(APlayerController* OwningPlayer)
{
	if (!OwningPlayer)
	{
		return;
	}
	
	if (SpectatorWidget)
	{
		return;
	}
	
	TSubclassOf<UUserWidget> WidgetClassToUse =
		GetWidgetClassFromTable(TEXT("Spectator"));
	
	if (!WidgetClassToUse)
	{
		WidgetClassToUse = SpectatorWidgetClass;
	}
	
	if (!WidgetClassToUse)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Spectator UI] Spectator 위젯 클래스를 찾을 수 없습니다."));
		return;
	}
	
	SpectatorWidget = CreateWidget<UNSSpectatorWidget>(
		OwningPlayer,
		WidgetClassToUse);
	
	if (!SpectatorWidget)
	{
		return;
	}
	
	SpectatorWidget->AddToViewport(20);
	SpectatorWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void UNSUIManagerSubsystem::ShowSpectator(const FString& SpectatingPlayerName)
{
	HideHUD();
	
	if (!SpectatorWidget)
	{
		return;
	}
	
	SpectatorWidget->SetSpectatingPlayerName(SpectatingPlayerName);
	SpectatorWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UNSUIManagerSubsystem::HideSpectator()
{
	if (!SpectatorWidget)
	{
		return;
	}
	
	SpectatorWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void UNSUIManagerSubsystem::ClearSpectator()
{
	if (!SpectatorWidget)
	{
		return;
	}
	
	SpectatorWidget->RemoveFromParent();
	SpectatorWidget = nullptr;
}

void UNSUIManagerSubsystem::UpdateRunEndResultFromGameState(const ANSRunGameState* RunGameState)
{
	UNSRunResultWidget* RunResultWidget =
		Cast<UNSRunResultWidget>(RunEndWidget);
	if (!RunResultWidget || !RunGameState)
	{
		return;
	}

	const FNSRunResultData& ResultData = RunGameState->RunResultData;

	RunResultWidget->SetRunResult(
		RunGameState->bIsClear,
		ResultData.EarnedGoods,
		ResultData.CommonGoods,
		ResultData.RunTimeSeconds,
		ResultData.KillCount);
}

void UNSUIManagerSubsystem::MarkTravelPawnReady()
{
	bTravelPawnReady = true;
	UE_LOG(LogTemp, Warning, TEXT("[TravelLoading] PawnReady / World=%s"), *GetNameSafe(GetWorld()));
	TryFinishTravelLoading();
}

void UNSUIManagerSubsystem::MarkTravelLevelReady()
{
	bTravelLevelReady = true;
	UE_LOG(LogTemp, Warning, TEXT("[TravelLoading] LevelReady / World=%s"), *GetNameSafe(GetWorld()));
	TryFinishTravelLoading();
}

void UNSUIManagerSubsystem::MarkTravelViewReady()
{
	bTravelViewReady = true;
	UE_LOG(LogTemp, Warning, TEXT("[TravelLoading] ViewReady / World=%s"), *GetNameSafe(GetWorld()));
	TryFinishTravelLoading();
}

void UNSUIManagerSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	UE_LOG(LogTemp, Warning, TEXT("[TravelLoading] PostLoadMap / World=%s / Active=%d"),
	*GetNameSafe(LoadedWorld), bTravelLoadingScreenActive ? 1 : 0);

	// 로딩 트래블이 진행 중일 때만 재확인
	if (!bTravelLoadingScreenActive || !LoadedWorld)
	{
		return;
	}

	APlayerController* PC = LoadedWorld->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	// !IsInViewport면 재생성 후 표시 (드롭됐던 위젯 복원)
	CreateLoadingScreen(PC);
	ShowLoadingScreen();
}

void UNSUIManagerSubsystem::HandleCommonDataReady()
{
	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &ThisClass::HandleCommonDataReady);
		UIWidgetDataTable = DataSubsystem->GetCommonUIWidgetDataTable();
	}

	RebuildWidgetClassCache();
	RetryPendingTitleCreation();
}

void UNSUIManagerSubsystem::RetryPendingTitleCreation()
{
	if (!bPendingTitleCreation)
	{
		return;
	}

	bPendingTitleCreation = false;

	APlayerController* OwningPlayer = PendingTitleOwningPlayer.Get();
	PendingTitleOwningPlayer.Reset();

	if (!OwningPlayer || TitleWidget)
	{
		return;
	}

	CreateTitle(OwningPlayer);
	ShowTitle();
}

void UNSUIManagerSubsystem::RebuildWidgetClassCache()
{
	WidgetClassCache.Reset();

	if (!UIWidgetDataTable || UIWidgetDataTable->GetRowStruct() != FNSUIWidgetData::StaticStruct())
	{
		return;
	}

	const FString ContextString = TEXT("RebuildWidgetClassCache");
	for (const FName& RowName : UIWidgetDataTable->GetRowNames())
	{
		const FNSUIWidgetData* WidgetData =
			UIWidgetDataTable->FindRow<FNSUIWidgetData>(RowName, ContextString, false);

		if (!WidgetData || WidgetData->WidgetClass.IsNull())
		{
			continue;
		}

		// NSDataSubsystem에서 CommonDataReady 전에 선로드하므로 여기서는 동기 로드하지 않음.
		UClass* LoadedWidgetClass = WidgetData->WidgetClass.Get();
		if (LoadedWidgetClass && LoadedWidgetClass->IsChildOf(UUserWidget::StaticClass()))
		{
			WidgetClassCache.Add(RowName, LoadedWidgetClass);
		}
	}
}

TSubclassOf<UUserWidget> UNSUIManagerSubsystem::GetWidgetClassFromTable(FName RowName) const
{
	if (RowName.IsNone())
	{
		return nullptr;
	}

	if (const TSubclassOf<UUserWidget>* CacheWidgetClass = WidgetClassCache.Find(RowName))
	{
		return *CacheWidgetClass;
	}

	return nullptr;
}

void UNSUIManagerSubsystem::TryFinishTravelLoading()
{
	if (!bTravelLoadingScreenActive)
	{
		return;
	}
	
	if (!bTravelPawnReady || !bTravelLevelReady || !bTravelViewReady)
	{
		return;
	}
	
	HideTravelLoadingScreen(); 
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

void UNSUIManagerSubsystem::UpdateExperience(float CurrentExperience, float RequiredExperience)
{
	if (!HUDWidget)
	{
		return;
	}
	HUDWidget->UpdateExperience(
		CurrentExperience,
		RequiredExperience);
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
	// @원종: pending 초기화.
	bPendingTitleCreation = false;
	PendingTitleOwningPlayer.Reset();

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
		// @원종: 위젯 클래스가 없으면 pending처리로 넘김
		if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
		{
			if (!DataSubsystem->IsCommonReady())
			{
				// DT_UIWidget 로딩이 끝나기 전에 Title 생성이 요청된 경우, CommonDataReady 이후 다시 CreateTitle을 호출.
				PendingTitleOwningPlayer = OwningPlayer;
				bPendingTitleCreation = true;

				DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &ThisClass::HandleCommonDataReady);
				DataSubsystem->OnCommonDataReady.AddDynamic(this, &ThisClass::HandleCommonDataReady);
				return;
			}
		}

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
	//런 결과창이 표시될때 기존 인게임 HUD가 겹처보이지 않게한다
	HideHUD();
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
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	
	// 이미 유효하게 살아있으면 재생성하지 않음
	if (LoadingScreenWidget && LoadingScreenSlate.IsValid())
	{
		return;
	}

	// 잔재가 남은 경우 정리 후 재생성
	LoadingScreenWidget = nullptr;
	LoadingScreenSlate.Reset();

	TSubclassOf<UUserWidget> WidgetClassToUse =
		GetWidgetClassFromTable(TEXT("LoadingScreen"));
	if (!WidgetClassToUse)
	{
		return;
	}

	// 오너를 GameInstance로 설정
	LoadingScreenWidget = CreateWidget<UUserWidget>(GI, WidgetClassToUse);
	if (!LoadingScreenWidget)
	{
		return;
	}

	UGameViewportClient* VC = GI->GetGameViewportClient();
	if (!VC)
	{
		// 뷰포트가 아직 없으면 위젯만 생성
		return;
	}

	// Slate 핸들을 한 번만 생성해 캐싱
	LoadingScreenSlate = LoadingScreenWidget->TakeWidget();

	VC->AddViewportWidgetContent(LoadingScreenSlate.ToSharedRef(), 1000 /*ZOrder*/);

	// 숨김 상태로 부착
	LoadingScreenWidget->SetVisibility(ESlateVisibility::Collapsed);
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
	UE_LOG(LogTemp, Warning, TEXT("[TravelLoading] Show / World=%s"), *GetNameSafe(GetWorld()));
	
	bTravelLoadingScreenActive = true;
	bTravelPawnReady = false;
	bTravelLevelReady = false;
	bTravelViewReady = false;
	CreateLoadingScreen(OwningPlayer);
	ShowLoadingScreen();
}

void UNSUIManagerSubsystem::RestoreTravelLoadingScreen(APlayerController* OwningPlayer)
{
	UE_LOG(LogTemp, Warning, TEXT("[TravelLoading] Restore / World=%s / Active=%d"),
	*GetNameSafe(GetWorld()),
	bTravelLoadingScreenActive ? 1 : 0);
	
	if (!bTravelLoadingScreenActive)
	{
		return;
	}

	CreateLoadingScreen(OwningPlayer);
	ShowLoadingScreen();
}

void UNSUIManagerSubsystem::HideTravelLoadingScreen()
{
	UE_LOG(LogTemp, Warning,
		TEXT("[TravelLoading] Hide / World=%s / PawnReady=%d / LevelReady=%d / ViewReady=%d"),
		*GetNameSafe(GetWorld()),
		bTravelPawnReady ? 1 : 0,
		bTravelLevelReady ? 1 : 0,
		bTravelViewReady ? 1 : 0);
	
	bTravelLoadingScreenActive = false;
	// 부착할 때 쓴 Slate 핸들로 제거
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGameViewportClient* VC = GI->GetGameViewportClient())
		{
			if (LoadingScreenSlate.IsValid())
			{
				VC->RemoveViewportWidgetContent(LoadingScreenSlate.ToSharedRef());
			}
		}
	}

	LoadingScreenSlate.Reset();
	// 다음 트래블에 새로 생성
	LoadingScreenWidget = nullptr; 
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
