// Copyright 2026 One Team. All rights reserved.


#include "NSRunGameMode.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/GameInstance/NSGameInstance.h"
#include "NeoSanctum/Core/Stage/NSStageManager.h"
#include "NeoSanctum/Core/Stage/NSMonsterPoolManager.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
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
	
	if (HasAuthority())
	{
		NSStageManager = NewObject<UNSStageManager>(this);
		// 클리어 판정 알림용 바인딩
		NSStageManager->OnStageCleared.BindUObject(
			this,
			&ANSRunGameMode::NotifyStageCleared_Implementation
		);
		
		NSMonsterPoolManager = NewObject<UNSMonsterPoolManager>(this);
	}
	
	
}

void ANSRunGameMode::NotifyStageCleared_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	// 맵 생성 중 적 카운팅 잘못되었을수도 있어서 검증용 추가 로직
	int32 ActualAliveEnemies = 0;
	for (TActorIterator<ANSEnemyCharacterBase> It(GetWorld()); It; ++It)
	{
		ANSEnemyCharacterBase* Enemy = *It;
		if (Enemy && !Enemy->IsDead() && !Enemy->IsInPool())
		{
			ActualAliveEnemies++;
		}
	}

	if (ActualAliveEnemies > 0)
	{
		// 카운팅 불일치 스테이지 매니저에서 보정
		if (NSStageManager)
		{
			NSStageManager->SetEnemyCount(ActualAliveEnemies);
		}
		UE_LOG(LogTemp, Warning, TEXT("적 카운팅 불일치 현재 남은 적: %d"), ActualAliveEnemies);
		return;
	}

	HandleRunOver(true);
}

void ANSRunGameMode::NotifyPlayerDied_Implementation(AController* DeadPlayer)
{
	if (!HasAuthority())
	{
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ANSPlayerController* NSPlayerController = Cast<ANSPlayerController>(It->Get());
		if (!NSPlayerController)
		{
			continue;
		}

		// 죽은 플레이어는 제외
		if (NSPlayerController == DeadPlayer)
		{
			continue;
		}

		// PlayerState 기반 생존 확인
		ANSPlayerState* NSPlayerState = NSPlayerController->GetPlayerState<ANSPlayerState>();
		if (NSPlayerState && !NSPlayerState->IsDead())
		{
			return;
		}
	}

	HandleRunOver(false);
}

void ANSRunGameMode::NotifyEnemyKilled_Implementation(ACharacter* DeadEnemy)
{
	if (!HasAuthority() || !DeadEnemy)
	{
		return;
	}

	if (NSStageManager)
	{
		NSStageManager->HandleEnemyKilled();
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

void ANSRunGameMode::ReturnMonsterToPool_Implementation(ACharacter* Monster)
{
	if (HasAuthority() && NSMonsterPoolManager)
	{
		NSMonsterPoolManager->ReturnMonsterToPool(Monster);
	}

}

void ANSRunGameMode::RequestSpawnMonster_Implementation(
	UClass* CharacterClass, 
	UNSEnemyData* EnemyData, 
	const FVector& Location,
	const FRotator& Rotation)
{
	if (!HasAuthority() || !NSMonsterPoolManager || !CharacterClass || !EnemyData)
	{
		return;
	}

	ACharacter* Spawned = NSMonsterPoolManager->GetPooledMonster(
		CharacterClass,
		EnemyData,
		Location, 
		Rotation);
	if (Spawned && NSStageManager)
	{
		NSStageManager->AddEnemyCount(1);
	}
}

void ANSRunGameMode::HandleRunOver(bool bIsClear)
{
	UNSGameInstance* NSGameInstance = Cast<UNSGameInstance>(GetGameInstance());
	if (NSGameInstance)
	{
		if (bIsClear)
		{
			UE_LOG(LogTemp, Log, TEXT("런 클리어"));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("전멸"));
		}
	}

	// 모든 플레이어에게 게임 종료 UI 표시
	AGameStateBase* CurrentGameState = GetGameState<AGameStateBase>();
	if (!CurrentGameState)
	{
		return;
	}

	for (APlayerState* PlayerState : CurrentGameState->PlayerArray)
	{
		ANSPlayerController* NSPlayerController = Cast<ANSPlayerController>(
			PlayerState->GetPlayerController()
		);
		if (NSPlayerController)
		{
			NSPlayerController->Client_ShowRunOverUI(bIsClear);
		}
	}
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

void ANSRunGameMode::SetEnemyCount(int32 Count)
{
	if (NSStageManager)
	{
		NSStageManager->SetEnemyCount(Count);
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
