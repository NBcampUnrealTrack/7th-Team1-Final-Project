// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSLevelCatalog.generated.h"

/**
 *  트래블에서 사용할 맵 리스트
 */
USTRUCT(BlueprintType)
struct FNSInRunLevelEntry
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	FName LevelId;
	
	UPROPERTY(EditAnywhere, meta=(AllowedClasses="/Script/Engine.World"))
	TSoftObjectPtr<UWorld> Level;
	
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
