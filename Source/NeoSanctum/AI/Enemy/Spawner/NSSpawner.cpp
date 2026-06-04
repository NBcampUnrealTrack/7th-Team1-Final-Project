// Copyright 2026 One Team. All rights reserved.


#include "NSSpawner.h"
#include "Engine/AssetManager.h"
#include "NeoSanctum/AI/Enemy/Spawner/NSMonsterSpawnType.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "GameFramework/GameModeBase.h"


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

	ProcessSpawnProbability(SpawnTable);
	RequestAsyncLoad();
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
	for (int32 i = 0; i < FinalSpawnQuantity; ++i)
	{
		FVector SpawnLocation = GetRandomSpawnLocation();
		INSRunGameModeInterface::Execute_RequestSpawnMonster(
			GameMode, CharacterClass, EnemyData, SpawnLocation, GetActorRotation());
	}

	UE_LOG(LogTemp, Log, TEXT("스폰 요청 수량: %d"), FinalSpawnQuantity);
}

FVector ANSSpawner::GetRandomSpawnLocation() const
{
	FVector Origin = GetActorLocation();

	// 반경 내 랜덤 XY
	FVector2D RandomXY = FMath::RandPointInCircle(SpawnRadius);
	FVector TraceStart = Origin + FVector(RandomXY.X, RandomXY.Y, 500.0f);
	FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, 2000.0f);

	// 지면 라인 트레이스 (경사면 대응)
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
	{
		// 캡슐 절반 높이만큼 띄움
		return Hit.Location + FVector(0.0f, 0.0f, 88.0f);
	}

	return Origin;
}

