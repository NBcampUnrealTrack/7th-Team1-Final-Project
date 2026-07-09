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
	UE_LOG(LogTemp, Warning, TEXT("[OutSpawn] FindPlayerStart 호출 Player=%s"), *GetNameSafe(Player));
	// 플레이어의 고정 슬롯 인덱스 결정(PlayerArray 내 위치)
	int32 SlotIndex = 0;

	if (ANSPlayerState* NSPS = Player ? Player->GetPlayerState<ANSPlayerState>() : nullptr)
	{
		// 아직 슬롯이 없으면 여기서 부여
		if (NSPS->GetPlayerSlotIndex() == INDEX_NONE)
		{
			NSPS->SetPlayerSlotIndex(FindFreeSlotIndex(NSPS));
		}
		SlotIndex = NSPS->GetPlayerSlotIndex();
	}

	const FName DesiredTag =
		*FString::Printf(TEXT("PlayerSpawn%d"), SlotIndex);
	
	UE_LOG(LogTemp, Warning, TEXT("[OutSpawn] SlotIndex=%d DesiredTag=%s"), SlotIndex, *DesiredTag.ToString());

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

	if (!HasAuthority() || !NewPlayer)
	{
		return;
	}

	ANSPlayerState* NSPS = NewPlayer->GetPlayerState<ANSPlayerState>();
	if (!NSPS)
	{
		return;
	}

	// 심리스 이월로 이미 슬롯이 있으면 유지
	if (NSPS->GetPlayerSlotIndex() != INDEX_NONE)
	{
		return;
	}

	NSPS->SetPlayerSlotIndex(FindFreeSlotIndex(NSPS));
}

int32 ANSOutGameMode::FindFreeSlotIndex(const APlayerState* Requester) const
{
	// 현재 사용 중인 슬롯 수집
	TSet<int32> UsedSlots;
	if (GameState)
	{
		for (APlayerState* OtherPS : GameState->PlayerArray)
		{
			if (!OtherPS || OtherPS == Requester)
			{
				continue;
			}
			if (const ANSPlayerState* OtherNSPS = Cast<ANSPlayerState>(OtherPS))
			{
				const int32 Slot = OtherNSPS->GetPlayerSlotIndex();
				if (Slot != INDEX_NONE)
				{
					UsedSlots.Add(Slot);
				}
			}
		}
	}

	// 0부터 빈 번호 탐색
	int32 Candidate = 0;
	while (UsedSlots.Contains(Candidate))
	{
		++Candidate;
	}
	
	return Candidate;
}
