// Copyright 2026 One Team. All rights reserved.


#include "NSGameInstance.h"
#include "Blueprint/UserWidget.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSessionSubsystem.h"

void UNSGameInstance::Init()
{
	Super::Init();
	
	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(
		this,
		&UNSGameInstance::OnPreLoadMap
		);

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&UNSGameInstance::OnPostLoadMap
	);
	
	SeamlessTravelStartHandle = FWorldDelegates::OnSeamlessTravelStart.AddUObject(
		this,
		&UNSGameInstance::OnSeamlessTravelStart);
	
}

void UNSGameInstance::Shutdown()
{
	FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	FWorldDelegates::OnSeamlessTravelStart.Remove(SeamlessTravelStartHandle);
	
	// 게임 종료 시 열려있는 세션 정리
	UNSSessionSubsystem* NSSessionSubsystem = GetSubsystem<UNSSessionSubsystem>();
	if (NSSessionSubsystem)
	{
		NSSessionSubsystem->DestroySession();
	}
	
	Super::Shutdown();
}

void UNSGameInstance::ShowLoadingScreen()
{
	if (!LoadingWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadingWidgetClass가 없습니다."));
		return;
	}
	
	if (LoadingWidget && !LoadingWidget->IsInViewport())
	{
		UE_LOG(LogTemp, Warning, TEXT("위젯 재생성"));
		LoadingWidget = nullptr;
	}
	
	// 중복 생성 방지
	if (LoadingWidget)
	{
		return;
	}

	LoadingWidget = CreateWidget<UUserWidget>(this, LoadingWidgetClass);
	if (LoadingWidget)
	{
		LoadingWidget->AddToViewport(100);
	}
}

void UNSGameInstance::HideLoadingScreen_Implementation()
{
	if (LoadingWidget)
	{
		LoadingWidget->RemoveFromParent();
		LoadingWidget = nullptr;
	}
}

void UNSGameInstance::OnPreLoadMap(const FString& MapURL)
{
	ShowLoadingScreen();
}

void UNSGameInstance::OnPostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld)
	{
		return;
	}
	
	// 로딩된 월드 이름 저장
	FString MapName = LoadedWorld->GetName();
	UE_LOG(LogTemp, Warning, TEXT("LoadMap 호출 맵 이름: %s"), *MapName);

	// 제외해둔 레벨에 있는 맵이면 return; 해서 UI 유지
	if (InRunLevelNames.Contains(MapName))
	{
		UE_LOG(LogTemp, Log, TEXT("(%s) 인 런 레벨이므로 UI 제거 X"), *MapName);
		UE_LOG(LogTemp, Log, TEXT("(%s) 인런 레벨 UI 복구"), *MapName);
		
		ShowLoadingScreen();
		return;
	}
	
	HideLoadingScreen_Implementation();
}

void UNSGameInstance::OnSeamlessTravelStart(UWorld* CurrentWorld, const FString& LevelName)
{
	UE_LOG(LogTemp, Log, TEXT("SeamlessTravel 시작: %s"), *LevelName);
	ShowLoadingScreen();
}
