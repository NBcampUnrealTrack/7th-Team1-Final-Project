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
#include "Engine/AssetManager.h"
#include "NeoSanctum/AI/Enemy/Spawner/NSMonsterSpawnType.h"
#include "NeoSanctum/AI/Enemy/Spawner/NSSpawner.h"
#include "NeoSanctum/Character/Enemy/NSEnemyPawnBase.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/Stage/NSBossEntryVolume.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Progression/Augment/NSAugmentInventoryComponent.h"
#include "NeoSanctum/Progression/Augment/NSAugmentSelectionComponent.h"
#include "NeoSanctum/Progression/Heal/NSHealReplicationProxy.h"
#include "NeoSanctum/Progression/Part/NSPartEquipComponent.h"
#include "NeoSanctum/Progression/Reward/NSRewardHandler.h"
#include "NeoSanctum/System/Subsystem/NSHealDropSubsystem.h"
#include "NeoSanctum/Tag/NSGameplayTags_Reward.h"
#include "NeoSanctum/Interaction/NPC/NSRescueNPC.h"
#include "NeoSanctum/Core/Waypoint/NSWaypointMarkerComponent.h"



ANSRunGameMode::ANSRunGameMode()
{
	bUseSeamlessTravel = true;
	
	GameStateClass = ANSRunGameState::StaticClass();
	PlayerControllerClass = ANSPlayerController::StaticClass();
	PlayerStateClass = ANSPlayerState::StaticClass();
	DefaultPawnClass = nullptr;

	ProjectileReplicationProxyClass = ANSProjectileReplicationProxy::StaticClass();
	CurrencyReplicationProxyClass = ANSCurrencyReplicationProxy::StaticClass();
	HealReplicationProxyClass = ANSHealReplicationProxy::StaticClass();
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
		EnsureHealProxy(*It);
	}
}

void ANSRunGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	
	// 인런 진입 후 참여 막는 용도
	ErrorMessage = TEXT("게임이 이미 진행 중이라 참가할 수 없습니다.");
	UE_LOG(LogTemp, Warning, TEXT("[Session] 인런 진행 중 조인 거부: %s"), *Address);
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

bool ANSRunGameMode::AreAllPlayersDeadOrGone() const
{
	ANSRunGameState* NSGameState = GetGameState<ANSRunGameState>();
	if (!NSGameState)
	{
		return false;
	}

	bool bAnyAlive = false;
	for (APlayerState* PS : NSGameState->PlayerArray)
	{
		ANSPlayerState* NSPS = Cast<ANSPlayerState>(PS);
		// 유효하지 않은 PS는 제외
		if (!NSPS)
		{
			continue;
		}               
		
		if (!NSPS->IsDead())
		{
			// 살아있는 사람 발견
			bAnyAlive = true; 
			break;
		}
	}
	
	// 살아있는 사람이 없으면 전멸
	return !bAnyAlive;   
}

void ANSRunGameMode::NotifyPlayerDied_Implementation(AController* DeadPlayer)
{
	if (!HasAuthority())
	{
		return;
	}

	if (AreAllPlayersDeadOrGone())
	{
		OpenRunEndVote(true);
	}
}

