// Copyright 2026 One Team. All rights reserved.


#include "NSRunGameMode.h"
#include "NeoSanctum/Progression/Currency/NSCurrencyComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Currency.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSGameFlowSubsystem.h"
#include "NeoSanctum/Core/Stage/NSStageManager.h"
#include "NeoSanctum/Core/Stage/NSMonsterPoolManager.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "NeoSanctum/Combat/Projectile/NSProjectileManagerComponent.h"
#include "NeoSanctum/Combat/Projectile/NSProjectileReplicationProxy.h"
#include "NeoSanctum/Progression/Currency/NSCurrencyReplicationProxy.h"
#include "NeoSanctum/System/Subsystem/NSCurrencyDropSubsystem.h"
#include "NeoSanctum/Data/Config/NSLevelConfig.h"
// 테스트용 임시 코드 (재화 드랍 테스트 — 드롭 테이블 연동 후 삭제)
#include "NeoSanctum/AI/Enemy/Spawner/NSSpawner.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/Stage/NSBossEntryVolume.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Progression/Augment/NSAugmentInventoryComponent.h"
#include "NeoSanctum/Progression/Augment/NSAugmentSelectionComponent.h"
#include "NeoSanctum/Progression/Part/NSPartEquipComponent.h"
#include "NeoSanctum/Progression/Reward/NSRewardHandler.h"
#include "NeoSanctum/Tag/NSGameplayTags_Reward.h"


ANSRunGameMode::ANSRunGameMode()
{
	bUseSeamlessTravel = true;
	
	GameStateClass = ANSRunGameState::StaticClass();
	PlayerControllerClass = ANSPlayerController::StaticClass();
	PlayerStateClass = ANSPlayerState::StaticClass();
	DefaultPawnClass = nullptr;

	ProjectileReplicationProxyClass = ANSProjectileReplicationProxy::StaticClass();
	CurrencyReplicationProxyClass = ANSCurrencyReplicationProxy::StaticClass();
}

void ANSRunGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		NSStageManager = NewObject<UNSStageManager>(this);
		// 클리어 판정 알림용 바인딩
		NSStageManager->OnObjectiveComplete.BindUObject(
			this,
			&ANSRunGameMode::HandleObjectiveComplete
		);
		
		NSMonsterPoolManager = NewObject<UNSMonsterPoolManager>(this);
		
		// 서버 보상 Roll이 매 실행마다 같은 순서로 고정되지 않도록 초기 시드 결정
		RewardRandomStream.Initialize(FMath::Rand());
		
		// 스테이지 타이머 시작(재개)
		if (UNSGameFlowSubsystem* Flow = 
			GetGameInstance()->GetSubsystem<UNSGameFlowSubsystem>())
		{
			Flow->ResumeDifficultyTimer();
		}
	}

	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		EnsureProjectileProxy(*It);
		EnsureCurrencyProxy(*It);
	}
}

