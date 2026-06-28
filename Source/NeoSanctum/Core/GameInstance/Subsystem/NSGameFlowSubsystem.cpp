// Copyright 2026 One Team. All rights reserved.


#include "NSGameFlowSubsystem.h"

#include "NeoSanctum/Core/Interface/NSGameInstanceInterface.h"
#include "NeoSanctum/Data/Config/NSLevelCatalog.h"
#include "NeoSanctum/Data/Config/NSDifficultyConfig.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"



UNSLevelCatalog* UNSGameFlowSubsystem::GetCatalog() const
{
	
	if (INSGameInstanceInterface* GameInstanceInterface = Cast<INSGameInstanceInterface>(GetGameInstance()))
	{
		return GameInstanceInterface->GetLevelCatalog();
	}
	return nullptr;
}

bool UNSGameFlowSubsystem::ServerTravelToWorld(const TSoftObjectPtr<UWorld>& Level, const FString& Options)
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
	
	// 클라는 자동으로 따라옴
	if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this))
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (APlayerController* LocalPlayerController = GameInstance->GetFirstLocalPlayerController())
			{
				UIManager->ShowTravelLoadingScreen(LocalPlayerController);
			}
		}
	}

	return World->ServerTravel(URL);
}

void UNSGameFlowSubsystem::StartNewRun()
{
	UNSLevelCatalog* NSCatalog = GetCatalog();
	
	if (!NSCatalog || NSCatalog->InRunLevels.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("인 런 레벨 없음 - 런 시작 불가"));
		return;
	}
	CurrentStageNumber = 1;
	// 기획상 스테이지 게임 시작은 무조건 스테이지 1부터(후에 수정될수도 있음)
	CurrentInRunIndex = 0; 
	
	ServerTravelToWorld(NSCatalog->InRunLevels[0].Level, FString());
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
	
	return ServerTravelToWorld(
		Catalog->InRunLevels[NextIndex].Level,
		FString());
}

bool UNSGameFlowSubsystem::ReturnToHub()
{
	UNSLevelCatalog* Catalog = GetCatalog();
	
	return Catalog ? ServerTravelToWorld(
		Catalog->HubLevel,
		FString()) : false;
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
	bDifficultyTimerRunning = true;
}

void UNSGameFlowSubsystem::PauseDifficultyTimer()
{
	bDifficultyTimerRunning = false;
}

void UNSGameFlowSubsystem::StopAndResetDifficultyTimer()
{
	bDifficultyTimerRunning = false;
	RunElapsedSeconds = 0.0f;
}

FNSDifficultyScale UNSGameFlowSubsystem::GetCurrentMonsterScale(int32 PlayerCount) const
{
	const UNSDifficultyConfig* DifficultyConfig = nullptr;
	if (INSGameInstanceInterface* GII = Cast<INSGameInstanceInterface>(GetGameInstance()))
	{
		DifficultyConfig = GII->GetDifficultyConfig();
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