void ANSRunGameMode::NotifyEnemyKilled_Implementation(AActor* DeadEnemy, AController* Killer)
{
	if (!HasAuthority() || !DeadEnemy)
	{
		return;
	}
	
	// 가해자 PlayerState에 랭크별 킬 기록
	if (ANSPlayerState* KillerPS =
		Killer ? Killer->GetPlayerState<ANSPlayerState>() : nullptr)
	{
		const UNSEnemyCoreComponent* CoreComponent =
			DeadEnemy->FindComponentByClass<UNSEnemyCoreComponent>();
		if (const UNSEnemyData* EnemyData = 
			CoreComponent ? CoreComponent->GetEnemyData() : nullptr)
		{
			KillerPS->AddKill(EnemyData->EnemyRank);
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[Kill] DeadEnemy=%s Killer=%s"),
	   *GetNameSafe(DeadEnemy), *GetNameSafe(Killer));

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

void ANSRunGameMode::TryStartPrewarm()
{
	 if (!HasAuthority() || !NSMonsterPoolManager)
    {
        return;
    }

    // 스포너 순회해서 Row에서 (EnemyData, CharacterClass) 수집
    TSet<TSoftObjectPtr<UNSEnemyData>> UniqueSet;
    TMap<TSoftObjectPtr<UNSEnemyData>, TSoftClassPtr<APawn>> DataToClass;
    int32 SpawnerCount = 0;

    for (TActorIterator<ANSSpawner> It(GetWorld()); It; ++It)
    {
        ANSSpawner* Spawner = *It;
    	// 보스는 풀링 대상 아님
        if (!Spawner || Spawner->IsBossSpawner())
        {
            continue;
        }
    	
        ++SpawnerCount;

        UDataTable* Table = Spawner->ResolveSpawnDataTable();
    	UE_LOG(LogTemp, Warning, TEXT("[Prewarm] Spawner=%s Table=%s"),
	   *GetNameSafe(Spawner),
	   *GetNameSafe(Table));
        if (!Table)
        {
            continue;
        }

        TArray<FNSMonsterSpawnRow*> Rows;
        Table->GetAllRows<FNSMonsterSpawnRow>(TEXT("Prewarm"), Rows);
        for (const FNSMonsterSpawnRow* Row : Rows)
        {
            if (Row && !Row->EnemyData.IsNull() && !Row->CharacterClass.IsNull())
            {
                UniqueSet.Add(Row->EnemyData);
                DataToClass.Add(Row->EnemyData, Row->CharacterClass);
            }
        }
    }

    // 스포너가 0개로 나온다면 던전 생성이 아직 안되었으므로 짧게 재시도
    if (SpawnerCount == 0 && PrewarmRetryCount < 20)
    {
        ++PrewarmRetryCount;
        FTimerHandle Tmp;
        GetWorldTimerManager().SetTimer(
        	Tmp,
        	this,
        	&ANSRunGameMode::TryStartPrewarm,
        	0.1f, 
        	false);
        return;
    }

    if (UniqueSet.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Prewarm] 열거된 EnemyData 0 — 프리워밍 스킵"));
        return;
    }

    // EnemyData 목록 async 로드
    TArray<TSoftObjectPtr<UNSEnemyData>> UniqueDatas = UniqueSet.Array();
    TArray<FSoftObjectPath> Paths;
    for (const TSoftObjectPtr<UNSEnemyData>& D : UniqueDatas)
    {
        Paths.Add(D.ToSoftObjectPath());
    }
    for (const auto& Pair : DataToClass)
    {
    	// 캐릭터 클래스도 로드
        Paths.Add(Pair.Value.ToSoftObjectPath()); 
    }

    FStreamableManager& Streamable = UAssetManager::Get().GetStreamableManager();
    PrewarmLoadHandle = Streamable.RequestAsyncLoad(
        Paths,
        FStreamableDelegate::CreateUObject(
            this, 
            &ANSRunGameMode::HandlePrewarmDataLoaded, 
            UniqueDatas, 
            DataToClass));
}

void ANSRunGameMode::TickPrewarmStep()
{
	if (!NSMonsterPoolManager || !NSMonsterPoolManager->PrewarmStep())
	{
		// PrewarmStep이 false 반환하거나 매니저가 없으면 타이머 정지
		GetWorldTimerManager().ClearTimer(PrewarmStepTimerHandle);
	}
}

void ANSRunGameMode::StartPrewarmFlow()
{
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Data)
	{
		return;
	}

	if (Data->AreCurrentStageSpawnerTablesLoaded())
	{
		TryStartPrewarm();
	}
	else
	{
		Data->OnStageSpawnerTablesReady.RemoveDynamic(
			this, 
			&ANSRunGameMode::HandleStageSpawnerTablesReadyForPrewarm);
		Data->OnStageSpawnerTablesReady.AddDynamic(
			this, 
			&ANSRunGameMode::HandleStageSpawnerTablesReadyForPrewarm);
		
		Data->LoadCurrentStageSpawnerTables();
	}
}

void ANSRunGameMode::HandleStageSpawnerTablesReadyForPrewarm()
{
	if (UNSDataSubsystem* Data = UNSDataSubsystem::Get(this))
	{
		Data->OnStageSpawnerTablesReady.RemoveDynamic(
			this,
			&ANSRunGameMode::HandleStageSpawnerTablesReadyForPrewarm);
	}
	
	TryStartPrewarm();
}

