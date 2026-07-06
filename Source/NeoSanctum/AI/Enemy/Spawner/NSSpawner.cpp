// Copyright 2026 One Team. All rights reserved.


#include "NSSpawner.h"
#include "Engine/AssetManager.h"
#include "NeoSanctum/AI/Enemy/Spawner/NSMonsterSpawnType.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "GameFramework/GameModeBase.h"
#include "NavigationSystem.h"
#include "RoomLevel.h"
#include "Room.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/GameStateBase.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"


ANSSpawner::ANSSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// 런타임에도 존재하는 실제 루트 (Standalone에서 트랜스폼 보존)
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SpawnRadiusVisualizer = CreateDefaultSubobject<USphereComponent>(TEXT("SpawnRadiusVisualizer"));
	// 루트 밑에 부착
	SpawnRadiusVisualizer->SetupAttachment(SceneRoot);
	SpawnRadiusVisualizer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnRadiusVisualizer->SetCollisionProfileName(TEXT("NoCollision"));
	// 쿠킹/런타임 제외
	SpawnRadiusVisualizer->bIsEditorOnly = true;
	// PIE,게임에서 숨김
	SpawnRadiusVisualizer->SetHiddenInGame(true);
	SpawnRadiusVisualizer->SetSphereRadius(SpawnRadius);
	// editor-only + hidden-in-game이라 레벨 편집 뷰포트에서만 표시됨
	SpawnRadiusVisualizer->SetVisibility(true);
	// 에디터 와이어프레임 색도 디버그 스피어 색과 맞춤
	SpawnRadiusVisualizer->ShapeColor = DebugSphereColor;
}

void ANSSpawner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터에서 SpawnRadius / DebugSphereColor 변경 시 시각화에 반영
	if (SpawnRadiusVisualizer)
	{
		SpawnRadiusVisualizer->SetSphereRadius(SpawnRadius);
		SpawnRadiusVisualizer->ShapeColor = DebugSphereColor;
		SpawnRadiusVisualizer->MarkRenderStateDirty();
	}
}

URoom* ANSSpawner::GetOwningRoom()
{
	if (CachedRoom) 
	{		
		return CachedRoom;
	}
	
	if (ULevel* Level = GetLevel())
	{
		if (ARoomLevel* RoomLevel = Cast<ARoomLevel>(Level->GetLevelScriptActor()))
		{
			CachedRoom = RoomLevel->GetRoom();
		}
	}
	
	return CachedRoom;
}

void ANSSpawner::EvaluateRelevancy(int32 MinRelevancy)
{
	if (!HasAuthority()) 
	{
		return;
	}
	
	if (MinRelevancy >= 0 && MinRelevancy <= SpawnRelevancyThreshold)
	{
		// 플레이어가 정해둔 거리 안으로 들어오면 스폰
		ActivateSpawner();
	}
	else if (MinRelevancy < 0)
	{
		// 모든 플레이어가 거리 밖으로 나가게되면 풀 반환
		ReturnMonstersToPool();
	}
	// MinRelevancy = 0 이면 현상유지
}

void ANSSpawner::ActivateSpawner()
{
	if (!HasAuthority())
	{
		return;
	}
	
	// 이미 스폰된 룸이면 무시
	if (bHasSpawned) 
	{
		return;
	}
	
	// 보스 스포너는 BossFight 페이즈에서만 활성화
	if (bIsBossSpawner)
	{
		const ANSRunGameState* RunGameState =
			GetWorld() ? GetWorld()->GetGameState<ANSRunGameState>() : nullptr;
		if (!RunGameState || RunGameState->StagePhase != ENSStagePhase::BossFight)
		{
			return;
		}
	}

	
	// 인원수 게이트 — 접속 인원이 임계값 미만이면 이 스포너는 켜지 않음
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const int32 PlayerCount = GameState ? GameState->PlayerArray.Num() : 1;
	if (PlayerCount < RequiredPlayerCount)
	{
		return;
	}

	UDataTable* ResolvedSpawnTable = ResolveSpawnDataTable();
	if (!ResolvedSpawnTable)
	{
		RequestStageSpawnerTableLoad();
		return;
	}
	
	bHasSpawned = true;
	ProcessSpawnProbability(ResolvedSpawnTable);
	RequestAsyncLoad();
}

