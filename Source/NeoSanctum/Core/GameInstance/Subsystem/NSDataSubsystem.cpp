// Copyright 2026 One Team. All rights reserved.


#include "NSDataSubsystem.h"

#include "GameplayEffect.h"
#include "Engine/DataTable.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"
#include "NeoSanctum/Data/AI/NSEnemyVisualParameterTypes.h"
#include "NeoSanctum/Data/Augment/NSAugmentRarityRuleSet.h"
#include "NeoSanctum/Data/Augment/NSAugmentTypes.h"
#include "NeoSanctum/Data/Character/NSCharacterBaseStatTypes.h"
#include "NeoSanctum/Data/Config/NSCommonDataConfig.h"
#include "NeoSanctum/Data/Config/NSLevelConfig.h"
#include "NeoSanctum/Data/Config/NSOutGameDataConfig.h"
#include "NeoSanctum/Data/Config/NSRunConfig.h"
#include "NeoSanctum/Data/CommonUpgrade/NSCommonUpgradeTypes.h"
#include "NeoSanctum/Data/Reward/NSRewardDataRegistry.h"
#include "NeoSanctum/Data/Reward/NSRewardTriggerData.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Progression/Part/NSPartPreviewStage.h"
#include "NeoSanctum/Data/Sound/NSSoundData.h"
#include "NeoSanctum/Data/UI/NSCharacterSkillUISet.h"
#include "NeoSanctum/Data/UI/NSGoodsUIData.h"
#include "NeoSanctum/Data/UI/NSMonsterUIData.h"
#include "NeoSanctum/Data/UI/NSSkillUIData.h"
#include "NeoSanctum/Data/UI/NSUIWidgetData.h"
#include "NeoSanctum/Data/VFX/NSVFXDataTableRow.h"

// Project Settings > Asset Manager 등록 이름과 반드시 일치

// Common (인런/아웃런 공통)
const FPrimaryAssetType UNSDataSubsystem::CommonDataConfigAssetType			= FPrimaryAssetType(TEXT("NSCommonDataConfig"));
const FPrimaryAssetType UNSDataSubsystem::CharacterAssetType				= FPrimaryAssetType(TEXT("NSCharacterData"));
const FPrimaryAssetType UNSDataSubsystem::HubAssetType						= FPrimaryAssetType(TEXT("NSHubData"));
const FPrimaryAssetType UNSDataSubsystem::PartAssetType						= FPrimaryAssetType(TEXT("NSPartData"));

// OutGame
const FPrimaryAssetType UNSDataSubsystem::OutGameDataConfigAssetType		= FPrimaryAssetType(TEXT("NSOutGameDataConfig"));

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

UDataTable* UNSDataSubsystem::GetCommonCharacterBaseStatTable() const
{
	const UNSCommonDataConfig* CommonConfig = GetCommonDataConfig();
	return CommonConfig ? CommonConfig->CharacterBaseStatTable.Get() : nullptr;
}

TSubclassOf<UGameplayEffect> UNSDataSubsystem::GetCharacterBaseStatInitEffectClass() const
{
	const UNSCommonDataConfig* CommonConfig = GetCommonDataConfig();
	return CommonConfig ? CommonConfig->CharacterBaseStatInitEffectClass.Get() : nullptr;
}

const FNSCharacterBaseStatRow* UNSDataSubsystem::FindCharacterBaseStatRow(const FGameplayTag& CharacterTag) const
{
	UDataTable* Table = GetCommonCharacterBaseStatTable();
	if (!Table || !CharacterTag.IsValid())
	{
		return nullptr;
	}

	const FName RowName = CharacterTag.GetTagName();
	const FString ContextString = TEXT("FindCharacterBaseStatRow");
	return Table->FindRow<FNSCharacterBaseStatRow>(RowName, ContextString, false);
}

UDataTable* UNSDataSubsystem::GetCommonStatDisplayInfoTable() const
{
	const UNSCommonDataConfig* CommonConfig = GetCommonDataConfig();
	return CommonConfig ? CommonConfig->StatDisplayInfoTable.Get() : nullptr;
}

const FNSStatDisplayInfoRow* UNSDataSubsystem::FindStatDisplayInfoRow(const FGameplayTag& StatTag) const
{
	UDataTable* Table = GetCommonStatDisplayInfoTable();
	if (!Table || !StatTag.IsValid())
	{
		return nullptr;
	}

	const FName RowName = StatTag.GetTagName();
	const FString ContextString = TEXT("FindStatDisplayInfoRow");
	return Table->FindRow<FNSStatDisplayInfoRow>(RowName, ContextString, false);
}