void ANSRunGameMode::HandlePrewarmDataLoaded(TArray<TSoftObjectPtr<UNSEnemyData>> UniqueDatas,
	TMap<TSoftObjectPtr<UNSEnemyData>, TSoftClassPtr<APawn>> DataToClass)
{
	 if (!NSMonsterPoolManager)
    {
        return;
    }

    // 랭크로 Normal, Elite 분류
    TArray<TSoftObjectPtr<UNSEnemyData>> NormalDatas;
    TArray<TSoftObjectPtr<UNSEnemyData>> EliteDatas;

    for (const TSoftObjectPtr<UNSEnemyData>& D : UniqueDatas)
    {
        UNSEnemyData* Data = D.Get();
        if (!Data)
        {
            continue;
        }
        if (Data->EnemyRank == ENSEnemyRank::Elite)
        {
            EliteDatas.Add(D);
        }
        else if (Data->EnemyRank == ENSEnemyRank::Normal)
        {
            NormalDatas.Add(D);
        }
    }

    // 그룹 상한을 종류 수로 균등 분배
    auto BuildRequests = [&](const TArray<TSoftObjectPtr<UNSEnemyData>>& Datas,
                             int32 Total, TArray<FNSPrewarmRequest>& Out)
    {
        const int32 Num = Datas.Num();
        if (Num == 0 || Total <= 0)
        {
            return;
        }
        const int32 Base = Total / Num;
        int32 Remainder = Total % Num;

        for (const TSoftObjectPtr<UNSEnemyData>& D : Datas)
        {
            FNSPrewarmRequest Req;
            Req.EnemyData = D.Get();
            if (const TSoftClassPtr<APawn>* ClassPtr = DataToClass.Find(D))
            {
                Req.CharacterClass = ClassPtr->Get();
            }
            Req.Count = Base + (Remainder > 0 ? 1 : 0);
            if (Remainder > 0) --Remainder;

            if (Req.CharacterClass && Req.EnemyData && Req.Count > 0)
            {
                Out.Add(Req);
            }
        }
    };

    TArray<FNSPrewarmRequest> Requests;
    BuildRequests(NormalDatas, PrewarmNormalTotal, Requests);
    BuildRequests(EliteDatas, PrewarmEliteTotal, Requests);

    UE_LOG(LogTemp, Log, TEXT("[Prewarm] Normal 종류 %d(총 %d) / Elite 종류 %d(총 %d) 배분"),
        NormalDatas.Num(), PrewarmNormalTotal, EliteDatas.Num(), PrewarmEliteTotal);

    FSimpleDelegate OnDone;
	OnDone.BindUObject(
		this,
		&ANSRunGameMode::HandlePrewarmComplete);
	
	NSMonsterPoolManager->PrewarmBegin(
		Requests,
		PrewarmPerTick,
		OnDone);
	
	GetWorldTimerManager().SetTimer(
	PrewarmStepTimerHandle,
	this, 
	&ANSRunGameMode::TickPrewarmStep,
	0.001f, 
	true);
	
	// 안전장치: 프리워밍이 완료 못 해도 일정 시간 후 게이트 강제 오픈
	GetWorldTimerManager().SetTimer(
		PrewarmTimeoutHandle,
		this, 
		&ANSRunGameMode::ForcePrewarmGateOpen,
		PrewarmTimeoutSeconds,
		false);
	
	UE_LOG(LogTemp, Warning, TEXT("[Prewarm] SetTimer 완료 Handle유효=%d Paused=%d WorldTime=%.2f"),
	PrewarmStepTimerHandle.IsValid() ? 1 : 0,
	GetWorld()->IsPaused() ? 1 : 0,
	GetWorld()->GetTimeSeconds());
}

void ANSRunGameMode::HandlePrewarmComplete()
{
	// 정상 완료 → 타임아웃 취소 + 게이트 오픈
	GetWorldTimerManager().ClearTimer(PrewarmTimeoutHandle);
	UE_LOG(LogTemp, Warning, TEXT("[Prewarm] 완료 → 로딩 게이트 오픈"));
	OpenPrewarmGateForAll();
}

