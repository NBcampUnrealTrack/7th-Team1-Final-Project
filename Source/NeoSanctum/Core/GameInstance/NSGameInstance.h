// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "NeoSanctum/Core/Interface/NSGameInstanceInterface.h"
#include "NSGameInstance.generated.h"


UCLASS()
class NEOSANCTUM_API UNSGameInstance :
public UGameInstance,
public INSGameInstanceInterface
{
	GENERATED_BODY()
	
public:
	virtual void Init() override;
	virtual void Shutdown() override;
	
	virtual void HideLoadingScreen_Implementation() override;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> LoadingWidget;

	// 로딩 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> LoadingWidgetClass;
	
	// 로딩 UI 자동 제거에서 제외할 레벨 목록
	UPROPERTY(EditDefaultsOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TSet<FString> InRunLevelNames;

	void ShowLoadingScreen();
	
	// 하드 트래블 전용
	void OnPreLoadMap(const FString& MapURL);
	void OnPostLoadMap(UWorld* LoadedWorld);

	// 심리스 트래블 전용
	void OnSeamlessTravelStart(UWorld* CurrentWorld, const FString& LevelName);
	
	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle SeamlessTravelStartHandle;
};
