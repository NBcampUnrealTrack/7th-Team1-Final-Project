// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NSGameFlowSubsystem.generated.h"

class UNSLevelCatalog;

/**
 * 아웃게임 <-> 인 런 간 트래블 로직 담당할 서브시스템
 */

UCLASS()
class NEOSANCTUM_API UNSGameFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	// 스테이지1 세팅 후 첫 레벨로 ServerTravel
	void StartNewRun(); 
	// 스테이지 클리어 이 후 다음 스테이지로 트래블하는 용도
	bool AdvanceToNextStage(); 
	
	bool ReturnToHub();
	
	// 스테이지3 클리어 후에 랜덤한 레벨 번호 뽑아주는 용도
	int32 PickRandomIndexExcludingCurrent() const;

	int32 GetCurrentStageNumber() const { return CurrentStageNumber; }
	
private:
	UNSLevelCatalog* GetCatalog() const;
	
	int32 PickNextInRunIndex() const;
	
	bool ServerTravelToWorld(const TSoftObjectPtr<UWorld>& Level, const FString& Options);
	
	int32 CurrentStageNumber = 0;
	
	int32 CurrentInRunIndex = INDEX_NONE;
};