void ANSSpawner::ReturnMonstersToPool()
{
	if (!HasAuthority())
	{
		return;
	}

	// 들어갔다 곧바로 나간 경우면 취소
	if (StreamingHandle.IsValid())
	{
		StreamingHandle->CancelHandle();
		StreamingHandle.Reset();
	}

	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (GameMode && GameMode->Implements<UNSRunGameModeInterface>())
	{
		for (ANSEnemyCharacterBase* Monster : SpawnedMonsters)
		{
			// 살아남은 몬스터만 반환
			if (IsValid(Monster))
			{
				INSRunGameModeInterface::Execute_ReturnMonsterToPool(GameMode, Monster);
			}
		}
	}

	SpawnedMonsters.Empty();
	bHasSpawned = false;
}

UDataTable* ANSSpawner::ResolveSpawnDataTable() const
{
	if (SpawnDataTableSource == ENSSpawnDataTableSource::Manual)
	{
		return SpawnDataTable;
	}
	
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem)
	{
		return nullptr;
	}
	
	if (SpawnDataTableSource == ENSSpawnDataTableSource::StageMelee)
	{
		return DataSubsystem->GetCurrentMeleeSpawnerTable();
	}
	
	if (SpawnDataTableSource == ENSSpawnDataTableSource::StageRange)
	{
		return DataSubsystem->GetCurrentRangeSpawnerTable();
	}
	
	return nullptr;
}

void ANSSpawner::RequestStageSpawnerTableLoad()
{
	if (SpawnDataTableSource == ENSSpawnDataTableSource::Manual)
	{
		return;
	}
	
	UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem)
	{
		return;
	}
	
	if (DataSubsystem->AreCurrentStageSpawnerTablesLoaded())
	{
		const FString SourceName = StaticEnum<ENSSpawnDataTableSource>()->GetNameStringByValue(
			static_cast<int64>(SpawnDataTableSource));
		NS_OBJ_LOG(LogNS, Warning,
			"스테이지 스포너 DT 로드는 완료됐지만 이 스포너가 사용할 테이블을 찾지 못했습니다. Source={Source}",
			("Source", SourceName)
		);
		return;
	}
	
	// 여러 스포너가 동시에 요청해도 DataSubsystem은 한 번만 로드.
	// 완료 후 각 스포너가 다시 ActivateSpawner()를 시도.
	DataSubsystem->OnStageSpawnerTablesReady.RemoveAll(this);
	DataSubsystem->OnStageSpawnerTablesReady.AddDynamic(this, &ANSSpawner::HandleStageSpawnerTablesReady);
	DataSubsystem->LoadCurrentStageSpawnerTables();
}

void ANSSpawner::HandleStageSpawnerTablesReady()
{
	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->OnStageSpawnerTablesReady.RemoveAll(this);
		
		ActivateSpawner();
	}
}

void ANSSpawner::ProcessSpawnProbability(UDataTable* SpawnTable)
{
	TArray<FNSMonsterSpawnRow*> Rows;
	SpawnTable->GetAllRows<FNSMonsterSpawnRow>(TEXT("Spawner"), Rows);
	if (Rows.IsEmpty())
	{
		return;
	}

	float TotalWeight = 0.0f;
	for (const FNSMonsterSpawnRow* Row : Rows)
	{
		if (Row)
		{
			TotalWeight += Row->SpawnWeight;
		}
	}

	float RandomRoll = FMath::FRandRange(0.0f, TotalWeight);
	float CurrentWeightSum = 0.0f;
	FNSMonsterSpawnRow* SelectedRow = Rows[0];

	for (FNSMonsterSpawnRow* Row : Rows)
	{
		if (!Row)
		{
			continue;
		}
		CurrentWeightSum += Row->SpawnWeight;
		if (RandomRoll <= CurrentWeightSum)
		{
			SelectedRow = Row;
			break;
		}
	}

	SoftCharacterClass = SelectedRow->CharacterClass;
	SoftEnemyData = SelectedRow->EnemyData;
	// bool 변수 값에 따라 스폰 수량 분기
	if (bUseFixedSpawnQuantity)
	{
		FinalSpawnQuantity = FixedSpawnQuantity;
	}
	else
	{
		FinalSpawnQuantity = FMath::RandRange(SelectedRow->MinQuantity, SelectedRow->MaxQuantity);
	}
}

