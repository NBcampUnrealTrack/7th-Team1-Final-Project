// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSLevelCatalog.generated.h"

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
	
	UPROPERTY(EditAnywhere, Category="Levels") 
	TArray<FNSInRunLevelEntry> InRunLevels;
};