void ANSRunGameMode::NotifyStageCleared_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (NSMonsterPoolManager)
	{
		TArray<ANSEnemyCharacterBase*> AliveEnemies;
		for (TActorIterator<ANSEnemyCharacterBase> It(GetWorld()); It; ++It)
		{
			ANSEnemyCharacterBase* Enemy = *It;
			if (Enemy && !Enemy->IsDead() && !Enemy->IsInPool())
			{
				AliveEnemies.Add(Enemy);
			}
		}
		for (ANSEnemyCharacterBase* Enemy : AliveEnemies)
		{
			NSMonsterPoolManager->ReturnMonsterToPool(Enemy);
		}
		UE_LOG(LogTemp, Log, TEXT("[StageClear] 남은 적 %d마리 풀로 반환"), AliveEnemies.Num());
	}
	
	// 스테이지 클리어 영구재화(공용/직업)를 각 플레이어 버킷에 적립
	if (ANSRunGameState* NSGameState = GetGameState<ANSRunGameState>())
	{
		for (APlayerState* PlayerState : NSGameState->PlayerArray)
		{
			if (ANSPlayerState* NSPlayerState = Cast<ANSPlayerState>(PlayerState))
			{
				if (UNSCurrencyComponent* Currency = NSPlayerState->GetCurrencyComponent())
				{
					Currency->AddPermanentDirect(
						NSGameplayTags::Currency_Common,
						StageClearCommonReward);
				}
			}
		}
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

void ANSRunGameMode::NotifyEnemyKilled_Implementation(AActor* DeadEnemy)
{
	if (!HasAuthority() || !DeadEnemy)
	{
		return;
	}

	// 킬 집계,보상은 페이즈 무관하게 항상 진행
	ANSRunGameState* RunGS = GetGameState<ANSRunGameState>();
	if (RunGS)
	{
		RunGS->AddRunResultKillCount();
	}
	
	HandleEnemyReward(DeadEnemy);
	HandleEnemyExperience(DeadEnemy);

	// 목표 진행은 Objective 페이즈에서만
	if (RunGS && RunGS->StagePhase ==
		ENSStagePhase::Objective && NSStageManager)
	{
		NSStageManager->HandleEnemyKilled();
		PushObjectiveStateToGameState();
	}
	// BossFight 페이즈에서 보스 랭크 사망 시 스테이지 클리어
	else if (RunGS && RunGS->StagePhase == ENSStagePhase::BossFight && IsBossEnemy(DeadEnemy))
	{
		NotifyStageCleared_Implementation();
	}
}

void ANSRunGameMode::HandleEnemyReward(AActor* DeadEnemy)
{
	if (!IsValid(DeadEnemy))
	{
		return;
	}

	FGameplayTag TriggerTag;
	if (!TryGetRewardTriggerTagFromEnemy(DeadEnemy, TriggerTag))
	{
		return;
	}
	
	UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem)
	{
		NS_LOG(LogNS, Warning,
			"Enemy Reward 처리에 필요한 DataSubsystem이 유효하지 않습니다. TriggerTag={TriggerTag}",
			("TriggerTag", TriggerTag.ToString())
		);
		return;
	}
	
	const UNSRewardDataRegistry* RewardDataRegistry = DataSubsystem->GetRewardDataRegistry();
	if (!RewardDataRegistry)
	{
		NS_LOG(LogNS, Warning,
			"Enemy Reward 처리에 필요한 RewardDataRegistry가 유효하지 않습니다. TriggerTag={TriggerTag}",
			("TriggerTag", TriggerTag.ToString())
		);
		return;
	}
	
	UNSRewardHandler::HandleRewardTrigger(
		GetWorld(),
		RewardDataRegistry,
		TriggerTag,
		DeadEnemy->GetActorLocation(),
		RewardRandomStream,
		RewardDroppedPartClass,
		RewardCurrencyDropDuration
	);
}

bool ANSRunGameMode::TryGetRewardTriggerTagFromEnemy(
	const AActor* DeadEnemy, FGameplayTag& OutTriggerTag) const
{
	OutTriggerTag = FGameplayTag();

	const UNSEnemyCoreComponent* CoreComponent =
		DeadEnemy ? DeadEnemy->FindComponentByClass<UNSEnemyCoreComponent>() : nullptr;
	if (!IsValid(CoreComponent))
	{
		return false;
	}

	const UNSEnemyData* EnemyData = CoreComponent->GetEnemyData();
	if (!EnemyData)
	{
		NS_LOG(LogNS, Warning,
			"Enemy Reward Trigger를 결정할 수 없습니다. EnemyData가 유효하지 않습니다. Enemy={Enemy}",
			("Enemy", GetNameSafe(DeadEnemy))
		);
		return false;
	}

	switch (EnemyData->EnemyRank)
	{
	case ENSEnemyRank::Normal:
		OutTriggerTag = NSGameplayTags::Reward_Trigger_NormalKill;
		return true;
		
	case ENSEnemyRank::Elite:
		OutTriggerTag = NSGameplayTags::Reward_Trigger_EliteKill;
		return true;
		
	case ENSEnemyRank::Boss:
		OutTriggerTag = NSGameplayTags::Reward_Trigger_BossKill;
		return true;
		
	default:
		NS_LOG(LogNS, Warning,
			"처리되지 않은 EnemyRank입니다. Enemy={Enemy}, EnemyRank={EnemyRank}",
			("Enemy", GetNameSafe(DeadEnemy)),
			("EnemyRank", static_cast<int32>(EnemyData->EnemyRank))
		);
		return false;
	}
}

