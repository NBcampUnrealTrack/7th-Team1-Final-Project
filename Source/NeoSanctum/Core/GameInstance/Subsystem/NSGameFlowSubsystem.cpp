// Copyright 2026 One Team. All rights reserved.


#include "NSGameFlowSubsystem.h"

#include "NSDataSubsystem.h"
#include "NeoSanctum/Core/Interface/NSGameInstanceInterface.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Data/Config/NSLevelCatalog.h"
#include "NeoSanctum/Data/Config/NSRunConfig.h"
#include "NeoSanctum/Data/Config/NSLevelConfig.h"
#include "NeoSanctum/Data/Config/NSDifficultyConfig.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"


static void SyncDifficultyTimerToGameState(
	const UObject* WorldContext,
	bool bRunning,
	float ElapsedSeconds,
	int32 StageNumber)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	ANSRunGameState* RunGameState = World->GetGameState<ANSRunGameState>();
	if (!RunGameState)
	{
		return;
	}

	RunGameState->SetDifficultyTimerState(
		bRunning,
		ElapsedSeconds,
		StageNumber);
}

UNSLevelCatalog* UNSGameFlowSubsystem::GetCatalog() const
{
	
	if (INSGameInstanceInterface* GameInstanceInterface = Cast<INSGameInstanceInterface>(GetGameInstance()))
	{
		return GameInstanceInterface->GetLevelCatalog();
	}
	return nullptr;
}

bool UNSGameFlowSubsystem::ServerTravelToWorld(const TSoftObjectPtr<UWorld>& Level, const FString& Options, bool bIsInRunTravel)
{
	UWorld* World = GetWorld();
	
	if (!World || World->GetNetMode() == NM_Client)
	{
		// 서버(호스트)만
		return false;
	}
	
	if (Level.IsNull())
	{
		return false;
	}
	
	FString URL = Level.ToSoftObjectPath().GetLongPackageName();
	
	if (!Options.IsEmpty())
	{
		URL += Options;
	}
	UE_LOG(LogTemp, Warning, TEXT("[TravelLoading] Show 호출: ServerTravelToWorld InRun=%d NetMode=%d"),
	bIsInRunTravel ? 1 : 0, (int32)GetWorld()->GetNetMode());
	
	// 클라는 자동으로 따라옴
	if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this))
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (APlayerController* LocalPlayerController = GameInstance->GetFirstLocalPlayerController())
			{
				UIManager->ShowTravelLoadingScreen(LocalPlayerController, bIsInRunTravel);
			}
		}
	}

	return World->ServerTravel(URL);
}

void UNSGameFlowSubsystem::StartNewRun()
{
	StopAndResetDifficultyTimer();
	
	UNSLevelCatalog* NSCatalog = GetCatalog();
	
	if (!NSCatalog || NSCatalog->InRunLevels.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("인 런 레벨 없음 - 런 시작 불가"));
		return;
	}
	CurrentStageNumber = 1;
	// 기획상 스테이지 게임 시작은 무조건 스테이지 1부터(후에 수정될수도 있음)
	CurrentInRunIndex = 0; 
	
	RequestEnterRun(NSCatalog->RunConfig, NSCatalog->InRunLevels[0].LevelConfig);
}

int32 UNSGameFlowSubsystem::PickNextInRunIndex() const
{
	UNSLevelCatalog* NSCatalog = GetCatalog();
	
	const int32 Num = NSCatalog ? NSCatalog->InRunLevels.Num() : 0;
	
	if (Num <= 0)
	{
		return INDEX_NONE;
	}	
	
	if (Num == 1) 
	{		
		return 0;
	}
	
	int32 Next = CurrentInRunIndex;
	
	// 직전 레벨 제외
	while (Next == CurrentInRunIndex)
	{
		Next = FMath::RandRange(0, Num - 1);
	} 
	
	return Next;
}

bool UNSGameFlowSubsystem::RequestEnterRun(
	const TSoftObjectPtr<UNSRunConfig>& RunConfig, const TSoftObjectPtr<UNSLevelConfig>& LevelConfig)
{
	if (RunConfig.IsNull() || LevelConfig.IsNull())
	{
		return false;
	}
	
	SelectedRunConfig = RunConfig;
	SelectedRunLevelConfig = LevelConfig;
	
	UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem)
	{
		return false;
	}
	
	DataSubsystem->OnRunGameDataReady.RemoveDynamic(this, &UNSGameFlowSubsystem::HandleRunGameDataReady);
	DataSubsystem->OnRunGameDataReady.AddDynamic(this, &UNSGameFlowSubsystem::HandleRunGameDataReady);
	
	DataSubsystem->EnterRun(RunConfig, LevelConfig);
	return true;
}

void UNSGameFlowSubsystem::HandleRunGameDataReady()
{
	UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem)
	{
		return;
	}
	
	DataSubsystem->OnRunGameDataReady.RemoveDynamic(this, &UNSGameFlowSubsystem::HandleRunGameDataReady);
	
	const UNSLevelConfig* LevelConfig = DataSubsystem->GetCurrentRunLevelConfig();
	if (!IsValid(LevelConfig))
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	// 서버는 travel 전에 필요한 데이터를 선로딩하고
	// 클라이언트는 인런 월드 진입 후 RunGameState 복제를 통해 로드
	ServerTravelToWorld(LevelConfig->TravelMap, FString(), true);
}

