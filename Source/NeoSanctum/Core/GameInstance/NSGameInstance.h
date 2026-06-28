// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "NeoSanctum/Core/Interface/NSGameInstanceInterface.h"
#include "NSGameInstance.generated.h"

class UNSSoundData;
class UNSLevelCatalog;
class UNSDifficultyConfig;
class UDataTable;

UCLASS()
class NEOSANCTUM_API UNSGameInstance :
public UGameInstance,
public INSGameInstanceInterface
{
	GENERATED_BODY()
	
public:
	static UNSGameInstance* Get(const UObject* WorldContext);
	
	virtual void Init() override;
	virtual void Shutdown() override;
	
	virtual void HideLoadingScreen_Implementation() override;
	
	// SoundDataTable 
	// TODO : 추후에 비동기로딩 흐름으로 옮기면 리팩토링 필요함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DataSet")
	TObjectPtr<UNSSoundData> SoundData;

	// VFXDataTable 
	// TODO : 추후에 비동기로딩 흐름으로 옮기면 리팩토링 필요함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DataSet")
	TObjectPtr<UDataTable> VFXDataTable;

	// HitReactionDataTable
	// TODO : 추후 비동기 로딩 흐름으로 대체되면 로드된 DataSet에서 조회
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DataSet")
	TObjectPtr<UDataTable> HitReactionDataTable;
	
	virtual UNSLevelCatalog* GetLevelCatalog() const override { return LevelCatalog; }
	virtual UNSDifficultyConfig* GetDifficultyConfig() const override { return DifficultyConfig; }

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> LoadingWidget;
	
	// 트래블할 레벨 저장용
	UPROPERTY(EditDefaultsOnly, Category="GameFlow")
	TObjectPtr<UNSLevelCatalog> LevelCatalog;
	
	// 레벨 스케일링용
	UPROPERTY(EditDefaultsOnly, Category="GameFlow") 
	TObjectPtr<UNSDifficultyConfig> DifficultyConfig; 

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