void ANSRunGameMode::HandleEnemyExperience(AActor* DeadEnemy)
{
	if (!IsValid(DeadEnemy))
	{
		return;
	}

	const UNSEnemyCoreComponent* CoreComponent = DeadEnemy->FindComponentByClass<UNSEnemyCoreComponent>();
	if (!IsValid(CoreComponent))
	{
		NS_LOG(LogNS, Warning,
			"EnemyCoreComponent를 찾을 수 없어 경험치를 지급하지 못했습니다. Enemy={Enemy}",
			("Enemy", GetNameSafe(DeadEnemy))
		);
		return;
	}

	const float ExperienceReward = CoreComponent->GetExperienceReward();

	NS_LOG(LogNS, Log,
		"Enemy 처치로 경험치 지급을 요청합니다. Enemy={Enemy}, ExperienceReward={ExperienceReward}",
		("Enemy", GetNameSafe(DeadEnemy)),
		("ExperienceReward", ExperienceReward)
	);

	UNSRewardHandler::HandleExperienceRewardEntry(GetWorld(), ExperienceReward);
}

int32 ANSRunGameMode::GetPlayerSlotIndex(AController* Player) const
{
	if (GameState && Player && Player->PlayerState)
	{
		const int32 FoundIndex =
			GameState->PlayerArray.IndexOfByKey(Player->PlayerState);
		return (FoundIndex != INDEX_NONE) ? FoundIndex : 0;
	}
	
	return 0;
}

void ANSRunGameMode::TeleportAllPlayersToBossRoom()
{
	if (!HasAuthority())
	{
		return;
	}
	AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS)
	{
		return;
	}

	for (APlayerState* PS : GS->PlayerArray)
	{
		APlayerController* PC = 
			PS ? Cast<APlayerController>(PS->GetPlayerController()) : nullptr;
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		// 사망/관전 중이면 스킵
		if (!Pawn) 
		{
			continue;
		}

		const int32 SlotIndex = GetPlayerSlotIndex(PC);
		const FName DesiredTag = *FString::Printf(TEXT("PlayerBossStart%d"), SlotIndex);

		// 해당 슬롯의 보스 스타트만 탐색
		AActor* Target = nullptr;
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			if (*It && (*It)->PlayerStartTag == DesiredTag)
			{
				Target = *It;
				break;
			}
		}
		
		if (!Target)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BossEntry] %s 태그 PlayerStart를 찾지 못함"), *DesiredTag.ToString());
			
			continue;
		}

		Pawn->TeleportTo(Target->GetActorLocation(), Target->GetActorRotation());
	}
}

void ANSRunGameMode::ReturnStrayEnemiesToPool()
{
	if (!HasAuthority() || !NSMonsterPoolManager)
	{
		return;
	}
	
	// 순회 중 반환은 위험하므로 먼저 수집 후 별도 루프
	TArray<ANSEnemyCharacterBase*> AliveEnemies;
	for (TActorIterator<ANSEnemyCharacterBase> It(GetWorld()); It; ++It)
	{
		ANSEnemyCharacterBase* Enemy = *It;
		if (Enemy && !Enemy->IsDead() && !Enemy->IsInPool())
		{
			AliveEnemies.Add(Enemy);
		}
	}
	
	for (ANSEnemyCharacterBase* Enemy : AliveEnemies)
	{
		NSMonsterPoolManager->ReturnMonsterToPool(Enemy);
	}
	
}

void ANSRunGameMode::RequestReturnToHub_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	// 서버 인런 데이터 언로드 및 아웃런 데이터 재로드
	if (UNSDataSubsystem* Data = UNSDataSubsystem::Get(this))
	{
		Data->ReturnToOutGame();
	}

	// 각 클라이언트에 인런 데이터 언로드 지시
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ANSPlayerController* PC = Cast<ANSPlayerController>(It->Get()))
		{
			PC->Client_NotifyReturnToHub();
		}
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
	
	int32 PlayerCount = GameState ? GameState->PlayerArray.Num() : 1; 
	FNSDifficultyScale Scale;
	if (UNSGameFlowSubsystem* Flow = GetGameInstance()->GetSubsystem<UNSGameFlowSubsystem>())
	{
		Scale = Flow->GetCurrentMonsterScale(PlayerCount);
	}

	ACharacter* Spawned = NSMonsterPoolManager->GetPooledMonster(
		CharacterClass,
		EnemyData,
		Location,
		Rotation,
		Scale);

	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(Spawned);

	if (Enemy && NSStageManager)
	{
		NSStageManager->AddEnemyCount(1);
	}

	return Enemy;
}