void ANSRunGameMode::ForcePrewarmGateOpen()
{
	// 타임아웃 → 프리워밍 미완이어도 게임 진행 보장
	UE_LOG(LogTemp, Warning, TEXT("[Prewarm] 타임아웃 → 게이트 강제 오픈"));
	OpenPrewarmGateForAll();
}

void ANSRunGameMode::OpenPrewarmGateForAll()
{
	// 호스트 자신
	if (UNSUIManagerSubsystem* UI = UNSUIManagerSubsystem::Get(this))
	{
		UI->MarkTravelPrewarmReady();
	}

	// 원격 클라 전원에 RPC
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ANSPlayerController* PC = Cast<ANSPlayerController>(It->Get()))
		{
			if (!PC->IsLocalController()) 
			{
				PC->Client_NotifyPrewarmReady();
			}
		}
	}
}

void ANSRunGameMode::RequestReturnToHub_Implementation()
{
	BeginReturnToHubTravel();
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

ANSEnemyPawnBase* ANSRunGameMode::RequestSpawnBoss_Implementation(UClass* BossClass, UNSEnemyData* EnemyData, const FVector& Location,
	const FRotator& Rotation)
{
	if (!HasAuthority() || !BossClass || !EnemyData)
	{
		return nullptr;
	}

	const int32 PlayerCount = GameState ? GameState->PlayerArray.Num() : 1;
	FNSDifficultyScale Scale;
	if (UNSGameFlowSubsystem* Flow = GetGameInstance()->GetSubsystem<UNSGameFlowSubsystem>())
	{
		Scale = Flow->GetCurrentMonsterScale(PlayerCount);
	}

	// 보스는 풀링하지 않고 직접 스폰
	const FTransform SpawnTransform(Rotation, Location);
	ANSEnemyPawnBase* Boss = GetWorld()->SpawnActorDeferred<ANSEnemyPawnBase>(
		BossClass, SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!Boss)
	{
		NS_ACTOR_LOG(this, LogNS, Warning,
			"보스 스폰 실패: BossClass가 ANSEnemyPawnBase 하위가 아닙니다. Class={Class}",
			("Class", GetNameSafe(BossClass)));
		return nullptr;
	}

	Boss->SetEnemyData(EnemyData);
	Boss->SetDifficultyScale(Scale);
	Boss->FinishSpawning(SpawnTransform);

	return Boss;
}

void ANSRunGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	EnsureProjectileProxy(NewPlayer);
	EnsureCurrencyProxy(NewPlayer);
	EnsureHealProxy(NewPlayer);
}

void ANSRunGameMode::Logout(AController* Exiting)
{
	DestroyProjectileProxy(Cast<APlayerController>(Exiting));
	DestroyCurrencyProxy(Cast<APlayerController>(Exiting));
	DestroyHealProxy(Cast<APlayerController>(Exiting));

	Super::Logout(Exiting);
	
	// 플레이어 탈주 후 남은 플레이어가 전원 사망인지 재판정
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(
				this,
				&ANSRunGameMode::CheckRunEndAfterLogout));
	}
}

void ANSRunGameMode::CheckRunEndAfterLogout()
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

	const int32 PlayerCount = NSGameState->PlayerArray.Num();
	const bool bAllDead = AreAllPlayersDeadOrGone();
	UE_LOG(LogTemp, Warning, TEXT("[RunEnd] NextTick 판정 PlayerArray=%d AllDead=%d Phase=%d"),
		PlayerCount, bAllDead ? 1 : 0, (int32)NSGameState->RunEndPhase);

	if (NSGameState->RunEndPhase == ENSRunEndPhase::None && bAllDead)
	{
		OpenRunEndVote(true);
	}
	else if (NSGameState->RunEndPhase == ENSRunEndPhase::Voting)
	{
		HandlePlayerConfirmed();
	}
}

