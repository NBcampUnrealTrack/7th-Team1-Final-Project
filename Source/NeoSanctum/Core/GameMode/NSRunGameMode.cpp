// Copyright 2026 One Team. All rights reserved.


#include "NSRunGameMode.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSGameFlowSubsystem.h"
#include "NeoSanctum/Core/Stage/NSStageManager.h"
#include "NeoSanctum/Core/Stage/NSMonsterPoolManager.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "NeoSanctum/Combat/Projectile/NSProjectileManagerComponent.h"
#include "NeoSanctum/Combat/Projectile/NSProjectileReplicationProxy.h"


ANSRunGameMode::ANSRunGameMode()
{
	bUseSeamlessTravel = true;
	
	GameStateClass = ANSRunGameState::StaticClass();
	PlayerControllerClass = ANSPlayerController::StaticClass();
	PlayerStateClass = ANSPlayerState::StaticClass();
	DefaultPawnClass = nullptr;

	ProjectileReplicationProxyClass = ANSProjectileReplicationProxy::StaticClass();
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

	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		EnsureProjectileProxy(*It);
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

	OpenRunEndVote(false);
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

	OpenRunEndVote(true);
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
	if (!HasAuthority())
	{
		return;
	}

	if (UNSGameFlowSubsystem* NSGameFlow = GetGameInstance()->GetSubsystem<UNSGameFlowSubsystem>())
	{
		NSGameFlow->ReturnToHub();
	}
}

void ANSRunGameMode::RequestMoveToNextStage_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UNSGameFlowSubsystem* NSGameFlow = GetGameInstance()->GetSubsystem<UNSGameFlowSubsystem>())
	{
		NSGameFlow->AdvanceToNextStage();
	}
}

void ANSRunGameMode::ReturnMonsterToPool_Implementation(ACharacter* Monster)
{
	if (!HasAuthority() || !NSMonsterPoolManager)
	{
		return;
	}

	// 몬스터가 살아있는채로 반환되는 경우에는 수동으로 카운트 줄임
	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(Monster);
	if (Enemy && !Enemy->IsDead() && NSStageManager)
	{
		NSStageManager->AddEnemyCount(-1);
	}

	NSMonsterPoolManager->ReturnMonsterToPool(Monster);
}

ANSEnemyCharacterBase* ANSRunGameMode::RequestSpawnMonster_Implementation(
	UClass* CharacterClass,
	UNSEnemyData* EnemyData,
	const FVector& Location,
	const FRotator& Rotation)
{
	if (!HasAuthority() || !NSMonsterPoolManager || !CharacterClass || !EnemyData)
	{
		return nullptr;
	}

	ACharacter* Spawned = NSMonsterPoolManager->GetPooledMonster(
		CharacterClass,
		EnemyData,
		Location,
		Rotation);

	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(Spawned);

	if (Enemy && NSStageManager)
	{
		NSStageManager->AddEnemyCount(1);
	}

	return Enemy;
}

void ANSRunGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	EnsureProjectileProxy(NewPlayer);
}

void ANSRunGameMode::Logout(AController* Exiting)
{
	DestroyProjectileProxy(Cast<APlayerController>(Exiting));

	Super::Logout(Exiting);
}

void ANSRunGameMode::HandleSeamlessTravelPlayer(AController*& Controller)
{
	Super::HandleSeamlessTravelPlayer(Controller);

	EnsureProjectileProxy(Cast<APlayerController>(Controller));
}

void ANSRunGameMode::EnsureProjectileProxy(APlayerController* PlayerController)
{
	if (!HasAuthority() ||
		!IsValid(PlayerController) ||
		!ProjectileReplicationProxyClass)
	{
		return;
	}

	if (ANSProjectileReplicationProxy* ExistingProxy = ProjectileProxies.FindRef(PlayerController))
	{
		if (IsValid(ExistingProxy))
		{
			return;
		}
	}

	UNSProjectileManagerComponent* ProjectileManager = GetProjectileManager();

	if (!ProjectileManager)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = PlayerController;
	SpawnParameters.Instigator = PlayerController->GetPawn();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ANSProjectileReplicationProxy* Proxy = GetWorld()->SpawnActor<ANSProjectileReplicationProxy>(
		ProjectileReplicationProxyClass,
		FTransform::Identity,
		SpawnParameters);

	if (!IsValid(Proxy))
	{
		return;
	}

	ProjectileProxies.Add(PlayerController, Proxy);
	ProjectileManager->RegisterReplicationProxy(Proxy);
}

void ANSRunGameMode::DestroyProjectileProxy(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	ANSProjectileReplicationProxy* Proxy = ProjectileProxies.FindRef(PlayerController);

	if (IsValid(Proxy))
	{
		if (UNSProjectileManagerComponent* ProjectileManager = GetProjectileManager())
		{
			ProjectileManager->UnregisterReplicationProxy(Proxy);
		}

		Proxy->Destroy();
	}

	ProjectileProxies.Remove(PlayerController);
}

