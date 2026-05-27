// Copyright 2026 One Team. All rights reserved.


#include "NSRunGameMode.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/GameInstance/NSGameInstance.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"


ANSRunGameMode::ANSRunGameMode()
{
	bUseSeamlessTravel = true;
	
	GameStateClass = ANSRunGameState::StaticClass();
	PlayerControllerClass = ANSPlayerController::StaticClass();
	PlayerStateClass = ANSPlayerState::StaticClass();
	DefaultPawnClass = nullptr;
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

void ANSRunGameMode::RequestReturnToHub_Implementation()
{
	GetWorld()->ServerTravel("/Game/NeoSanctum/Map/L_HideOut");
}

void ANSRunGameMode::RequestMoveToNextStage_Implementation()
{
	GetWorld()->ServerTravel("/Game/NeoSanctum/Map/L_CanyonPlay");
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
	
	// TODO: 후에 플레이어 선택지 띄우는 UI 호출 함수 연동해야함
}

void ANSRunGameMode::RespawnAllPlayers()
{
	if (!HasAuthority())
	{
		return;
	}

	AGameStateBase* CurrentGameState = GetGameState<AGameStateBase>();
	if (!CurrentGameState)
	{
		return;
	}

	for (APlayerState* PlayerState : CurrentGameState->PlayerArray)
	{
		ANSPlayerController* NSPlayerController = Cast<ANSPlayerController>(PlayerState->GetPlayerController());
		if (NSPlayerController)
		{
			NSPlayerController->ExitSpectatorAndRespawn();
		}
	}
}

AActor* ANSRunGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	AActor* FirstFoundStart = nullptr;
	
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;
		if (PlayerStart && PlayerStart->PlayerStartTag == FName("PlayerSpawn"))
		{
			// 자리가 전부 찼을 때를 대비해서 첫번째 위치 저장
			if (!FirstFoundStart)
			{
				FirstFoundStart = PlayerStart;
			}

			// 발견한 자리에 캐릭터있는지 체크용
			FVector SpawnLocation = PlayerStart->GetActorLocation();
			FQuat SpawnRotation = PlayerStart->GetActorQuat();
			FCollisionShape CharacterCapsule = FCollisionShape::MakeCapsule(34.0f, 88.0f);
			
			TArray<FOverlapResult> Overlaps;
			FCollisionQueryParams QueryParams;
			
			bool bIsOverlapPlayer = false;
			
			if (GetWorld()->OverlapMultiByChannel(
				Overlaps,
				SpawnLocation,
				SpawnRotation,
				ECC_Pawn,
				CharacterCapsule, 
				QueryParams))
			{
				for (const FOverlapResult& Overlap : Overlaps)
				{
					AActor* OverlappedActor = Overlap.GetActor();
					
					if (OverlappedActor && OverlappedActor->IsA(APawn::StaticClass()))
					{
						bIsOverlapPlayer = true;
						break;
					}
				}
			}
			
			if (!bIsOverlapPlayer)
			{
				UE_LOG(LogTemp, Log, TEXT("GameMode: 비어있는 스폰 발견: %s"), *PlayerStart->GetName());
				return PlayerStart;
			}
		}
	}

	// 자리가 없으면 첫번째 PlayerStart 위치로 강제 소환
	if (FirstFoundStart)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameMode: 자리 X 강제 스폰"));
		return FirstFoundStart;
	}

	// 태그 있는 PlayerStart 없으면 기본 스폰 위치로 스폰
	return Super::FindPlayerStart_Implementation(Player, IncomingName);
}

void ANSRunGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// 런 레벨 시작시 캐릭터 자동 스폰 방지용
}