void ANSRunGameMode::HandleSeamlessTravelPlayer(AController*& Controller)
{
	Super::HandleSeamlessTravelPlayer(Controller);

	EnsureProjectileProxy(Cast<APlayerController>(Controller));
	EnsureCurrencyProxy(Cast<APlayerController>(Controller));
	EnsureHealProxy(Cast<APlayerController>(Controller));
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

void ANSRunGameMode::EnsureHealProxy(APlayerController* PlayerController)
{
	if (!HasAuthority() || !IsValid(PlayerController) || !HealReplicationProxyClass)
	{
		return;
	}
	
	if (ANSHealReplicationProxy* ExistingProxy = HealProxies.FindRef(PlayerController))
	{
		if (IsValid(ExistingProxy))
		{
			return;
		}
	}
	
	UWorld* World = GetWorld();
	UNSHealDropSubsystem* DropSys = World ? World->GetSubsystem<UNSHealDropSubsystem>() : nullptr;
	if (!DropSys)
	{
		return;
	}
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = PlayerController;
	SpawnParameters.Instigator = PlayerController->GetPawn();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	ANSHealReplicationProxy* Proxy = World->SpawnActor<ANSHealReplicationProxy>(
		HealReplicationProxyClass,
		FTransform::Identity,
		SpawnParameters);
	
	if (!IsValid(Proxy))
	{
		return;
	}
	
	HealProxies.Add(PlayerController, Proxy);
	DropSys->RegisterProxy(Proxy);
}

void ANSRunGameMode::DestroyHealProxy(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	ANSHealReplicationProxy* Proxy = HealProxies.FindRef(PlayerController);

	if (IsValid(Proxy))
	{
		if (UWorld* World = GetWorld())
		{
			if (UNSHealDropSubsystem* DropSys = World->GetSubsystem<UNSHealDropSubsystem>())
			{
				DropSys->UnregisterProxy(Proxy);
			}
		}

		Proxy->Destroy();
	}

	HealProxies.Remove(PlayerController);
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
		StartDifficultyTimerForReadyStage();
		StartPrewarmFlow();
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
		Data->OnRunGameDataReady.RemoveDynamic(
			this,
			&ANSRunGameMode::HandleRunDataReadyForPrewarm);
		Data->OnRunGameDataReady.AddDynamic(
			this, 
			&ANSRunGameMode::HandleRunDataReadyForPrewarm);
	}
}

void ANSRunGameMode::HandleRunDataReadyForPrewarm()
{
	if (UNSDataSubsystem* Data = UNSDataSubsystem::Get(this))
	{
		Data->OnRunGameDataReady.RemoveDynamic(this, &ANSRunGameMode::HandleRunDataReadyForPrewarm);
	}
	StartPrewarmFlow();
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

	// 구출 목표면 대상 NPC 마커 활성화 (전 클라 공통, 리플리케이션)
	ActivateRescueMarkersIfNeeded();
}

bool ANSRunGameMode::ShouldShowRescueMarker(FName InNPCId) const
{
	// 목표가 아직 없거나
	if (!NSStageManager || !NSStageManager->IsObjectiveInitialized())
	{
		return false;
	}

	// 목표가 NPC구출이 아니면 마커 대상 아님
	if (NSStageManager->GetObjectiveType() != ENSStageObjectiveType::RescueNPC)
	{
		return false;
	}

	// 지정 대상이 있으면 일치할 때만 마커 대상
	const FName TargetNPCId = NSStageManager->GetTargetNPCId();
	return TargetNPCId.IsNone() || TargetNPCId == InNPCId;
}