UNSProjectileManagerComponent* ANSRunGameMode::GetProjectileManager() const
{
	const ANSRunGameState* RunGameState =
		GetGameState<ANSRunGameState>();

	return RunGameState
		       ? RunGameState->FindComponentByClass<
			       UNSProjectileManagerComponent>()
		       : nullptr;
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

void ANSRunGameMode::OpenRunEndVote(bool bHubOnly)
{
	if (!HasAuthority())
	{
		return;
	}
	
	ANSRunGameState* NSGameState = GetGameState<ANSRunGameState>();
	if (!NSGameState || NSGameState->RunEndPhase != ENSRunEndPhase::None)
	{	
		return;
	}
	
	for (APlayerState* PlayerState : NSGameState->PlayerArray)
	{
		if (ANSPlayerState* NSPlayerState = Cast<ANSPlayerState>(PlayerState))
		{
			NSPlayerState->RunChoice =
				ENSRunChoice::ReturnToHub; NSPlayerState->bVoteConfirmed = false;
		}
	}
	
	NSGameState->bIsClear           = bHubOnly;
	NSGameState->SetRunEndPhase(ENSRunEndPhase::Voting);
	NSGameState->PhaseEndServerTime = NSGameState->GetServerWorldTimeSeconds() + VoteDuration;
	NSGameState->ForceNetUpdate();

	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&ANSRunGameMode::ResolveVote,
		VoteDuration, 
		false);
	
}

void ANSRunGameMode::HandlePlayerConfirmed()
{
	ANSRunGameState* NSGameState = GetGameState<ANSRunGameState>();
	if (!NSGameState || NSGameState->RunEndPhase != ENSRunEndPhase::Voting)
	{		
		return;
	}	
	
	for (APlayerState* PlayerState : NSGameState->PlayerArray)
	{
		ANSPlayerState* NSPlayerState = Cast<ANSPlayerState>(PlayerState);
		// 투표를 플레이어들이 아직 다 안했다면 진행 X
		if (!NSPlayerState || !NSPlayerState->bVoteConfirmed)
		{		
			return;
		}
	}
	
	// 플레이어 전원 투표 시작했다면 10초 안 기다리고 즉시 실행
	ResolveVote();
}

void ANSRunGameMode::ResolveVote()
{
	ANSRunGameState* NSGameState = GetGameState<ANSRunGameState>();
	if (!NSGameState || NSGameState->RunEndPhase != ENSRunEndPhase::Voting) return;
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);

	int32 Next = 0, Hub = 0;
	for (APlayerState* PlayerState : NSGameState->PlayerArray)
	{
		ANSPlayerState* NSPlayerState = Cast<ANSPlayerState>(PlayerState);
		const bool bNext = NSPlayerState && NSPlayerState->bVoteConfirmed
						   && NSPlayerState->RunChoice == ENSRunChoice::NextStage;
		bNext ? ++Next : ++Hub;
	}
	
	const bool bGoNext = !NSGameState->bIsClear && (Next > Hub);

	NSGameState->NextVotes        = Next;
	NSGameState->HubVotes         = Hub;
	NSGameState->WinningChoice    = bGoNext ? ENSRunChoice::NextStage : ENSRunChoice::ReturnToHub;
	NSGameState->SetRunEndPhase(ENSRunEndPhase::Result);
	NSGameState->PhaseEndServerTime = NSGameState->GetServerWorldTimeSeconds() + ResultDisplayDuration;
	NSGameState->ForceNetUpdate();

	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&ANSRunGameMode::OnResultDisplayFinished,
		ResultDisplayDuration,
		false);
}

void ANSRunGameMode::OnResultDisplayFinished()
{
	ANSRunGameState* NSGameState = GetGameState<ANSRunGameState>();
	const bool bGoNext = NSGameState && NSGameState->WinningChoice == ENSRunChoice::NextStage;

	if (NSGameState)
	{
		NSGameState->SetRunEndPhase(ENSRunEndPhase::None);
	}

	if (UNSGameFlowSubsystem* NSGameFlow = GetGameInstance()->GetSubsystem<UNSGameFlowSubsystem>())
	{
		if (bGoNext)
		{
			NSGameFlow->AdvanceToNextStage();
		}
		else
		{
			// 거점 귀환할 때 정보 저장
			SaveAllPlayersProgress();
			NSGameFlow->ReturnToHub();
		}
	}
}

void ANSRunGameMode::SaveAllPlayersProgress()
{
	if (!HasAuthority())
	{
		return;
	}

	ANSRunGameState* NSGameState = GetGameState<ANSRunGameState>();
	if (!NSGameState)
	{
		return;
	}

	for (APlayerState* PlayerState : NSGameState->PlayerArray)
	{
		ANSPlayerState* NSPlayerState = Cast<ANSPlayerState>(PlayerState);
		if (!NSPlayerState)
		{
			continue;
		}

		ANSPlayerController* NSPlayerController = Cast<ANSPlayerController>(NSPlayerState->GetPlayerController());
		if (!NSPlayerController)
		{
			continue;
		}

		NSPlayerController->SaveProgressToOwningClient();
	}
}

void ANSRunGameMode::SubmitRunChoice_Implementation(APlayerController* Voter, ENSRunChoice Choice)
{
	if (!HasAuthority() || !Voter)
	{
		return;
	}
	
	if (ANSPlayerState* NSPlayerState = Voter->GetPlayerState<ANSPlayerState>())
	{
		NSPlayerState->RunChoice = Choice;
		NSPlayerState->bVoteConfirmed = true;
	}
	// 전원 확인 시 ResolveVote
	HandlePlayerConfirmed();
}