void ANSRunGameMode::NotifyNPCRescued_Implementation(FName RescuedNPCId)
{
	if (!HasAuthority() || !NSStageManager)
	{
		return;
	}

	const ANSRunGameState* RunGS = GetGameState<ANSRunGameState>();
	if (!RunGS || RunGS->StagePhase != ENSStagePhase::Objective)
	{
		return;
	}

	NSStageManager->NotifyNPCRescued(RescuedNPCId);
	
	PushObjectiveStateToGameState();
}

void ANSRunGameMode::NotifyBossGateReached_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}
	
	ANSRunGameState* RunGS = GetGameState<ANSRunGameState>();
	// BossReady 상태에서만 1회 처리 (이중 트리거 방지)
	if (!RunGS || RunGS->StagePhase != ENSStagePhase::BossReady)
	{
		return;
	}

	ReturnStrayEnemiesToPool();

	// 보스 진입 시점에 난이도 타이머 정지 (보스 스케일 고정)
	if (UNSGameFlowSubsystem* Flow = GetGameInstance()->GetSubsystem<UNSGameFlowSubsystem>())
	{
		Flow->PauseDifficultyTimer();
	}

	TeleportAllPlayersToBossRoom();
	// 페이즈 BossFight로 전환
	RunGS->SetStagePhase(ENSStagePhase::BossFight);
	// 보스 스포너 활성화
	ActivateBossSpawners();
}

void ANSRunGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	EnsureProjectileProxy(NewPlayer);
	EnsureCurrencyProxy(NewPlayer);
}

void ANSRunGameMode::Logout(AController* Exiting)
{
	DestroyProjectileProxy(Cast<APlayerController>(Exiting));
	DestroyCurrencyProxy(Cast<APlayerController>(Exiting));

	Super::Logout(Exiting);
}

void ANSRunGameMode::HandleSeamlessTravelPlayer(AController*& Controller)
{
	Super::HandleSeamlessTravelPlayer(Controller);

	EnsureProjectileProxy(Cast<APlayerController>(Controller));
	EnsureCurrencyProxy(Cast<APlayerController>(Controller));
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

void ANSRunGameMode::EnsureCurrencyProxy(APlayerController* PlayerController)
{
	if (!HasAuthority() ||
		!IsValid(PlayerController) ||
		!CurrencyReplicationProxyClass)
	{
		return;
	}

	if (ANSCurrencyReplicationProxy* ExistingProxy = CurrencyProxies.FindRef(PlayerController))
	{
		if (IsValid(ExistingProxy))
		{
			return;
		}
	}

	UWorld* World = GetWorld();
	UNSCurrencyDropSubsystem* DropSys = World ? World->GetSubsystem<UNSCurrencyDropSubsystem>() : nullptr;
	if (!DropSys)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = PlayerController;
	SpawnParameters.Instigator = PlayerController->GetPawn();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ANSCurrencyReplicationProxy* Proxy = World->SpawnActor<ANSCurrencyReplicationProxy>(
		CurrencyReplicationProxyClass,
		FTransform::Identity,
		SpawnParameters);

	if (!IsValid(Proxy))
	{
		return;
	}

	CurrencyProxies.Add(PlayerController, Proxy);
	DropSys->RegisterProxy(Proxy);
}

void ANSRunGameMode::DestroyCurrencyProxy(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	ANSCurrencyReplicationProxy* Proxy = CurrencyProxies.FindRef(PlayerController);

	if (IsValid(Proxy))
	{
		if (UWorld* World = GetWorld())
		{
			if (UNSCurrencyDropSubsystem* DropSys = World->GetSubsystem<UNSCurrencyDropSubsystem>())
			{
				DropSys->UnregisterProxy(Proxy);
			}
		}

		Proxy->Destroy();
	}

	CurrencyProxies.Remove(PlayerController);
}

// 거점 귀환 시 영구 재화 커밋 + 지갑 비우기
void ANSRunGameMode::CommitAndClearAllWallets(float Multiplier)
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
		ANSPlayerState* PS = Cast<ANSPlayerState>(PlayerState);
		if (!PS)
		{
			continue;
		}
		if (UNSCurrencyComponent* Currency = PS->GetCurrencyComponent())
		{
			Currency->CommitRunPermanent(Multiplier);
			Currency->ClearWallet();
		}
	}
}