void ANSSpawner::RequestAsyncLoad()
{
	if (!HasAuthority() || SoftCharacterClass.IsNull() || SoftEnemyData.IsNull())
	{
		return;
	}

	TArray<FSoftObjectPath> AssetsToLoad;
	AssetsToLoad.Add(SoftCharacterClass.ToSoftObjectPath());
	AssetsToLoad.Add(SoftEnemyData.ToSoftObjectPath());

	FStreamableManager& StreamableManager = UAssetManager::Get().GetStreamableManager();
	StreamingHandle = StreamableManager.RequestAsyncLoad(
		AssetsToLoad,
		FStreamableDelegate::CreateUObject(this, &ANSSpawner::OnLoadCompleted));
}

void ANSSpawner::OnLoadCompleted()
{
	if (!HasAuthority())
	{
		return;
	}

	ExecuteFinalSpawn();
}

void ANSSpawner::ExecuteFinalSpawn()
{
	if (!HasAuthority())
	{
		return;
	}

	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (!GameMode || !GameMode->Implements<UNSRunGameModeInterface>())
	{
		return;
	}

	UClass* CharacterClass = SoftCharacterClass.Get();
	UNSEnemyData* EnemyData = SoftEnemyData.Get();

	if (!CharacterClass || !EnemyData)
	{
		CharacterClass = FallbackCharacterClass;
		EnemyData = FallbackEnemyData;
		UE_LOG(LogTemp, Warning, TEXT("로드 실패 — 폴백 스폰"));
	}

	if (!CharacterClass || !EnemyData)
	{
		return;
	}

	// 스폰 요청(카운트는 게임모드가 스폰 성공했을 때 진행)
	TArray<FVector> PlacedLocations;
	PlacedLocations.Reserve(FinalSpawnQuantity);

	for (int32 i = 0; i < FinalSpawnQuantity; ++i)
	{
		FVector SpawnLocation = GetRandomSpawnLocation(PlacedLocations);
		PlacedLocations.Add(SpawnLocation);

		ANSEnemyCharacterBase* Spawned = INSRunGameModeInterface::Execute_RequestSpawnMonster(
			GameMode,
			CharacterClass,
			EnemyData,
			SpawnLocation,
			GetActorRotation());

		if (Spawned)
		{
			SpawnedMonsters.Add(Spawned);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("스폰 요청 수량: %d"), FinalSpawnQuantity);
}

void ANSSpawner::BeginPlay()
{
	Super::BeginPlay();
	
#if ENABLE_DRAW_DEBUG
	if (bShowRuntimeSpawnRadius)
	{
		// 스포너는 고정이므로 BeginPlay에서 persistent 1회만 그리면 충분(틱 비용 0)
		DrawDebugSphere(GetWorld(), GetActorLocation(), SpawnRadius,
			24, DebugSphereColor, /*bPersistent*/ true, -1.f, 0, 2.f);
	}
#endif

}

void ANSSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->OnStageSpawnerTablesReady.RemoveAll(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

FVector ANSSpawner::GetRandomSpawnLocation(const TArray<FVector>& AlreadyPlaced) const
{
	const FVector Origin = GetActorLocation();
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());

	const int32 MaxTries = 10;
	const float MinSpacingSq = MinSpawnSpacing * MinSpawnSpacing;

	for (int32 Try = 0; Try < MaxTries; ++Try)
	{
		// 스포너 주변 작은 반경
		const FVector2D Off = FMath::RandPointInCircle(SpawnRadius);
		const FVector Candidate = Origin + FVector(Off.X, Off.Y, 0.0f);

		// 바닥 유효성(네브메시 투영)
		FNavLocation NavLoc;
		if (NavSys && NavSys->ProjectPointToNavigation(Candidate, NavLoc, FVector(100, 100, 200)))
		{
			// 캡슐 겹침만 피하는 가벼운 간격 검사
			bool bTooClose = false;
			for (const FVector& P : AlreadyPlaced)
			{
				if (FVector::DistSquared2D(P, NavLoc.Location) < MinSpacingSq)
				{
					bTooClose = true;
					break;
				}
			}
			if (!bTooClose)
			{
				return NavLoc.Location + FVector(0.0f, 0.0f, 88.0f);
			}
		}
	}
	// 실패 시 스포너 위치
	return Origin + FVector(0.0f, 0.0f, 88.0f);
}
