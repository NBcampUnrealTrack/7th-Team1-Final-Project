// Copyright 2026 One Team. All rights reserved.


#include "NSDataSubsystem.h"

#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"
// #include "NeoSanctum/Data/Augment/NSAugmentDefinition.h"

// Project Settings > Asset Manager 등록 이름과 반드시 일치
const FPrimaryAssetType UNSDataSubsystem::PlayerAssetType        = FPrimaryAssetType(TEXT("NSPlayerData"));
const FPrimaryAssetType UNSDataSubsystem::HubAssetType           = FPrimaryAssetType(TEXT("NSHubData"));
const FPrimaryAssetType UNSDataSubsystem::PartAssetType         = FPrimaryAssetType(TEXT("NSPartData"));
const FPrimaryAssetType UNSDataSubsystem::MonsterAssetType      = FPrimaryAssetType(TEXT("NSMonsterData"));
const FPrimaryAssetType UNSDataSubsystem::AugmentAssetType      = FPrimaryAssetType(TEXT("NSAugmentData"));
const FPrimaryAssetType UNSDataSubsystem::AugmentPoolAssetType  = FPrimaryAssetType(TEXT("NSAugmentPool"));
// TODO: 레벨 전용 GA가 있다면 아웃런, 인런 구분해서 여기서 추가해서 사용하게끔 

void UNSDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// 타이틀에서는 데이터 로드를 시작하지 않음.
	// 외부에서 LoadOutGameData/EnterRun/ReturnToOutGame 으로 명시적으로 진입.
}

void UNSDataSubsystem::Deinitialize()
{
	UnloadAll();
	Super::Deinitialize();
}

UNSDataSubsystem* UNSDataSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (!GI)
	{
		return nullptr;
	}
	return GI->GetSubsystem<UNSDataSubsystem>();
}

// ================================================================
// 전환 진입점
// ================================================================

void UNSDataSubsystem::LoadCommonData()
{
	if (CurrentPhase != ENSDataLoadPhase::NotStarted)
	{
		return;
	}
	StartLoadCommon();
}

void UNSDataSubsystem::LoadOutGameData()
{
	// 이미 OutGame 로드 중/완료면 무시
	if (CurrentPhase == ENSDataLoadPhase::LoadingOutGame || CurrentPhase == ENSDataLoadPhase::OutGameReady)
	{
		return;
	}
	StartLoadOutGame();
}

void UNSDataSubsystem::EnterRun()
{
	if (CurrentPhase == ENSDataLoadPhase::LoadingRun || CurrentPhase == ENSDataLoadPhase::RunReady)
	{
		return;
	}
	UnloadOutGame();
	StartLoadRun();
}

void UNSDataSubsystem::ReturnToOutGame()
{
	if (CurrentPhase == ENSDataLoadPhase::LoadingOutGame || CurrentPhase == ENSDataLoadPhase::OutGameReady)
	{
		return;
	}
	UnloadRun();
	StartLoadOutGame();
}

// ================================================================
// Common 로드
// ================================================================

void UNSDataSubsystem::StartLoadCommon()
{
	SetPhase(ENSDataLoadPhase::LoadingCommon);

	const TArray<FPrimaryAssetType> Types = { PlayerAssetType };

	TArray<FSoftObjectPath> Paths;
	CollectSoftPaths(Types, Paths);

	UE_LOG(LogTemp, Log, TEXT("[NSDataSubsystem] Common paths total=%d"), Paths.Num());

	if (Paths.IsEmpty())
	{
		OnCommonAssetsLoaded();
		return;
	}

	CommonHandle = UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateUObject(this, &UNSDataSubsystem::OnCommonAssetsLoaded));
}

void UNSDataSubsystem::OnCommonAssetsLoaded()
{
	CacheLoaded({ PlayerAssetType });
	SetPhase(ENSDataLoadPhase::CommonReady);
	OnCommonDataReady.Broadcast();
}

// ================================================================
// OutGame 로드
// ================================================================

void UNSDataSubsystem::StartLoadOutGame()
{
	SetPhase(ENSDataLoadPhase::LoadingOutGame);

	const TArray<FPrimaryAssetType> Types = { HubAssetType, PartAssetType };

	TArray<FSoftObjectPath> Paths;
	CollectSoftPaths(Types, Paths);

	if (Paths.IsEmpty())
	{
		OnOutGameAssetsLoaded();
		return;
	}

	OutGameHandle = UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateUObject(this, &UNSDataSubsystem::OnOutGameAssetsLoaded));
}

void UNSDataSubsystem::OnOutGameAssetsLoaded()
{
	CacheLoaded({ HubAssetType, PartAssetType });
	SetPhase(ENSDataLoadPhase::OutGameReady);
	OnOutGameDataReady.Broadcast();
}

// ================================================================
// Run 로드
// ================================================================

void UNSDataSubsystem::StartLoadRun()
{
	SetPhase(ENSDataLoadPhase::LoadingRun);

	const TArray<FPrimaryAssetType> Types =
		{ MonsterAssetType, AugmentAssetType, AugmentPoolAssetType, PartAssetType };

	TArray<FSoftObjectPath> Paths;
	CollectSoftPaths(Types, Paths);

	if (Paths.IsEmpty())
	{
		OnRunAssetsLoaded();
		return;
	}

	RunHandle = UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateUObject(this, &UNSDataSubsystem::OnRunAssetsLoaded));
}