// 거점 귀환 시 인런 증강 Clear
void ANSRunGameMode::ClearAllAugments()
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
		ANSPlayerState* PS = Cast<ANSPlayerState>(PlayerState);
		if (!PS)
		{
			continue;
		}
		if (UNSAugmentInventoryComponent* Augment = PS->GetAugmentInventory())
		{
			Augment->ClearAll();
		}
	}
}

// 거점 귀환 시 인런 파츠 Clear
void ANSRunGameMode::ClearAllParts()
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
		ANSPlayerState* PS = Cast<ANSPlayerState>(PlayerState);
		if (!PS)
		{
			continue;
		}
		if (UNSPartEquipComponent* Part = PS->GetPartEquipComponent())
		{
			Part->ClearAll();
		}
	}
}

void ANSRunGameMode::ResetAugmentSelectionQueues()
{
	if (!HasAuthority())
	{
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ANSPlayerController* PC = Cast<ANSPlayerController>(It->Get()))
		{
			if (UNSAugmentSelectionComponent* SelectionComponent =
				PC->FindComponentByClass<UNSAugmentSelectionComponent>())
			{
				SelectionComponent->Reset();
			}
		}
	}
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
	
	SyncRunDataConfigToGameState();
}

void ANSRunGameMode::SetEnemyCount(int32 Count)
{
	if (NSStageManager)
	{
		NSStageManager->SetEnemyCount(Count);
	}
}

void ANSRunGameMode::InitializeStage()
{
	if (!HasAuthority())
	{
		return;
	}

	// 플레이어 스폰 + RunConfig/LevelConfig를 GameState에 복제
	RespawnAllPlayers();

	// 2) 목표 초기화: 인런 데이터가 준비됐으면 즉시 아니면 준비 완료 콜백에서 초기화
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Data)
	{
		return;
	}

	if (Data->IsRunReady())
	{
		InitializeObjectiveInternal();
	}
	else
	{
		// 중복 바인딩 방지 후 준비 완료 대기
		Data->OnRunGameDataReady.RemoveDynamic(
			this,
			&ANSRunGameMode::HandleRunDataReadyForObjective);
		Data->OnRunGameDataReady.AddDynamic(
			this,
			&ANSRunGameMode::HandleRunDataReadyForObjective);
	}
}



void ANSRunGameMode::InitializeObjectiveInternal()
{
	if (!NSStageManager)
	{
		return;
	}

	// 이미 로드된 현재 스테이지 LevelConfig에서 목표 풀 읽기
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	const UNSLevelConfig* LevelConfig = Data ? Data->GetCurrentRunLevelConfig() : nullptr;
	if (!LevelConfig || LevelConfig->ObjectivePool.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("ObjectivePool 비어있음 — 목표 초기화 실패"));
		
		return;
	}

	// 풀에서 랜덤 1개 선택
	const int32 Index = FMath::RandRange(
		0, 
		LevelConfig->ObjectivePool.Num() - 1);
	const int32 PlayerCount =
		GameState ? GameState->PlayerArray.Num() : 1;
	NSStageManager->InitializeObjective(
		LevelConfig->ObjectivePool[Index]
		, PlayerCount);

	// 스테이지 진입 페이즈 명시
	if (ANSRunGameState* RunGS = GetGameState<ANSRunGameState>())
	{
		RunGS->SetStagePhase(ENSStagePhase::Objective);
	}

	// 초기 목표 상태를 UI용으로 복제
	PushObjectiveStateToGameState();
}