TSubclassOf<UGameplayEffect> UNSDataSubsystem::GetCommonUpgradeInitEffectClass() const
{
	const UNSCommonDataConfig* CommonConfig = GetCommonDataConfig();
	return CommonConfig ? CommonConfig->CommonUpgradeInitEffectClass.Get() : nullptr;
}

TSubclassOf<UGameplayEffect> UNSDataSubsystem::GetInstantHealEffectClass() const
{
	const UNSRunConfig* RunConfig = GetCurrentRunConfig();
	return RunConfig ? RunConfig->InstantHealEffectClass.Get() : nullptr;
}

UDataTable* UNSDataSubsystem::GetHealPotionTable() const
{
	const UNSRunConfig* RunConfig = GetCurrentRunConfig();
	return RunConfig ? RunConfig->HealPotionTable.Get() : nullptr;
}

UNSSoundData* UNSDataSubsystem::GetCommonSoundData() const
{
	const UNSCommonDataConfig* CommonConfig = GetCommonDataConfig();
	return CommonConfig ? CommonConfig->SoundData.Get() : nullptr;
}

UDataTable* UNSDataSubsystem::GetCommonVFXDataTable() const
{
	const UNSCommonDataConfig* CommonConfig = GetCommonDataConfig();
	return CommonConfig ? CommonConfig->VFXDataTable.Get() : nullptr;
}

UDataTable* UNSDataSubsystem::GetCommonHitReactionDataTable() const
{
	const UNSCommonDataConfig* CommonConfig = GetCommonDataConfig();
	return CommonConfig ? CommonConfig->HitReactionDataTable.Get() : nullptr;
}

UDataTable* UNSDataSubsystem::GetCommonPlayerAttackFeedbackDataTable() const
{
	const UNSCommonDataConfig* CommonConfig = GetCommonDataConfig();
	return CommonConfig ? CommonConfig->PlayerAttackFeedbackDataTable.Get() : nullptr;
}

UDataTable* UNSDataSubsystem::GetCommonUIWidgetDataTable() const
{
	const UNSCommonDataConfig* CommonDataConfig = GetCommonDataConfig();
	return CommonDataConfig ? CommonDataConfig->UIWidgetDataTable.Get() : nullptr;
}

UDataTable* UNSDataSubsystem::GetCommonCharacterSkillUISetTable() const
{
	const UNSCommonDataConfig* CommonDataConfig = GetCommonDataConfig();
	return CommonDataConfig ? CommonDataConfig->CharacterSkillUISetTable.Get() : nullptr;
}

UDataTable* UNSDataSubsystem::GetCommonSkillUIDataTable() const
{
	const UNSCommonDataConfig* CommonDataConfig = GetCommonDataConfig();
	return CommonDataConfig ? CommonDataConfig->SkillUIDataTable.Get() : nullptr;
}

UDataTable* UNSDataSubsystem::GetCommonGoodsUIDataTable() const
{
	const UNSCommonDataConfig* CommonDataConfig = GetCommonDataConfig();
	return CommonDataConfig ? CommonDataConfig->GoodsUIDataTable.Get() : nullptr;
}

const FNSGoodsUIData* UNSDataSubsystem::FindCommonGoodsUIDataByTag(const FGameplayTag& GoodsTag) const
{
	const UDataTable* GoodsTable = GetCommonGoodsUIDataTable();
	if (!IsValid(GoodsTable) || GoodsTable->GetRowStruct() != FNSGoodsUIData::StaticStruct())
	{
		return nullptr;
	}

	TArray<FNSGoodsUIData*> Rows;
	GoodsTable->GetAllRows(TEXT("FindCommonGoodsUIDataByTag"), Rows);

	for (const FNSGoodsUIData* Row :Rows)
	{
		if (Row && Row->GoodsTag.MatchesTagExact(GoodsTag))
		{
			return Row;
		}
	}

	return nullptr;
}

UDataTable* UNSDataSubsystem::GetCommonGuideTextDataTable() const
{
	const UNSCommonDataConfig* CommonDataConfig = GetCommonDataConfig();
	return CommonDataConfig ? CommonDataConfig->GuideTextDataTable.Get() : nullptr;
}

