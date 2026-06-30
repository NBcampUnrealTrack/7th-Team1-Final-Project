// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NeoSanctum/Core/GameFlow/NSDifficultyType.h"
#include "NSGameFlowSubsystem.generated.h"

class UNSLevelCatalog;
class UNSLevelConfig;
class UNSRunConfig;

/**
 * 아웃게임 <-> 인 런 간 트래블 로직 담당할 서브시스템
 */

UCLASS()
class NEOSANCTUM_API UNSGameFlowSubsystem :
public UGameInstanceSubsystem,
public FTickableGameObject
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
	
	// 스테이지 입장 시
	void ResumeDifficultyTimer();   
	// 보스룸 진입 시
	void PauseDifficultyTimer();  
	// 런 시작/거점 귀환 시
	void StopAndResetDifficultyTimer();   
	float GetRunElapsedSeconds() const { return RunElapsedSeconds; }
	FNSDifficultyScale GetCurrentMonsterScale(int32 PlayerCount) const;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return bDifficultyTimerRunning; }
	
	const TSoftObjectPtr<UNSRunConfig>& GetSelectedRunConfig() const { return SelectedRunConfig; }
	const TSoftObjectPtr<UNSLevelConfig>& GetSelectedRunLevelConfig() const { return SelectedRunLevelConfig; }
	
private:
	UNSLevelCatalog* GetCatalog() const;
	
	int32 PickNextInRunIndex() const;
	
	bool ServerTravelToWorld(const TSoftObjectPtr<UWorld>& Level, const FString& Options);
	
	bool RequestEnterRun(
		const TSoftObjectPtr<UNSRunConfig>& RunConfig, const TSoftObjectPtr<UNSLevelConfig>& LevelConfig);
	
	UFUNCTION()
	void HandleRunGameDataReady();
	
	// ServerTravel 이후 RunGameMode가 RunGameState에 복제할 인런 데이터 구성.
	TSoftObjectPtr<UNSRunConfig>	SelectedRunConfig;
	TSoftObjectPtr<UNSLevelConfig>	SelectedRunLevelConfig;
	
	int32 CurrentStageNumber = 0;
	
	int32 CurrentInRunIndex = INDEX_NONE;
	
	float RunElapsedSeconds = 0.f;
	
	bool  bDifficultyTimerRunning = false;
};