void ANSRunGameMode::HandleRunDataReadyForObjective()
{
	if (UNSDataSubsystem* Data = UNSDataSubsystem::Get(this))
	{
		Data->OnRunGameDataReady.RemoveDynamic(
			this, 
			&ANSRunGameMode::HandleRunDataReadyForObjective);
	}
	
	InitializeObjectiveInternal();
}

void ANSRunGameMode::ActivateBossSpawners()
{
	if (!HasAuthority())
	{
		return;
	}
	// 월드의 보스 전용 스포너를 활성화
	for (TActorIterator<ANSSpawner> It(GetWorld()); It; ++It)
	{
		ANSSpawner* Spawner = *It;
		if (Spawner && Spawner->IsBossSpawner())
		{
			Spawner->ActivateSpawner();
			break;
		}
	}
}

bool ANSRunGameMode::IsBossEnemy(const AActor* DeadEnemy) const
{
	const UNSEnemyCoreComponent* CoreComponent =
		DeadEnemy ? DeadEnemy->FindComponentByClass<UNSEnemyCoreComponent>() : nullptr;
	if (!IsValid(CoreComponent))
	{
		return false;
	}
	
	const UNSEnemyData* EnemyData = CoreComponent->GetEnemyData();
	
	return EnemyData && EnemyData->EnemyRank == ENSEnemyRank::Boss;
}

AActor* ANSRunGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	// 플레이어의 고정 슬롯 인덱스 결정(PlayerArray 내 위치)
	const int32 SlotIndex = GetPlayerSlotIndex(Player);
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

void ANSRunGameMode::CancelRunChoice_Implementation(APlayerController* PlayerController)
{
	if (!HasAuthority() || !PlayerController)
	{
		return;
	}

	ANSPlayerState* NSPlayerState =
		PlayerController->GetPlayerState<ANSPlayerState>();
	if (!NSPlayerState || !NSPlayerState->bVoteConfirmed)
	{
		return;
	}

	ANSRunGameState* RunGameState = GetGameState<ANSRunGameState>();
	if (!RunGameState)
	{
		return;
	}

	NSPlayerState->bVoteConfirmed = false;

	if (NSPlayerState->RunChoice == ENSRunChoice::NextStage)
	{
		RunGameState->NextVotes = FMath::Max(RunGameState->NextVotes - 1, 0);
	}
	else
	{
		RunGameState->HubVotes = FMath::Max(RunGameState->HubVotes - 1, 0);
	}

	RunGameState->NotifyRunVoteChanged();
	RunGameState->ForceNetUpdate();

	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);

	RunGameState->PhaseEndServerTime =
		RunGameState->GetServerWorldTimeSeconds() + VoteDuration;

	RunGameState->OnRep_PhaseEndServerTime();

	RunGameState->SetRunEndPhase(ENSRunEndPhase::Voting);
	RunGameState->ForceNetUpdate();

	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&ANSRunGameMode::ResolveVote,
		VoteDuration,
		false);
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
			NSPlayerState->RunChoice = ENSRunChoice::ReturnToHub;
			NSPlayerState->bVoteConfirmed = false;
		}
	}
	
	NSGameState->bIsClear = !bHubOnly;
	NSGameState->PhaseEndServerTime =
		NSGameState->GetServerWorldTimeSeconds() + VoteDuration;
	NSGameState->OnRep_PhaseEndServerTime();
	NSGameState->NextVotes = 0;
	NSGameState->HubVotes = 0;
	NSGameState->NotifyRunVoteChanged();
	
	FNSRunResultData ResultData;

	for (APlayerState* PlayerState : NSGameState->PlayerArray)
	{
		const ANSPlayerState* NSPlayerState = Cast<ANSPlayerState>(PlayerState);
		if (!NSPlayerState)
		{
			continue;
		}

		const UNSCurrencyComponent* Currency = NSPlayerState->GetCurrencyComponent();
		if (!Currency)
		{
			continue;
		}

		ResultData.EarnedGoods += static_cast<int32>(Currency->GetTemp());
		ResultData.CommonGoods += static_cast<int32>(
			Currency->GetPermanent(NSGameplayTags::Currency_Common));
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UNSUIManagerSubsystem* UIManager =
			GameInstance->GetSubsystem<UNSUIManagerSubsystem>())
		{
			UIManager->CacheRunResultTime();
			ResultData.RunTimeSeconds = UIManager->GetRunResultTimeSeconds();
		}
	}

	ResultData.KillCount = NSGameState->RunResultData.KillCount;

	NSGameState->SetRunResultData(ResultData);
	
	NSGameState->SetRunEndPhase(ENSRunEndPhase::Voting);
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
	
	const bool bGoNext = NSGameState->bIsClear && (Next > Hub);

	NSGameState->NextVotes = Next;
	NSGameState->HubVotes = Hub;
	NSGameState->NotifyRunVoteChanged();
	NSGameState->WinningChoice = bGoNext
		? ENSRunChoice::NextStage
		: ENSRunChoice::ReturnToHub;
	NSGameState->PhaseEndServerTime =
		NSGameState->GetServerWorldTimeSeconds() + ResultDisplayDuration;

	NSGameState->OnRep_PhaseEndServerTime();

	NSGameState->SetRunEndPhase(ENSRunEndPhase::Result);
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
			// bIsClear 확인 필요
			const float Multiplier = (NSGameState && NSGameState->bIsClear) ? ClearMultiplier : FailMultiplier;
			CommitAndClearAllWallets(Multiplier);
			ClearAllAugments();
			ClearAllParts();
			ResetAugmentSelectionQueues();
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