void UNSDataSubsystem::OnRunAssetsLoaded()
{
	CacheLoaded({ MonsterAssetType, AugmentAssetType, AugmentPoolAssetType, PartAssetType });
	// StartLoadRunDependents();
	OnRunDependentsLoaded();
}

// Augment 정의가 참조하는 GE/GA 클래스 사전 로드
// void UNSDataSubsystem::StartLoadRunDependents()
// {
// 	TArray<FSoftObjectPath> DepPaths;
// 	for (const TPair<FPrimaryAssetId, TObjectPtr<UObject>>& Pair : DataCache)
// 	{
// 		if (UNSAugmentDefinition* Aug = Cast<UNSAugmentDefinition>(Pair.Value.Get()))
// 		{
// 			if (!Aug->StackEffectClass.IsNull())
// 			{
// 				DepPaths.Add(Aug->StackEffectClass.ToSoftObjectPath());
// 			}
// 			if (!Aug->GrantedAbilityClass.IsNull())
// 			{
// 				DepPaths.Add(Aug->GrantedAbilityClass.ToSoftObjectPath());
// 			}
// 		}
// 	}
//
// 	if (DepPaths.IsEmpty())
// 	{
// 		OnRunDependentsLoaded();
// 		return;
// 	}
//
// 	RunDependentsHandle = UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
// 		DepPaths,
// 		FStreamableDelegate::CreateUObject(this, &UNSDataSubsystem::OnRunDependentsLoaded));
// }

void UNSDataSubsystem::OnRunDependentsLoaded()
{
	SetPhase(ENSDataLoadPhase::RunReady);
	OnRunGameDataReady.Broadcast();
}

// ================================================================
// 언로드
// ================================================================

void UNSDataSubsystem::UnloadCommon()
{
	if (CommonHandle.IsValid())
	{
		CommonHandle->ReleaseHandle();
		CommonHandle.Reset();
	}
	UnloadByTypes({ PlayerAssetType });
}

void UNSDataSubsystem::UnloadOutGame()
{
	if (OutGameHandle.IsValid())
	{
		OutGameHandle->ReleaseHandle();
		OutGameHandle.Reset();
	}
	UnloadByTypes({ HubAssetType, PartAssetType });
}

void UNSDataSubsystem::UnloadRun()
{
	if (RunDependentsHandle.IsValid())
	{
		RunDependentsHandle->ReleaseHandle();
		RunDependentsHandle.Reset();
	}
	if (RunHandle.IsValid())
	{
		RunHandle->ReleaseHandle();
		RunHandle.Reset();
	}
	UnloadByTypes({ MonsterAssetType, AugmentAssetType, AugmentPoolAssetType, PartAssetType });
}

void UNSDataSubsystem::UnloadAll()
{
	UnloadOutGame();
	UnloadRun();
	UnloadCommon();
	DataCache.Empty();
	SetPhase(ENSDataLoadPhase::NotStarted);
}

// ================================================================
// 헬퍼
// ================================================================

void UNSDataSubsystem::CollectSoftPaths(const TArray<FPrimaryAssetType>& Types, TArray<FSoftObjectPath>& OutPaths) const
{
	UAssetManager& AM = UAssetManager::Get();
	for (const FPrimaryAssetType& Type : Types)
	{
		TArray<FAssetData> Assets;
		AM.GetPrimaryAssetDataList(Type, Assets);
		for (const FAssetData& AD : Assets)
		{
			OutPaths.Add(AD.GetSoftObjectPath());
		}
	}
}

void UNSDataSubsystem::CacheLoaded(const TArray<FPrimaryAssetType>& Types)
{
	UAssetManager& AM = UAssetManager::Get();
	for (const FPrimaryAssetType& Type : Types)
	{
		TArray<FPrimaryAssetId> Ids;
		AM.GetPrimaryAssetIdList(Type, Ids);
		for (const FPrimaryAssetId& Id : Ids)
		{
			if (UObject* Obj = AM.GetPrimaryAssetObject(Id))
			{
				DataCache.Add(Id, Obj);
			}
		}
	}
}

void UNSDataSubsystem::UnloadByTypes(const TArray<FPrimaryAssetType>& Types)
{
	UAssetManager& AM = UAssetManager::Get();

	TArray<FPrimaryAssetId> AllIds;
	for (const FPrimaryAssetType& Type : Types)
	{
		TArray<FPrimaryAssetId> Ids;
		AM.GetPrimaryAssetIdList(Type, Ids);
		for (const FPrimaryAssetId& Id : Ids)
		{
			DataCache.Remove(Id);
			AllIds.Add(Id);
		}
	}

	if (!AllIds.IsEmpty())
	{
		AM.UnloadPrimaryAssets(AllIds);
	}
}

void UNSDataSubsystem::SetPhase(ENSDataLoadPhase NewPhase)
{
	CurrentPhase = NewPhase;
	OnPhaseChanged.Broadcast(NewPhase);
}