bool UNSGameFlowSubsystem::AdvanceToNextStage()
{
	UNSLevelCatalog* Catalog = GetCatalog();
	const int32 Num = Catalog ? Catalog->InRunLevels.Num() : 0;
	if (Num <= 0)
	{
		return false;
	}

	int32 NextIndex;
	// 첫 사이클은 스테이지 순서대로 나오도록 고정
	if (CurrentStageNumber < Num)
	{
		NextIndex = CurrentStageNumber; 
	}
	// 스테이지 1~3의 한 사이클이 끝났다면 그 이후부터는 랜덤으로 레벨이 나오도록함
	else
	{
		NextIndex = PickRandomIndexExcludingCurrent();
	}

	if (!Catalog->InRunLevels.IsValidIndex(NextIndex))
	{
		return false;
	}
	CurrentInRunIndex = NextIndex;
	CurrentStageNumber++;
	
	PauseDifficultyTimer();
	SetDifficultyTimerWaitingForReady(true);
	
	return RequestEnterRun(Catalog->RunConfig, Catalog->InRunLevels[NextIndex].LevelConfig);
}

bool UNSGameFlowSubsystem::ReturnToHub()
{
	StopAndResetDifficultyTimer();
	
	UNSLevelCatalog* Catalog = GetCatalog();
	
	return Catalog ? ServerTravelToWorld(
		Catalog->HubLevel,
		FString(), 
		false) : false;
}

int32 UNSGameFlowSubsystem::PickRandomIndexExcludingCurrent() const
{
	UNSLevelCatalog* Catalog = GetCatalog();
	const int32 Num = Catalog ? Catalog->InRunLevels.Num() : 0;
	if (Num <= 0)
	{
		return INDEX_NONE;
	}
	
	if (Num == 1)
	{
		return 0;
	}
	
	int32 Next = CurrentInRunIndex;
	while (Next == CurrentInRunIndex)
	{
		Next = FMath::RandRange(0, Num - 1);
	}
	
	return Next;
}

void UNSGameFlowSubsystem::ResumeDifficultyTimer()
{
	if (bDifficultyTimerWaitingForReady)
	{
		return;
	}

	bDifficultyTimerRunning = true;

	SyncDifficultyTimerToGameState(
		this,
		true,
		RunElapsedSeconds,
		CurrentStageNumber);
}

void UNSGameFlowSubsystem::PauseDifficultyTimer()
{
	bDifficultyTimerRunning = false;

	SyncDifficultyTimerToGameState(
		this,
		false,
		RunElapsedSeconds,
		CurrentStageNumber);
}

void UNSGameFlowSubsystem::StopAndResetDifficultyTimer()
{
	bDifficultyTimerRunning = false;
	RunElapsedSeconds = 0.0f;

	SyncDifficultyTimerToGameState(
		this,
		false,
		RunElapsedSeconds,
		CurrentStageNumber);
}

void UNSGameFlowSubsystem::RestartDifficultyTimer()
{
	bDifficultyTimerWaitingForReady = false;
	
	StopAndResetDifficultyTimer();
	ResumeDifficultyTimer();
}

void UNSGameFlowSubsystem::SetDifficultyTimerWaitingForReady(bool bWaiting)
{
	bDifficultyTimerWaitingForReady = bWaiting;
}

int32 UNSGameFlowSubsystem::GetDifficultyLevel() const
{
	const float Interval = GetDifficultyTimeStepInterval();
	
	if (Interval <= 0.0f)
	{
		return 1;
	}
	
	return FMath::FloorToInt(RunElapsedSeconds / Interval) + 1;
}

float UNSGameFlowSubsystem::GetDifficultyProgressPercent() const
{
	const float Interval = GetDifficultyTimeStepInterval();
	
	if (Interval <= 0.0f)
	{
		return 0.0f;
	}
	
	const float CurrentStepElapsed =
		FMath::Fmod(RunElapsedSeconds, Interval);
	
	return FMath::Clamp(CurrentStepElapsed / Interval, 0.0f, 1.0f);
}

float UNSGameFlowSubsystem::GetDifficultyTimeStepInterval() const
{
	const UNSDifficultyConfig* DifficultyConfig = nullptr;
	
	if (const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		if (const UNSRunConfig* RunConfig = DataSubsystem->GetCurrentRunConfig())
		{
			DifficultyConfig = RunConfig->DifficultyConfig.Get();
		}
	}
	
	if (!DifficultyConfig)
	{
		return 60.0f;
	}
	
	return FMath::Max(DifficultyConfig->TimeStepInterval, 1.0f);
}

FNSDifficultyScale UNSGameFlowSubsystem::GetCurrentMonsterScale(int32 PlayerCount) const
{
	const UNSDifficultyConfig* DifficultyConfig = nullptr;
	
	if (const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		if (const UNSRunConfig* RunConfig = DataSubsystem->GetCurrentRunConfig())
		{
			DifficultyConfig = RunConfig->DifficultyConfig.Get();
		}
	}
	
	// config 미설정 시 기본값(1배) 폴백
	if (!DifficultyConfig)
	{
		return FNSDifficultyScale();
	}
	
	return DifficultyConfig->Evaluate(
		RunElapsedSeconds,
		CurrentStageNumber,
		PlayerCount);
}

void UNSGameFlowSubsystem::Tick(float DeltaTime)
{
	if (!bDifficultyTimerRunning)
	{
		return;
	}
	
	const UWorld* World = GetWorld();
	
	// 서버만 누적
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	} 
	
	RunElapsedSeconds += DeltaTime;
}

TStatId UNSGameFlowSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UNSGameFlowSubsystem, STATGROUP_Tickables);
}