const UNSOutGameDataConfig* UNSDataSubsystem::GetOutGameDataConfig() const
{
	const TArray<UNSOutGameDataConfig*> OutGameDataConfigs =
		GetAllDataOfType<UNSOutGameDataConfig>(OutGameDataConfigAssetType);

	if (OutGameDataConfigs.IsEmpty())
	{
		return nullptr;
	}

	return OutGameDataConfigs[0];
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

float UNSDataSubsystem::GetDefenseMitigationConstant() const
{
	const UNSRunConfig* RunConfig = GetCurrentRunConfig();
	// k가 0 이하이면 y = k/(k+Defense)가 항상 0이 되어 데미지가 전부 0으로 사라지므로 최소값 보정.
	const float RowConstant = RunConfig ? RunConfig->DefenseMitigationConstant : 100.0f;
	return FMath::Max(RowConstant, 1.0f);
}

float UNSDataSubsystem::GetMaxExperience() const
{
	const UNSRunConfig* RunConfig = GetCurrentRunConfig();
	// 0 이하이면 AddExperience의 while이 무한 루프가 되므로 최소값 보정.
	const float RawMax = RunConfig ? RunConfig->MaxExperience : 100.0f;
	return FMath::Max(RawMax, 1.0f);
}

UDataTable* UNSDataSubsystem::GetCommonMonsterUIDataTable() const
{
	const UNSCommonDataConfig* CommonDataConfig = GetCommonDataConfig();
	return CommonDataConfig ? CommonDataConfig->MonsterUIDataTable.Get() : nullptr;
}

const FNSMonsterUIData* UNSDataSubsystem::FindMonsterUIData(const FGameplayTag& EnemyId) const
{
	const UDataTable* MonsterUIProfileTable = GetCommonMonsterUIDataTable();
	if (!IsValid(MonsterUIProfileTable) ||
		MonsterUIProfileTable->GetRowStruct() != FNSMonsterUIData::StaticStruct() ||
		!EnemyId.IsValid())
	{
		return nullptr;
	}

	const FString ContextString = TEXT("FindMonsterUIProfileRow");
	if (const FNSMonsterUIData* Row =
		MonsterUIProfileTable->FindRow<FNSMonsterUIData>(EnemyId.GetTagName(), ContextString, false))
	{
		return Row;
	}

	TArray<FNSMonsterUIData*> Rows;
	MonsterUIProfileTable->GetAllRows(ContextString, Rows);

	for (const FNSMonsterUIData* Row : Rows)
	{
		if (Row && Row->EnemyId.IsValid() && Row->EnemyId.MatchesTagExact(EnemyId))
		{
			return Row;
		}
	}

	return nullptr;
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

void UNSDataSubsystem::PreloadOutGameData()
{
	// 이미 OutGame 준비,진행 중이면 아무것도 안 함
	if (IsOutGameReady() || CurrentPhase == ENSDataLoadPhase::LoadingOutGame)
	{
		return;
	}

	// Common이 이미 끝났으면 바로 OutGame
	if (IsCommonReady())
	{
		LoadOutGameData();
		return;
	}

	// 아직이면 Common 완료를 스스로 바인딩해서 이어감
	OnCommonDataReady.RemoveDynamic(this, &UNSDataSubsystem::HandlePreloadCommonReady);
	OnCommonDataReady.AddDynamic(this, &UNSDataSubsystem::HandlePreloadCommonReady);

	// NotStarted면 시작, 이미 로딩 중이면 작동안함
	LoadCommonData();
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

void UNSDataSubsystem::GetCurrentEnemyVisualParameterRows(
	const FGameplayTag& EnemyId,
	TArray<const FNSEnemyVisualParameterRow*>& OutRows) const
{
	OutRows.Reset();

	if (!EnemyId.IsValid())
	{
		return;
	}

	const UDataTable* Table = CurrentEnemyVisualParameterTable.Get();
	if (!IsValid(Table) || Table->GetRowStruct() != FNSEnemyVisualParameterRow::StaticStruct())
	{
		return;
	}

	TArray<FNSEnemyVisualParameterRow*> Rows;
	Table->GetAllRows<FNSEnemyVisualParameterRow>(
		TEXT("GetCurrentEnemyVisualParameterRows"),
		Rows);

	for (const FNSEnemyVisualParameterRow* Row : Rows)
	{
		if (!Row || !Row->bEnabled)
		{
			continue;
		}

		if (Row->EnemyId.MatchesTagExact(EnemyId))
		{
			OutRows.Add(Row);
		}
	}
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
	BuildPartRowCache();
	BuildSlotRowCache();
	BuildPartShopRerollRowCache();
	BuildDroppedPartConfigCache();
	CacheCommonUpgradeNodeRows();
	StartLoadCommonReferenceAssets();
}

void UNSDataSubsystem::StartLoadCommonReferenceAssets()
{
	TArray<FSoftObjectPath> AssetsToLoad;
	
	// VFX DT Row 안의 NiagaraSystem은 DataAsset 번들로만으로는 보장되지 않으므로 별도 선로드.
	CollectVFXSystemPathsFromTable(GetCommonVFXDataTable(), AssetsToLoad);
	// UIManager가 생성하는 위젯 클래스는 Row 안의 SoftClass라서 DataTable과 별도로 선로드.
	CollectUIWidgetClassPathsFromTable(GetCommonUIWidgetDataTable(), AssetsToLoad);
	// HUD/Goods/Skill가 필요한 아이콘 SoftObject를 미리 선로드.
	CollectCommonUIIconPaths(
		GetCommonCharacterSkillUISetTable(),
		GetCommonSkillUIDataTable(),
		GetCommonGoodsUIDataTable(),
		AssetsToLoad
	);
	
	if (AssetsToLoad.IsEmpty())
	{
		OnCommonReferenceAssetsLoaded();
		return;
	}
	
	FStreamableManager& StreamableManager = UAssetManager::Get().GetStreamableManager();
	CommonReferencedAssetsHandle = StreamableManager.RequestAsyncLoad(
		AssetsToLoad,
		FStreamableDelegate::CreateUObject(this, &ThisClass::OnCommonReferenceAssetsLoaded)
	);
}

void UNSDataSubsystem::OnCommonReferenceAssetsLoaded()
{
	CacheCommonFeedbackRows();
	
	SetPhase(ENSDataLoadPhase::CommonReady);
	NS_NET_LOG(this, LogNS, Warning, "CommonData 로딩 완료");
	OnCommonDataReady.Broadcast();
}

void UNSDataSubsystem::CollectVFXSystemPathsFromTable(
	const UDataTable* VFXTable, TArray<FSoftObjectPath>& OutPaths) const
{
	if (!IsValid(VFXTable) || VFXTable->GetRowStruct() != FNSVFXDataTableRow::StaticStruct())
	{
		return;
	}
	
	const FString ContextString = TEXT("CollectVFXSystemPathsFromTable");
	for (const FName& RowName : VFXTable->GetRowNames())
	{
		const FNSVFXDataTableRow* Row = VFXTable->FindRow<FNSVFXDataTableRow>(RowName, ContextString, false);
		if (Row && !Row->NiagaraSystem.IsNull())
		{
			OutPaths.AddUnique(Row->NiagaraSystem.ToSoftObjectPath());
		}
	}
}

void UNSDataSubsystem::CollectUIWidgetClassPathsFromTable(
	const UDataTable* UIWidgetTable, TArray<FSoftObjectPath>& OutPaths) const
{
	if (!IsValid(UIWidgetTable) || UIWidgetTable->GetRowStruct() != FNSUIWidgetData::StaticStruct())
	{
		return;
	}

	const FString ContextString = TEXT("CollectUIWidgetClassPathsFromTable");
	for (const FName& RowName : UIWidgetTable->GetRowNames())
	{
		const FNSUIWidgetData* Row = UIWidgetTable->FindRow<FNSUIWidgetData>(RowName, ContextString, false);

		if (Row && !Row->WidgetClass.IsNull())
		{
			OutPaths.AddUnique(Row->WidgetClass.ToSoftObjectPath());
		}
	}
}

void UNSDataSubsystem::CollectCommonUIIconPaths(
	const UDataTable* CharacterSkillUISetTable,
	const UDataTable* SkillUIDataTable,
	const UDataTable* GoodsUIDataTable,
	TArray<FSoftObjectPath>& OutPaths) const
{
	if (IsValid(CharacterSkillUISetTable) &&
		CharacterSkillUISetTable->GetRowStruct() == FNSCharacterSkillUISet::StaticStruct())
	{
		const FString ContextString = TEXT("CollectCommonUIIconPaths_CharacterSkillUISet");
		for (const FName& RowName : CharacterSkillUISetTable->GetRowNames())
		{
			const FNSCharacterSkillUISet* Row =
				CharacterSkillUISetTable->FindRow<FNSCharacterSkillUISet>(RowName, ContextString, false);

			if (!Row)
			{
				continue;
			}

			if (!Row->Skill1InputDisplay.InputIcon.IsNull())
			{
				OutPaths.AddUnique(Row->Skill1InputDisplay.InputIcon.ToSoftObjectPath());
			}

			if (!Row->Skill2InputDisplay.InputIcon.IsNull())
			{
				OutPaths.AddUnique(Row->Skill2InputDisplay.InputIcon.ToSoftObjectPath());
			}

			if (!Row->Skill3InputDisplay.InputIcon.IsNull())
			{
				OutPaths.AddUnique(Row->Skill3InputDisplay.InputIcon.ToSoftObjectPath());
			}
		}
	}

	if (IsValid(SkillUIDataTable) && SkillUIDataTable->GetRowStruct() == FNSSkillUIData::StaticStruct())
	{
		const FString ContextString = TEXT("CollectCommonUIIconPaths_SkillUI");
		for (const FName& RowName : SkillUIDataTable->GetRowNames())
		{
			const FNSSkillUIData* Row =
				SkillUIDataTable->FindRow<FNSSkillUIData>(RowName, ContextString, false);

			if (Row && !Row->SkillIcon.IsNull())
			{
				OutPaths.AddUnique(Row->SkillIcon.ToSoftObjectPath());
			}
		}
	}

	if (IsValid(GoodsUIDataTable) && GoodsUIDataTable->GetRowStruct() == FNSGoodsUIData::StaticStruct())
	{
		const FString ContextString = TEXT("CollectCommonUIIconPaths_GoodsUI");
		for (const FName& RowName : GoodsUIDataTable->GetRowNames())
		{
			const FNSGoodsUIData* Row =
				GoodsUIDataTable->FindRow<FNSGoodsUIData>(RowName, ContextString, false);

			if (Row && !Row->GoodsIcon.IsNull())
			{
				OutPaths.AddUnique(Row->GoodsIcon.ToSoftObjectPath());
			}
		}
	}
}

void UNSDataSubsystem::CacheCommonFeedbackRows()
{
	CachedHitReactionRows.Reset();
	CachedPlayerAttackFeedbackRows.Reset();
	
	if (const UDataTable* HitReactionTable = GetCommonHitReactionDataTable())
	{
		TArray<FNSHitReactionData*> Rows;
		HitReactionTable->GetAllRows(TEXT("CacheCommonFeedbackRows"), Rows);
		for (const FNSHitReactionData* Row : Rows)
		{
			if (Row)
			{
				CachedHitReactionRows.Add(*Row);
			}
		}
	}
	
	if (const UDataTable* PlayerAttackFeedbackTable = GetCommonPlayerAttackFeedbackDataTable())
	{
		TArray<FNSPlayerAttackFeedbackData*> Rows;
		PlayerAttackFeedbackTable->GetAllRows(TEXT("CacheCommonPlayerAttackFeedbackRows"), Rows);
		for (const FNSPlayerAttackFeedbackData* Row : Rows)
		{
			if (Row)
			{
				CachedPlayerAttackFeedbackRows.Add(*Row);
			}
		}
	}
}

// ================================================================
// OutGame 로드
// ================================================================

void UNSDataSubsystem::StartLoadOutGame()
{
	SetPhase(ENSDataLoadPhase::LoadingOutGame);

	const TArray<FPrimaryAssetType> Types =
	{
		OutGameDataConfigAssetType,
		HubAssetType,
		PartAssetType
	};

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
	CacheLoaded(
	{
		OutGameDataConfigAssetType,
		HubAssetType,
		PartAssetType
	});

	StartLoadOutGameReferenceAssets();
}

void UNSDataSubsystem::StartLoadOutGameReferenceAssets()
{
	TArray<FSoftObjectPath> AssetsToLoad;

	if (const UNSOutGameDataConfig* OutGameConfig = GetOutGameDataConfig())
	{
		CollectCharacterSelectPathsFromTable(
			OutGameConfig->CharacterSelectDataTable.Get(),
			AssetsToLoad
		);
	}

	if (AssetsToLoad.IsEmpty())
	{
		OnOutGameReferenceAssetsLoaded();
		return;
	}

	FStreamableManager& StreamableManager = UAssetManager::Get().GetStreamableManager();
	OutGameReferencedAssetHandle = StreamableManager.RequestAsyncLoad(
		AssetsToLoad,
		FStreamableDelegate::CreateUObject(this, &ThisClass::OnOutGameReferenceAssetsLoaded)
	);
}

void UNSDataSubsystem::OnOutGameReferenceAssetsLoaded()
{
	CacheCharacterSelectRows();

	CacheLoaded({ HubAssetType, PartAssetType });

	// 파츠샵을 열기 전에 모든 파츠 메시를 미리 로드해 3D프리뷰가 잘 나오게
	ANSPartPreviewStage::WarmupAllPartMeshes(GetGameInstance());

	SetPhase(ENSDataLoadPhase::OutGameReady);
	NS_NET_LOG(this, LogNS, Warning, "OutGameData 로딩 완료");
	OnOutGameDataReady.Broadcast();
}

void UNSDataSubsystem::CollectCharacterSelectPathsFromTable(
	const UDataTable* CharacterSelectTable, TArray<FSoftObjectPath>& OutPaths) const
{
	if (!IsValid(CharacterSelectTable) || CharacterSelectTable->GetRowStruct() != FNSCharacterSelectData::StaticStruct())
	{
		return;
	}

	const FString ContextString = TEXT("CollectCharacterSelectPathsFromTable");
	for (const FName& RowName : CharacterSelectTable->GetRowNames())
	{
		const FNSCharacterSelectData* Row =
			CharacterSelectTable->FindRow<FNSCharacterSelectData>(RowName, ContextString, false);

		if (!Row)
		{
			continue;
		}

		if (!Row->CharacterData.IsNull())
		{
			OutPaths.AddUnique(Row->CharacterData.ToSoftObjectPath());
		}

		if (!Row->PreviewTexture.IsNull())
		{
			OutPaths.AddUnique(Row->PreviewTexture.ToSoftObjectPath());
		}
	}
}

void UNSDataSubsystem::CacheCharacterSelectRows()
{
	CachedCharacterSelectRows.Reset();

	const UNSOutGameDataConfig* OutGameConfig = GetOutGameDataConfig();
	const UDataTable* CharacterSelectTable = OutGameConfig ? OutGameConfig->CharacterSelectDataTable.Get() : nullptr;

	if (!IsValid(CharacterSelectTable) || CharacterSelectTable->GetRowStruct() != FNSCharacterSelectData::StaticStruct())
	{
		return;
	}

	TArray<FNSCharacterSelectData*> Rows;
	CharacterSelectTable->GetAllRows(TEXT("CachedCharacterSelectRows"), Rows);

	for (const FNSCharacterSelectData* Row : Rows)
	{
		if (Row)
		{
			CachedCharacterSelectRows.Add(*Row);
		}
	}
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
	
	StartLoadStageEnemyVisualParameterTable();
}

void UNSDataSubsystem::StartLoadStageSpawnerTables()
{
	if (!IsValid(CurrentRunLevelConfig))
	{
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

	NS_NET_LOG(this, LogNS, Warning,
		"StageSpawnerTable 로딩 완료. Melee={Melee}, Range={Range}",
		("Melee", CurrentMeleeSpawnerTable ? CurrentMeleeSpawnerTable->GetName() : FString(TEXT("None"))),
		("Range", CurrentRangeSpawnerTable ? CurrentRangeSpawnerTable->GetName() : FString(TEXT("None")))
	);
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
	
	if (CommonReferencedAssetsHandle.IsValid())
	{
		CommonReferencedAssetsHandle->ReleaseHandle();
		CommonReferencedAssetsHandle.Reset();
	}
	
	CachedHitReactionRows.Reset();
	CachedPlayerAttackFeedbackRows.Reset();
	
	UnloadByTypes(
	{
		CommonDataConfigAssetType,
		CharacterAssetType,
	});
}

void UNSDataSubsystem::UnloadOutGame()
{
	if (OutGameHandle.IsValid())
	{
		OutGameHandle->ReleaseHandle();
		OutGameHandle.Reset();
	}

	if (OutGameReferencedAssetHandle.IsValid())
	{
		OutGameReferencedAssetHandle->ReleaseHandle();
		OutGameReferencedAssetHandle.Reset();
	}

	CachedCharacterSelectRows.Reset();

	UnloadByTypes({OutGameDataConfigAssetType, HubAssetType, PartAssetType });
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
	
	if (StageEnemyVisualParameterTableHandle.IsValid())
	{
		StageEnemyVisualParameterTableHandle->ReleaseHandle();
		StageEnemyVisualParameterTableHandle.Reset();
	}

	CurrentEnemyVisualParameterTable = nullptr;
	bStageEnemyVisualParameterTableLoaded = false;
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

// ================================================================
// 파츠 row 캐시
// ================================================================

void UNSDataSubsystem::BuildPartRowCache()
{
	CachedPartRowsByDefId.Empty();

	const UNSCommonDataConfig* CommonConfig = GetCommonDataConfig();
	UDataTable* DT = CommonConfig ? CommonConfig->PartsBaseStatTable.Get() : nullptr;
	if (!DT)
	{
		return;
	}

	for (const FName& RowName : DT->GetRowNames())
	{
		const FNSPartDefinitionRow* Row =
			DT->FindRow<FNSPartDefinitionRow>(RowName, TEXT("BuildPartRowCache"), false);
		if (!Row || !Row->bEnabled || Row->Definition.IsNull())
		{
			continue;
		}

		const FPrimaryAssetId DefId =
			UAssetManager::Get().GetPrimaryAssetIdForPath(Row->Definition.ToSoftObjectPath());
		if (DefId.IsValid())
		{
			CachedPartRowsByDefId.Add(DefId, *Row);
		}
	}
}

const FNSPartDefinitionRow* UNSDataSubsystem::GetPartRow(const FPrimaryAssetId& DefId) const
{
	return CachedPartRowsByDefId.Find(DefId);
}

const TMap<FPrimaryAssetId, FNSPartDefinitionRow>& UNSDataSubsystem::GetAllPartRows() const
{
	return CachedPartRowsByDefId;
}

// ================================================================
// 슬롯 row 캐시
// ================================================================

void UNSDataSubsystem::BuildSlotRowCache()
{
	CachedSlotRowsBySlot.Empty();

	const UNSCommonDataConfig* CommonConfig = GetCommonDataConfig();
	UDataTable* DT = CommonConfig ? CommonConfig->PartsSlotBaseStatTable.Get() : nullptr;
	if (!DT)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DataSubsystem] PartSlotTable이 설정되지 않았습니다."));
		return;
	}

	for (const FName& RowName : DT->GetRowNames())
	{
		const FNSPartSlotRow* Row =
			DT->FindRow<FNSPartSlotRow>(RowName, TEXT("BuildSlotRowCache"), false);
		if (!Row || !Row->bEnabled)
		{
			continue;
		}
		CachedSlotRowsBySlot.Add(Row->SlotTag, *Row);
	}
}

const FNSPartSlotRow* UNSDataSubsystem::GetSlotRow(FGameplayTag Slot) const
{
	return CachedSlotRowsBySlot.Find(Slot);
}

const TMap<FGameplayTag, FNSPartSlotRow>& UNSDataSubsystem::GetAllSlotRows() const
{
	return CachedSlotRowsBySlot;
}

// ================================================================
// CommonUpgrade row 캐시
// ================================================================
void UNSDataSubsystem::CacheCommonUpgradeNodeRows()
{
	CachedCommonUpgradeNodeRows.Reset();

	const UNSCommonDataConfig* CommonConfig = GetCommonDataConfig();
	UDataTable* DT = CommonConfig ? CommonConfig->CommonUpgradeNodeTable.Get() : nullptr;
	if (!DT)
	{
		NS_NET_LOG(this, LogNS, Warning, "CommonUpgradeNodeTable이 설정되지 않았습니다.");
		return;
	}

	for (const FName& RowName : DT->GetRowNames())
	{
		const FString ContextString = TEXT("CacheCommonUpgradeNodeRows");
		const FNSCommonUpgradeNodeRow* Row =
			DT->FindRow<FNSCommonUpgradeNodeRow>(RowName, ContextString, false);
		if (!Row)
		{
			continue;
		}

		CachedCommonUpgradeNodeRows.Add(RowName, *Row);
	}
}

void UNSDataSubsystem::HandlePreloadCommonReady()
{
	OnCommonDataReady.RemoveDynamic(
		this,
		&UNSDataSubsystem::HandlePreloadCommonReady);
	LoadOutGameData();
}

void UNSDataSubsystem::StartLoadStageEnemyVisualParameterTable()
{
	CurrentEnemyVisualParameterTable = nullptr;
	bStageEnemyVisualParameterTableLoaded = false;

	if (!IsValid(CurrentRunLevelConfig))
	{
		OnStageEnemyVisualParameterTableLoaded();
		return;
	}

	if (CurrentRunLevelConfig->EnemyVisualParameterTable.IsNull())
	{
		OnStageEnemyVisualParameterTableLoaded();
		return;
	}

	FStreamableManager& StreamableManager = UAssetManager::Get().GetStreamableManager();

	// EnemyVisualParameterTable 비동기 로드 요청에 사용할 에셋 경로 변수
	const FSoftObjectPath TablePath =
		CurrentRunLevelConfig->EnemyVisualParameterTable.ToSoftObjectPath();

	StageEnemyVisualParameterTableHandle = StreamableManager.RequestAsyncLoad(
		TablePath,
		FStreamableDelegate::CreateUObject(
			this,
			&ThisClass::OnStageEnemyVisualParameterTableLoaded));
}

void UNSDataSubsystem::OnStageEnemyVisualParameterTableLoaded()
{
	CurrentEnemyVisualParameterTable =
		CurrentRunLevelConfig
			? CurrentRunLevelConfig->EnemyVisualParameterTable.Get()
			: nullptr;

	bStageEnemyVisualParameterTableLoaded = true;

	if (CurrentEnemyVisualParameterTable &&
		CurrentEnemyVisualParameterTable->GetRowStruct() != FNSEnemyVisualParameterRow::StaticStruct())
	{
		NS_NET_LOG(this, LogNS, Warning,
			"EnemyVisualParameterTable RowStruct가 FNSEnemyVisualParameterRow가 아닙니다. Table={Table}",
			("Table", GetNameSafe(CurrentEnemyVisualParameterTable))
		);

		CurrentEnemyVisualParameterTable = nullptr;
	}

	FinishStageConfigLoad();
}

void UNSDataSubsystem::FinishStageConfigLoad()
{
	SetPhase(ENSDataLoadPhase::RunReady);

	NS_NET_LOG(this, LogNS, Warning,
		"RunData 로딩 완료. RunConfig={RunConfig}, LevelConfig={LevelConfig}, EnemyVisualParameterTable={EnemyVisualParameterTable}",
		("RunConfig", CurrentRunConfig ? CurrentRunConfig->GetName() : FString(TEXT("None"))),
		("LevelConfig", CurrentRunLevelConfig ? CurrentRunLevelConfig->GetName() : FString(TEXT("None"))),
		("EnemyVisualParameterTable", CurrentEnemyVisualParameterTable ? CurrentEnemyVisualParameterTable->GetName() : FString(TEXT("None")))
	);

	OnRunGameDataReady.Broadcast();
}

const FNSCommonUpgradeNodeRow* UNSDataSubsystem::GetCommonUpgradeNodeRow(FName NodeId) const
{
	return CachedCommonUpgradeNodeRows.Find(NodeId);
}

const TMap<FName, FNSCommonUpgradeNodeRow>& UNSDataSubsystem::GetAllCommonUpgradeNodeRows() const
{
	return CachedCommonUpgradeNodeRows;
}

// ================================================================
// 파츠 리롤/상점 row 캐시
// ================================================================

void UNSDataSubsystem::BuildPartShopRerollRowCache()
{
	CachedShopRerollRowsByRarity.Empty();

	const UNSCommonDataConfig* CommonConfig = GetCommonDataConfig();
	UDataTable* DT = CommonConfig ? CommonConfig->PartsUpgradeTable.Get() : nullptr;
	if (!DT)
	{
		return;
	}

	for (const FName& RowName : DT->GetRowNames())
	{
		const FNSPartShopRerollRow* Row =
			DT->FindRow<FNSPartShopRerollRow>(RowName, TEXT("BuildPartShopRerollRowCache"), false);
		if (!Row)
		{
			continue;
		}
		CachedShopRerollRowsByRarity.Add(Row->Rarity, *Row);
	}
}

const FNSPartShopRerollRow* UNSDataSubsystem::GetPartShopRerollRow(ENSPartRarity Rarity) const
{
	return CachedShopRerollRowsByRarity.Find(Rarity);
}

const TMap<ENSPartRarity, FNSPartShopRerollRow>& UNSDataSubsystem::GetAllPartShopRerollRows() const
{
	return CachedShopRerollRowsByRarity;
}

void UNSDataSubsystem::BuildDroppedPartConfigCache()
{
	bDroppedPartConfigRowLoaded = false;

	const UNSCommonDataConfig* CommonConfig = GetCommonDataConfig();
	UDataTable* DT = CommonConfig ? CommonConfig->DroppedPartConfigTable.Get() : nullptr;
	if (!DT)
	{
		return;
	}

	if (const FNSDroppedPartConfigRow* Row =
		DT->FindRow<FNSDroppedPartConfigRow>(TEXT("Default"), TEXT("BuildDroppedPartConfigCache"), false))
	{
		CachedDroppedPartConfigRow = *Row;
		bDroppedPartConfigRowLoaded = true;
	}
}

const FNSDroppedPartConfigRow* UNSDataSubsystem::GetDroppedPartConfigRow() const
{
	return bDroppedPartConfigRowLoaded ? &CachedDroppedPartConfigRow : nullptr;
}
