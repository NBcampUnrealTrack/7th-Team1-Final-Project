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


ANSSpawner::ANSSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ANSSpawner::OnActorEnteredRoom(AActor* OtherActor)
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (!OtherActor || !OtherActor->IsA(ANSPlayerCharacterBase::StaticClass()))
	{
		return;
	}
	
	++PlayersInRoom;
	ActivateSpawner();
}

void ANSSpawner::OnActorExitedRoom(AActor* OtherActor)
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (!OtherActor || !OtherActor->IsA(ANSPlayerCharacterBase::StaticClass())) 
	{		
		return;
	}

	PlayersInRoom = FMath::Max(0, PlayersInRoom - 1);

	// 아직 룸에 플레이어가 남아 있으면 몬스터 유지
	if (PlayersInRoom > 0)
	{
		return;
	}

	// 마지막 플레이어가 나갔을 때만 풀로 반환
	ReturnMonstersToPool();
}

void ANSSpawner::ActivateSpawner()
{
	if (!HasAuthority() || !SpawnDataTable)
	{
		return;
	}
	
	// 이미 스폰된 룸이면 무시
	if (bHasSpawned) 
	{
		return;
	}

	bHasSpawned = true;
	ProcessSpawnProbability(SpawnDataTable);
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