void ANSRunGameMode::HandleObjectiveComplete()
{
	if (!HasAuthority())
	{
		return;
	}

	if (ANSRunGameState* NSGameState = GetGameState<ANSRunGameState>())
	{
		NSGameState->SetStagePhase(ENSStagePhase::BossReady);
		UE_LOG(LogTemp, Log, TEXT("[Stage] 페이즈 전환 → BossReady"));
	}
	
	// 배치된 모든 보스 진입 볼륨 활성화
	for (TActorIterator<ANSBossEntryVolume> It(GetWorld()); It; ++It)
	{
		It->Activate();
		break; 
	}
}

void ANSRunGameMode::SyncRunDataConfigToGameState()
{
	UNSGameFlowSubsystem* GameFlow = 
		GetGameInstance() ? GetGameInstance()->GetSubsystem<UNSGameFlowSubsystem>() : nullptr;
	
	ANSRunGameState* RunGameState = GetGameState<ANSRunGameState>();
	
	if (!GameFlow || !RunGameState)
	{
		return;
	}
	
	RunGameState->SetRunDataConfig(GameFlow->GetSelectedRunConfig(), GameFlow->GetSelectedRunLevelConfig());
}

void ANSRunGameMode::PushObjectiveStateToGameState()
{
	ANSRunGameState* RunGS = GetGameState<ANSRunGameState>();
	if (!RunGS || !NSStageManager)
	{
		return;
	}

	FNSStageObjectiveState ObjectiveState;
	ObjectiveState.Type    = NSStageManager->GetObjectiveType();
	ObjectiveState.Current = NSStageManager->GetObjectiveCurrent();
	ObjectiveState.Target  = NSStageManager->GetObjectiveTarget();
	RunGS->SetObjectiveState(ObjectiveState);
}

void ANSRunGameMode::SubmitRunChoice_Implementation(APlayerController* Voter, ENSRunChoice Choice)
{
	if (!HasAuthority() || !Voter)
	{
		return;
	}

	ANSPlayerState* NSPlayerState = Voter->GetPlayerState<ANSPlayerState>();
	if (!NSPlayerState || NSPlayerState->bVoteConfirmed)
	{
		return;
	}

	ANSRunGameState* RunGameState = GetGameState<ANSRunGameState>();
	if (!RunGameState)
	{
		return;
	}

	NSPlayerState->RunChoice = Choice;
	NSPlayerState->bVoteConfirmed = true;

	if (Choice == ENSRunChoice::NextStage)
	{
		RunGameState->NextVotes++;
	}
	else
	{
		RunGameState->HubVotes++;
	}

	RunGameState->NotifyRunVoteChanged();
	RunGameState->ForceNetUpdate();

	// 전원 확인 시 ResolveVote
	HandlePlayerConfirmed();
}
