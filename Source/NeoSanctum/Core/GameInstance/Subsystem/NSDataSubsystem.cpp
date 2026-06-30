// Copyright 2026 One Team. All rights reserved.


#include "NSDataSubsystem.h"

#include "Engine/DataTable.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"
#include "NeoSanctum/Data/Augment/NSAugmentRarityRuleSet.h"
#include "NeoSanctum/Data/Augment/NSAugmentTypes.h"
#include "NeoSanctum/Data/Config/NSCommonDataConfig.h"
#include "NeoSanctum/Data/Config/NSLevelConfig.h"
#include "NeoSanctum/Data/Config/NSRunConfig.h"
#include "NeoSanctum/Data/Reward/NSRewardDataRegistry.h"
#include "NeoSanctum/Data/Reward/NSRewardTriggerData.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"

// Project Settings > Asset Manager 등록 이름과 반드시 일치

// Common (인런/아웃런 공통)
const FPrimaryAssetType UNSDataSubsystem::CommonDataConfigAssetType			= FPrimaryAssetType(TEXT("NSCommonDataConfig"));
const FPrimaryAssetType UNSDataSubsystem::CharacterAssetType				= FPrimaryAssetType(TEXT("NSCharacterData"));
const FPrimaryAssetType UNSDataSubsystem::HubAssetType						= FPrimaryAssetType(TEXT("NSHubData"));
const FPrimaryAssetType UNSDataSubsystem::PartAssetType						= FPrimaryAssetType(TEXT("NSPartData"));

// OutGame


// InRun
const FPrimaryAssetType UNSDataSubsystem::RunConfigAssetType				= FPrimaryAssetType(TEXT("NSRunConfig"));
const FPrimaryAssetType UNSDataSubsystem::LevelConfigAssetType				= FPrimaryAssetType(TEXT("NSLevelConfig"));
const FPrimaryAssetType UNSDataSubsystem::MonsterAssetType					= FPrimaryAssetType(TEXT("NSMonsterData"));
const FPrimaryAssetType UNSDataSubsystem::AugmentAssetType					= FPrimaryAssetType(TEXT("NSAugmentData"));
const FPrimaryAssetType UNSDataSubsystem::AugmentRarityRuleSetAssetType		= FPrimaryAssetType(TEXT("NSAugmentRarityRuleSet"));
const FPrimaryAssetType UNSDataSubsystem::RewardTriggerAssetType			= FPrimaryAssetType(TEXT("NSRewardTriggerData"));
// TODO: 레벨 전용 GA가 있다면 아웃런, 인런 구분해서 여기서 추가해서 사용하게끔

// DataAsset의 meta=(AssetBundles="...") 와 반드시 일치
const TArray<FName> UNSDataSubsystem::CommonBundles  = { FName("CommonUI"),  FName("CommonData")  };
const TArray<FName> UNSDataSubsystem::OutGameBundles = { FName("OutRunUI"),  FName("OutRunData")  };
const TArray<FName> UNSDataSubsystem::RunBundles     = { FName("InRunUI"),   FName("InRunData")   };

void UNSDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// 캐릭터 기본 데이터처럼 전 구간에서 쓰는 데이터는 게임 종료 전까지 유지.
	LoadCommonData();
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

const UNSRewardTriggerData* UNSDataSubsystem::FindRewardTriggerDataByTag(const FGameplayTag& TriggerTag) const
{
	if (!IsValid(RewardDataRegistry))
	{
		return nullptr;
	}
	
	return RewardDataRegistry->FindRewardTriggerDataByTag(TriggerTag);
}

const UNSRewardDataRegistry* UNSDataSubsystem::GetRewardDataRegistry() const
{
	return RewardDataRegistry;
}

const UNSCommonDataConfig* UNSDataSubsystem::GetCommonDataConfig() const
{
	const TArray<UNSCommonDataConfig*> CommonConfigs = 
		GetAllDataOfType<UNSCommonDataConfig>(CommonDataConfigAssetType);
	
	if (CommonConfigs.IsEmpty())
	{
		return nullptr;
	}
	
	if (CommonConfigs.Num() > 1)
	{
		NS_OBJ_LOG(LogNS, Warning,
			"NSCommonDataConfig가 여러 개 로드되었습니다. 첫 번째 설정을 사용합니다. Count={Count}",
			("Count", CommonConfigs.Num())
		);
	}
	
	return CommonConfigs[0];
}

