// Copyright 2026 One Team. All rights reserved.


#include "NSDataSubsystem.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"
#include "NeoSanctum/Data/Reward/NSRewardDataRegistry.h"

// Project Settings > Asset Manager 등록 이름과 반드시 일치
const FPrimaryAssetType UNSDataSubsystem::PlayerAssetType			= FPrimaryAssetType(TEXT("NSPlayerData"));
const FPrimaryAssetType UNSDataSubsystem::HubAssetType				= FPrimaryAssetType(TEXT("NSHubData"));
const FPrimaryAssetType UNSDataSubsystem::PartAssetType				= FPrimaryAssetType(TEXT("NSPartData"));
const FPrimaryAssetType UNSDataSubsystem::MonsterAssetType			= FPrimaryAssetType(TEXT("NSMonsterData"));
const FPrimaryAssetType UNSDataSubsystem::AugmentAssetType			= FPrimaryAssetType(TEXT("NSAugmentData"));
const FPrimaryAssetType UNSDataSubsystem::AugmentPoolAssetType		= FPrimaryAssetType(TEXT("NSAugmentPool"));
const FPrimaryAssetType UNSDataSubsystem::RewardTriggerAssetType	= FPrimaryAssetType(TEXT("NSRewardTriggerData"));
// TODO: 레벨 전용 GA가 있다면 아웃런, 인런 구분해서 여기서 추가해서 사용하게끔

// DataAsset의 meta=(AssetBundles="...") 와 반드시 일치
const TArray<FName> UNSDataSubsystem::CommonBundles  = { FName("CommonUI"),  FName("CommonData")  };
const TArray<FName> UNSDataSubsystem::OutGameBundles = { FName("OutRunUI"),  FName("OutRunData")  };
const TArray<FName> UNSDataSubsystem::RunBundles     = { FName("InRunUI"),   FName("InRunData")   };

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

void UNSDataSubsystem::SetCachedProgressPayload(const FNSProgressPayload& Payload)
{
	CachedProgressPayload = Payload;
	bHasCachedProgressPayload = true;
}

bool UNSDataSubsystem::GetCachedProgressPayload(FNSProgressPayload& OutPayload) const
{
	if (!bHasCachedProgressPayload)
	{
		return false;
	}

	OutPayload = CachedProgressPayload;
	return true;
}

void UNSDataSubsystem::ApplyCachedProgressTo(class UNSPlayerProgressComponent* ProgressComponent) const
{
	if (!ProgressComponent || !bHasCachedProgressPayload)
	{
		return;
	}

	ProgressComponent->ApplyPayload(CachedProgressPayload);
}

// ================================================================
// Common 로드
// ================================================================

void UNSDataSubsystem::StartLoadCommon()
{
	SetPhase(ENSDataLoadPhase::LoadingCommon);

	const TArray<FPrimaryAssetType> Types = { PlayerAssetType };

	TArray<FPrimaryAssetId> Ids;
	GatherAssetIds(Types, Ids);

	if (Ids.IsEmpty())
	{
		OnCommonAssetsLoaded();
		return;
	}

	CommonHandle = UAssetManager::Get().LoadPrimaryAssets(
		Ids,
		CommonBundles,
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

	TArray<FPrimaryAssetId> Ids;
	GatherAssetIds(Types, Ids);

	if (Ids.IsEmpty())
	{
		OnOutGameAssetsLoaded();
		return;
	}

	OutGameHandle = UAssetManager::Get().LoadPrimaryAssets(
		Ids,
		OutGameBundles,
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

	TArray<FPrimaryAssetId> Ids;
	GatherAssetIds(Types, Ids);

	if (Ids.IsEmpty())
	{
		OnRunAssetsLoaded();
		return;
	}

	RunHandle = UAssetManager::Get().LoadPrimaryAssets(
		Ids,
		RunBundles,
		FStreamableDelegate::CreateUObject(this, &UNSDataSubsystem::OnRunAssetsLoaded));
}

void UNSDataSubsystem::OnRunAssetsLoaded()
{
	CacheLoaded({ MonsterAssetType, AugmentAssetType, AugmentPoolAssetType, PartAssetType });
	BuildRewardDataRegistry();
	
	SetPhase(ENSDataLoadPhase::RunReady);
	OnRunGameDataReady.Broadcast();
}

void UNSDataSubsystem::BuildRewardDataRegistry()
{
	if (!IsValid(RewardDataRegistry))
	{
		RewardDataRegistry = NewObject<UNSRewardDataRegistry>(this);
	}
	
	const TArray<UNSRewardTriggerData*> RewardTriggerDataList =
		GetAllDataOfType<UNSRewardTriggerData>(RewardTriggerAssetType);
	
	RewardDataRegistry->Build(RewardTriggerDataList);
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
	if (RunHandle.IsValid())
	{
		RunHandle->ReleaseHandle();
		RunHandle.Reset();
	}
	
	if (IsValid(RewardDataRegistry))
	{
		RewardDataRegistry->Reset();
		RewardDataRegistry = nullptr;
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

void UNSDataSubsystem::GatherAssetIds(const TArray<FPrimaryAssetType>& Types, TArray<FPrimaryAssetId>& OutIds) const
{
	UAssetManager& AM = UAssetManager::Get();
	for (const FPrimaryAssetType& Type : Types)
	{
		TArray<FPrimaryAssetId> Ids;
		AM.GetPrimaryAssetIdList(Type, Ids);
		OutIds.Append(Ids);
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
