// Copyright 2026 One Team. All rights reserved.


#include "NSOutGameMode.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "NeoSanctum/Core/GameState/NSOutGameState.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSGameFlowSubsystem.h"

ANSOutGameMode::ANSOutGameMode()
{
	bUseSeamlessTravel = true;
	
	GameStateClass = ANSOutGameState::StaticClass();
	PlayerControllerClass = ANSPlayerController::StaticClass();
	PlayerStateClass = ANSPlayerState::StaticClass();
}

void ANSOutGameMode::RequestStartRun_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}
	
	ANSOutGameState* NSGameState = GetGameState<ANSOutGameState>();
	if (!NSGameState || !NSGameState->IsAllPlayersReady()) 
	{
		return;
	}
	
	if (UNSGameFlowSubsystem* NSGameFlow = GetGameInstance()->GetSubsystem<UNSGameFlowSubsystem>())
	{
		NSGameFlow->StartNewRun();
	}
}

AActor* ANSOutGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	// 플레이어의 고정 슬롯 인덱스 결정(PlayerArray 내 위치)
	int32 SlotIndex = 0;
	if (GameState && Player && Player->PlayerState)
	{
		const int32 FoundIndex = 
			GameState->PlayerArray.IndexOfByKey(Player->PlayerState);
		SlotIndex = (FoundIndex != INDEX_NONE) ? FoundIndex : 0;
	}

	const FName DesiredTag =
		*FString::Printf(TEXT("PlayerSpawn%d"), SlotIndex);

	APlayerStart* FallbackStart = nullptr;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* CandidateStart = *It;
		if (!CandidateStart)
		{
			continue;
		}
		
		// 최후 폴백
		if (!FallbackStart)
		{
			FallbackStart = CandidateStart;
		}    
		
		// 내 슬롯 자리
		if (CandidateStart->PlayerStartTag == DesiredTag)
		{
			return CandidateStart;                                  
		}
	}
	
	return FallbackStart ? FallbackStart : Super::FindPlayerStart_Implementation(Player, IncomingName);
}

void ANSOutGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	// TODO: 플레이어 입장 시 SaveData에서 진행도 받아서 연동해야함
}