UDataTable* UNSDataSubsystem::GetCommonAbilityBaseStatTable() const
{
	const UNSCommonDataConfig* CommonConfig = GetCommonDataConfig();
	if (!CommonConfig)
	{
		return nullptr;
	}
	
	return CommonConfig->AbilityBaseStatTable.Get();
}

UDataTable* UNSDataSubsystem::GetCurrentAugmentDefinitionTable() const
{
	if (!CurrentRunConfig)
	{
		return nullptr;
	}
	
	return CurrentRunConfig->AugmentDefinitionTable.Get();
}

const UNSAugmentRarityRuleSet* UNSDataSubsystem::GetCurrentAugmentRarityRuleSet() const
{
	if (!CurrentRunConfig)
	{
		return nullptr;
	}
	
	return CurrentRunConfig->AugmentRarityRuleSet.Get();
}

void UNSDataSubsystem::LoadCurrentStageSpawnerTables()
{
	if (bStageSpawnerTablesLoaded)
	{
		OnStageSpawnerTablesReady.Broadcast();
		return;
	}
	
	if (StageSpawnerTableHandle.IsValid())
	{
		return;
	}
	
	StartLoadStageSpawnerTables();
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

void UNSDataSubsystem::EnterRun(TSoftObjectPtr<UNSRunConfig> RunConfig, TSoftObjectPtr<UNSLevelConfig> LevelConfig)
{
	if (RunConfig.IsNull() || LevelConfig.IsNull())
	{
		return;
	}

	if (CurrentPhase == ENSDataLoadPhase::LoadingRun)
	{
		return;
	}

	const bool bNeedsRunReload = 
		!CurrentRunConfig ||
		CurrentRunConfig->GetPrimaryAssetId().PrimaryAssetName != FName(*RunConfig.GetAssetName());
	
	if (CurrentPhase == ENSDataLoadPhase::OutGameReady)
	{
		UnloadOutGame();
	}
	else if (CurrentPhase == ENSDataLoadPhase::RunReady)
	{
		UnloadStage();
	}

	if (bNeedsRunReload)
	{
		// UnloadRun()은 기존 런 상태와 Pending 값을 모두 비우므로
		// 새 RunConfig/LevelConfig는 언로드 이후에 다시 저장.
		UnloadRun();
	}
	
	PendingRunConfig = RunConfig;
	PendingStageLevelConfig = LevelConfig;
	
	if (bNeedsRunReload)
	{
		StartLoadRunConfig();
		return;
	}
	
	StartLoadStageConfig();
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

	const TArray<FPrimaryAssetType> Types = 
	{
		CommonDataConfigAssetType,
		CharacterAssetType
	};

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
	CacheLoaded(
{ 
			CommonDataConfigAssetType,
			CharacterAssetType
		}
	);
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

void UNSDataSubsystem::StartLoadRunConfig()
{
	SetPhase(ENSDataLoadPhase::LoadingRun);
	
	CurrentRunConfig = nullptr;
	PendingRunAssetIds.Reset();
	
	const FPrimaryAssetId RunConfigId(RunConfigAssetType, FName(*PendingRunConfig.GetAssetName()));

	RunConfigHandle = UAssetManager::Get().LoadPrimaryAsset(
		RunConfigId,
		RunBundles,
		FStreamableDelegate::CreateUObject(this, &UNSDataSubsystem::OnRunConfigLoaded)
	);
}

void UNSDataSubsystem::OnRunConfigLoaded()
{
	CurrentRunConfig = PendingRunConfig.Get();
	if (!IsValid(CurrentRunConfig))
	{
		SetPhase(ENSDataLoadPhase::NotStarted);
		return;
	}
	
	TArray<FPrimaryAssetId> Ids;
	
	// 파츠/보상 트리거는 런 전체에서 유지되는 데이터로 취급.
	GatherAssetIds({ PartAssetType, RewardTriggerAssetType }, Ids);
	
	if (!CurrentRunConfig->AugmentRarityRuleSet.IsNull())
	{
		Ids.AddUnique(FPrimaryAssetId(
			AugmentRarityRuleSetAssetType,
			FName(*CurrentRunConfig->AugmentRarityRuleSet.GetAssetName()))
		);
	}
	
	CollectAugmentDefinitionIdsFromTable(CurrentRunConfig->AugmentDefinitionTable.Get(), Ids);
	PendingRunAssetIds = Ids;
	
	if (Ids.IsEmpty())
	{
		OnRunAssetsLoaded();
		return;
	}
	
	RunHandle = UAssetManager::Get().LoadPrimaryAssets(
		PendingRunAssetIds,
		RunBundles,
		FStreamableDelegate::CreateUObject(this, &UNSDataSubsystem::OnRunAssetsLoaded)
	);
}

void UNSDataSubsystem::OnRunAssetsLoaded()
{
	// RunConfig에서 수집한 런 공통 에셋을 캐싱하고, 이후 스테이지 전용 LevelConfig를 로드.
	CacheLoadedByIds(PendingRunAssetIds);
	BuildRewardDataRegistry();
	
	StartLoadStageConfig();
}

void UNSDataSubsystem::StartLoadStageConfig()
{
	SetPhase(ENSDataLoadPhase::LoadingRun);
	
	CurrentRunLevelConfig = nullptr;
	PendingStageAssetIds.Reset();
	
	const FPrimaryAssetId LevelConfigId(LevelConfigAssetType, FName(*PendingStageLevelConfig.GetAssetName()));
	
	// Stage LevelConfig PrimaryAsset 자체만 캐시에 보관.
	// TravelMap과 Spawner DT는 travel 전 번들 로드 대상에서 제외.
	static const TArray<FName> ConfigOnlyBundles;

	StageLevelConfigHandle = UAssetManager::Get().LoadPrimaryAsset(
		LevelConfigId,
		ConfigOnlyBundles,
		FStreamableDelegate::CreateUObject(this, &UNSDataSubsystem::OnStageConfigLoaded)
	);
}

void UNSDataSubsystem::OnStageConfigLoaded()
{
	CurrentRunLevelConfig = PendingStageLevelConfig.Get();
	if (!IsValid(CurrentRunLevelConfig))
	{
		SetPhase(ENSDataLoadPhase::NotStarted);
		return;
	}
	
	// 현재 스테이지 LevelConfig PrimaryAsset 자체를 캐시에 보관.
	PendingStageAssetIds.AddUnique(CurrentRunLevelConfig->GetPrimaryAssetId());
	
	CacheLoadedByIds(PendingStageAssetIds);
	
	SetPhase(ENSDataLoadPhase::RunReady);
	OnRunGameDataReady.Broadcast();
}

void UNSDataSubsystem::StartLoadStageSpawnerTables()
{
	if (!IsValid(CurrentRunLevelConfig))
	{
		NS_OBJ_LOG(LogNS, Warning, "스테이지 스포너 DT 로드가 요청됐지만 CurrentRunLevelConfig가 아직 준비되지 않았습니다.");
		return;
	}
	
	TArray<FSoftObjectPath> AssetToLoad;
	
	if (!CurrentRunLevelConfig->MeleeSpawnerTable.IsNull())
	{
		AssetToLoad.Add(CurrentRunLevelConfig->MeleeSpawnerTable.ToSoftObjectPath());
	}
	
	if (!CurrentRunLevelConfig->RangeSpawnerTable.IsNull())
	{
		AssetToLoad.Add(CurrentRunLevelConfig->RangeSpawnerTable.ToSoftObjectPath());
	}
	
	if (AssetToLoad.IsEmpty())
	{
		OnStageSpawnerTableLoaded();
		return;
	}
	
	// 스테이지 안에서는 이 핸들을 유지해서 같은 DT를 여러 스포너가 재사용.
	FStreamableManager& StreamableManager = UAssetManager::Get().GetStreamableManager();
	StageSpawnerTableHandle = StreamableManager.RequestAsyncLoad(
		AssetToLoad,
		FStreamableDelegate::CreateUObject(this, &ThisClass::OnStageSpawnerTableLoaded)
	);
}

void UNSDataSubsystem::OnStageSpawnerTableLoaded()
{
	CurrentMeleeSpawnerTable = CurrentRunLevelConfig ? CurrentRunLevelConfig->MeleeSpawnerTable.Get() : nullptr;
	CurrentRangeSpawnerTable = CurrentRunLevelConfig ? CurrentRunLevelConfig->RangeSpawnerTable.Get() : nullptr;
	bStageSpawnerTablesLoaded = true;
	
	if (!CurrentMeleeSpawnerTable && !CurrentRangeSpawnerTable)
	{
		NS_OBJ_LOG(LogNS, Warning, "스테이지 스포너 DT 로드가 완료됐지만 근접/원거리 스폰 테이블이 모두 비어 있습니다.");
	}
	
	OnStageSpawnerTablesReady.Broadcast();
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
	UnloadByTypes(
	{ 
			CommonDataConfigAssetType,
			CharacterAssetType, 
		}
	);
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

void UNSDataSubsystem::UnloadStage()
{
	if (StageLevelConfigHandle.IsValid())
	{
		StageLevelConfigHandle->ReleaseHandle();
		StageLevelConfigHandle.Reset();
	}

	if (StageHandle.IsValid())
	{
		StageHandle->ReleaseHandle();
		StageHandle.Reset();
	}
	
	UnloadByIds(PendingStageAssetIds);
	
	PendingStageAssetIds.Reset();
	PendingStageLevelConfig.Reset();
	CurrentRunLevelConfig = nullptr;
	
	if (StageSpawnerTableHandle.IsValid())
	{
		StageSpawnerTableHandle->ReleaseHandle();
		StageSpawnerTableHandle.Reset();
	}
	
	CurrentMeleeSpawnerTable = nullptr;
	CurrentRangeSpawnerTable = nullptr;
	bStageSpawnerTablesLoaded = false;
}

void UNSDataSubsystem::UnloadRun()
{
	UnloadStage();
	
	if (RunConfigHandle.IsValid())
	{
		RunConfigHandle->ReleaseHandle();
		RunConfigHandle.Reset();
	}
	
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
	
	UnloadByIds(PendingRunAssetIds);
	
	PendingRunAssetIds.Reset();
	PendingRunConfig.Reset();
	CurrentRunConfig = nullptr;
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

void UNSDataSubsystem::CollectAugmentDefinitionIdsFromTable(
	const UDataTable* AugmentDefinitionTable, TArray<FPrimaryAssetId>& OutIds) const
{
	if (!IsValid(AugmentDefinitionTable) || 
		AugmentDefinitionTable->GetRowStruct() != FNSAugmentDefinitionRow::StaticStruct())
	{
		return;
	}
	
	TSet<FPrimaryAssetId> UniqueIds;
	const FString ContextString = TEXT("CollectAugmentDefinitionIdsFromTable");
	
	for (const FName& RowName : AugmentDefinitionTable->GetRowNames())
	{
		const FNSAugmentDefinitionRow* Row =
			AugmentDefinitionTable->FindRow<FNSAugmentDefinitionRow>(RowName, ContextString, false);
		
		if (!Row || Row->Definition.IsNull())
		{
			continue;
		}
		
		// DT_AugmentDefinition이 이번 런의 증강 후보 원본이므로 Row의 Definition만 선로딩 대상으로 수집.
		const FPrimaryAssetId DefId(AugmentAssetType, FName(*Row->Definition.GetAssetName()));
		if (DefId.IsValid())
		{
			UniqueIds.Add(DefId);
		}
	}
	
	OutIds.Append(UniqueIds.Array());
}

void UNSDataSubsystem::CacheLoadedByIds(const TArray<FPrimaryAssetId>& Ids)
{
	UAssetManager& AM = UAssetManager::Get();

	for (const FPrimaryAssetId& Id : Ids)
	{
		if (UObject* Obj = AM.GetPrimaryAssetObject(Id))
		{
			DataCache.Add(Id, Obj);
		}
	}
}

void UNSDataSubsystem::UnloadByIds(const TArray<FPrimaryAssetId>& Ids)
{
	UAssetManager& AM = UAssetManager::Get();

	for (const FPrimaryAssetId& Id : Ids)
	{
		DataCache.Remove(Id);
	}

	if (!Ids.IsEmpty())
	{
		AM.UnloadPrimaryAssets(Ids);
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
