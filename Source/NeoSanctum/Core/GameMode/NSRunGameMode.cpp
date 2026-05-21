// Copyright 2026 One Team. All rights reserved.


#include "NSRunGameMode.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/GameInstance/NSGameInstance.h"


ANSRunGameMode::ANSRunGameMode()
{
	GameStateClass = ANSRunGameState::StaticClass();
	PlayerControllerClass = ANSPlayerController::StaticClass();
	PlayerStateClass = ANSPlayerState::StaticClass();
	
	bUseSeamlessTravel = true;
}

void ANSRunGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	// TODO: 후에 StageManager 구현 후 연결해야함
}

void ANSRunGameMode::NotifyStageCleared_Implementation()
{
	HandleRunOver(true);
}

void ANSRunGameMode::NotifyPlayerDied_Implementation(AController* DeadPlayer)
{
	bool bIsAllPlayersDead = true;

	// 게임의 모든 플레이어 컨트롤러 순회
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (PlayerController)
		{
			// 컨트롤러가 소유한 플레이어 스테이트 캐스팅
			ANSPlayerState* NSPlayerState = PlayerController->GetPlayerState<ANSPlayerState>();
			if (NSPlayerState)
			{
				// TODO: 후에 플레이어 스테이트의 상태 태그(Dead) 체크해서 전멸 판정 로직 추가해야함
			}
		}
	}

	if (bIsAllPlayersDead)
	{
		HandleRunOver(false);
	}
}


void ANSRunGameMode::HandleRunOver(bool bIsClear)
{
	UNSGameInstance* NSGameInstance = Cast<UNSGameInstance>(GetGameInstance());
	if (NSGameInstance)
	{
		if (bIsClear)
		{
			UE_LOG(LogTemp, Log, TEXT("런 클리어 보상 지급"));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("런 전멸 실패 보상 지급"));
		}
	}

	GetWorld()->ServerTravel("/Game/TestSpace/TestInGame?listen");
}
