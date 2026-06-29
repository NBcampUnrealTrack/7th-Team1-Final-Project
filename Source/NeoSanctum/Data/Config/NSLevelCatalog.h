// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSLevelCatalog.generated.h"

class UNSRunConfig;
class UNSLevelConfig;

/**
 * 인런 스테이지 목록의 단일 엔트리.
 * LevelId는 식별용이고, LevelConfig는 해당 스테이지 진입에 필요한 맵/데이터 설정을 가리킴.
 */
USTRUCT(BlueprintType)
struct FNSInRunLevelEntry
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	FName LevelId;
	
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UNSLevelConfig> LevelConfig;
};

/**
 * 타이틀, 거점, 인런 맵 목록을 보관하는 레벨 카탈로그 Data Asset.
 *
 * GameFlowSubsystem과 SessionSubsystem이 목적지 맵을 찾을 때 참조.
 */
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSLevelCatalog : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category="Levels")
	TSoftObjectPtr<UWorld> TitleLevel;
	
	UPROPERTY(EditAnywhere, Category="Levels")
	TSoftObjectPtr<UWorld> HubLevel;
	
	// 인런 시작 시 한 번 로드하고 런 종료 전까지 유지할 공통 런 데이터.
	UPROPERTY(EditAnywhere, Category = "Run")
	TSoftObjectPtr<UNSRunConfig> RunConfig;
	
	// 인런에서 순회할 스테이지 목록. 각 항목은 스테이지 전용 LevelConfig를 가리킴.
	UPROPERTY(EditAnywhere, Category="Levels") 
	TArray<FNSInRunLevelEntry> InRunLevels;
};
