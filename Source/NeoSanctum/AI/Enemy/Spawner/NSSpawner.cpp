// Copyright 2026 One Team. All rights reserved.


#include "NSSpawner.h"
#include "Engine/AssetManager.h"
#include "NeoSanctum/AI/Enemy/Spawner/NSMonsterSpawnType.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "GameFramework/GameModeBase.h"
#include "NavigationSystem.h"
#include "RoomLevel.h"


ANSSpawner::ANSSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ANSSpawner::ActivateSpawner(UDataTable* SpawnTable)
{
	if (!HasAuthority() || !SpawnTable)
	{
		return;
	}
	
	// 이미 스폰된 룸이면 무시
	if (bHasSpawned) 
	{
		return;
	}

	bHasSpawned = true;

	ProcessSpawnProbability(SpawnTable);
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
	FinalSpawnQuantity = FMath::RandRange(SelectedRow->MinQuantity, SelectedRow->MaxQuantity);
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

FVector ANSSpawner::GetRandomSpawnLocation(const TArray<FVector>& AlreadyPlaced) const
{
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSystem)
	{
		return GetActorLocation();
	}

	FVector Center, Extent;
	GetRoomBounds(Center, Extent);

	// 벽에 붙는 것 방지용
	const float Margin = 100.0f;
	const float HalfX = FMath::Max(Extent.X - Margin, 0.0f);
	const float HalfY = FMath::Max(Extent.Y - Margin, 0.0f);

	const int32 MaxTries = 20;
	FVector LastValid = GetActorLocation();

	for (int32 Try = 0; Try < MaxTries; ++Try)
	{
		// 스포너가 있는 룸 안에서만 샘플링
		const FVector Candidate = Center + FVector(
			FMath::FRandRange(-HalfX, HalfX),
			FMath::FRandRange(-HalfY, HalfY),
			0.0f);

		// 옆 룸으로 안 넘어가게 조절
		FNavLocation NavLoc;
		const FVector QueryExtent(150.0f, 150.0f, Extent.Z + 100.0f);
		if (!NavSystem->ProjectPointToNavigation(Candidate, NavLoc, QueryExtent))
		{
			// 해당되는 자리에 네브메시 없으면 저장안함
			continue; 
		}

		LastValid = NavLoc.Location;

		// 최소 간격 검사
		bool bTooClose = false;
		for (const FVector& Placed : AlreadyPlaced)
		{
			if (FVector::DistSquared2D(Placed, NavLoc.Location) < 
				MinSpawnSpacing * MinSpawnSpacing)
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

	// 실패해도 마지막 유효 네브메시 점을 반환
	return LastValid + FVector(0.0f, 0.0f, 88.0f);
}

// 자기 룸의 바운드 얻기(없으면 스포너 반경 폴백)
bool ANSSpawner::GetRoomBounds(FVector& OutCenter, FVector& OutExtent) const
{
	if (ULevel* Level = GetLevel())
	{
		if (ARoomLevel* RoomLevel = Cast<ARoomLevel>(Level->GetLevelScriptActor()))
		{
			OutCenter = RoomLevel->GetBoundsCenter();
			OutExtent = RoomLevel->GetBoundsExtent();
			return true;
		}
	}
	OutCenter = GetActorLocation();
	OutExtent = FVector(FallbackSpawnRadius, FallbackSpawnRadius, 1000.0f);
	
	return false;
}