void ANSRunGameMode::ActivateRescueMarkersIfNeeded()
{
	// 이미 스폰돼 있는 구출 NPC들의 마커 켜기 -> 목표 초기화보다 늦게 스폰되는 NPC는 RescueNPC::BeginPlay에서 한번 더 확인
	for (TActorIterator<ANSRescueNPC> It(GetWorld()); It; ++It)
	{
		if (!ShouldShowRescueMarker(It->GetNPCId()))
		{
			continue;
		}

		if (UNSWaypointMarkerComponent* Marker =
			It->FindComponentByClass<UNSWaypointMarkerComponent>())
		{
			Marker->SetMarkerActive(true);
		}
	}
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
	StartDifficultyTimerForReadyStage();
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

void ANSRunGameMode::StartDifficultyTimerForReadyStage()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UNSGameFlowSubsystem* Flow =
		GetGameInstance()->GetSubsystem<UNSGameFlowSubsystem>())
	{
		if (Flow->GetCurrentStageNumber() <= 1)
		{
			Flow->RestartDifficultyTimer();
		}
		else
		{
			Flow->SetDifficultyTimerWaitingForReady(false);
			Flow->ResumeDifficultyTimer();
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
		ANSPlayerState* NSPlayerState =
			Cast<ANSPlayerState>(PlayerState);

		if (!NSPlayerState)
		{
			++Hub;
			continue;
		}

		// 제한시간까지 투표하지 않은 플레이어는 거점 복귀로 처리한다.
		if (!NSPlayerState->bVoteConfirmed)
		{
			NSPlayerState->RunChoice =
				ENSRunChoice::ReturnToHub;

			NSPlayerState->bVoteConfirmed = true;
			NSPlayerState->ForceNetUpdate();
		}

		if (NSPlayerState->RunChoice == ENSRunChoice::NextStage)
		{
			++Next;
		}
		else
		{
			++Hub;
		}
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

	if (ResultDisplayDuration <= 0.0f)
	{
		GetWorldTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(
				this,
				&ANSRunGameMode::OnResultDisplayFinished));

		return;
	}

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
			BeginReturnToHubTravel();
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
	ObjectiveState.Description = NSStageManager->GetObjectiveDescription();
	RunGS->SetObjectiveState(ObjectiveState);
}

void ANSRunGameMode::SubmitRunChoice_Implementation(APlayerController* Voter, ENSRunChoice Choice)
{
	if (!HasAuthority() || !Voter)
	{
		return;
	}

	ANSRunGameState* RunGameState =
		GetGameState<ANSRunGameState>();

	ANSPlayerState* NSPlayerState =
		Voter->GetPlayerState<ANSPlayerState>();

	if (!RunGameState || !NSPlayerState)
	{
		return;
	}

	// 제한 시간이 끝난 뒤 들어온 투표 요청은 처리하지 않는다.
	if (RunGameState->RunEndPhase != ENSRunEndPhase::Voting)
	{
		return;
	}

	// 실패한 런에서는 거점 복귀만 선택할 수 있다.
	if (!RunGameState->bIsClear &&
		Choice == ENSRunChoice::NextStage)
	{
		return;
	}

	if (NSPlayerState->bVoteConfirmed)
	{
		// 이미 같은 선택지에 투표한 경우 중복 처리하지 않는다.
		if (NSPlayerState->RunChoice == Choice)
		{
			return;
		}

		// 기존 선택지의 투표 수를 먼저 차감한다.
		if (NSPlayerState->RunChoice == ENSRunChoice::NextStage)
		{
			RunGameState->NextVotes =
				FMath::Max(RunGameState->NextVotes - 1, 0);
		}
		else
		{
			RunGameState->HubVotes =
				FMath::Max(RunGameState->HubVotes - 1, 0);
		}
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

	NSPlayerState->ForceNetUpdate();
	RunGameState->NotifyRunVoteChanged();
	RunGameState->ForceNetUpdate();

	// 제한 시간 동안 재투표할 수 있어야 하므로
	// HandlePlayerConfirmed()는 호출하지 않는다.
}

void ANSRunGameMode::BeginReturnToHubTravel()
{
	if (!HasAuthority())
	{
		return;
	}

	if (bReturnToHubTravelStarted)
	{
		return;
	}
	
	bReturnToHubTravelStarted = true;

	// 서버 인런 데이터 언로드 및 아웃게임 데이터 재로드
	if (UNSDataSubsystem* Data = UNSDataSubsystem::Get(this))
	{
		Data->ReturnToOutGame();
	}

	// 각 클라이언트에 인런 데이터 언로드 및 아웃게임 데이터 준비 지시
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ANSPlayerController* PC = Cast<ANSPlayerController>(It->Get()))
		{
			PC->Client_NotifyReturnToHub();
		}
	}

	// 실제 거점 맵으로 ServerTravel
	if (UNSGameFlowSubsystem* NSGameFlow =
		GetGameInstance()->GetSubsystem<UNSGameFlowSubsystem>())
	{
		NSGameFlow->ReturnToHub();
	}
}
