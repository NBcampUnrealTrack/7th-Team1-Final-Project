// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/StreamableManager.h"
#include "NeoSanctum/Core/PlayerState/NSProgressTypes.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NeoSanctum/Data/Combat/NSHitReactionData.h"
#include "NeoSanctum/Data/Combat/NSPlayerAttackFeedbackData.h"
#include "NeoSanctum/Data/CommonUpgrade/NSCommonUpgradeTypes.h"
#include "NeoSanctum/Data/UI/NSCharacterSelectData.h"
#include "NSDataSubsystem.generated.h"

struct FNSCharacterBaseStatRow;
struct FNSGoodsUIData;
struct FNSMonsterUIData;
class UNSOutGameDataConfig;
class UNSSoundData;
class UNSAugmentRarityRuleSet;
class UNSCommonDataConfig;
class UNSRunConfig;
class UDataTable;
class UNSLevelConfig;
class UNSRewardDataRegistry;
class UNSRewardTriggerData;

UENUM(BlueprintType)
enum class ENSDataLoadPhase : uint8
{
	NotStarted       UMETA(DisplayName = "시작 전"),
	LoadingCommon    UMETA(DisplayName = "공용 데이터 로딩"),
	CommonReady      UMETA(DisplayName = "공용 데이터 준비 완료"),
	LoadingOutGame   UMETA(DisplayName = "아웃런 데이터 로딩"),
	OutGameReady     UMETA(DisplayName = "아웃런 준비 완료"),
	LoadingRun       UMETA(DisplayName = "인런 데이터 로딩"),
	RunReady         UMETA(DisplayName = "인런 준비 완료")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNSCommonDataReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNSOutGameDataReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNSRunGameDataReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNSStageSpawnerTablesReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNSDataPhaseChanged, ENSDataLoadPhase, NewPhase);

/**
 * 게임 데이터(공용/거점/인런) 로드/언로드를 일괄 관리하는 GameInstanceSubsystem.
 *
 * 로드 페이즈
 *  - App Start -> LoadCommonData()              : 공용 데이터 로드 (인런/아웃런 공통, 게임 종료 전까지 유지)
 *  - Title -> (게임시작) -> LoadOutGameData()   : OutGame 데이터 로드
 *  - OutGame -> (인런 진입) -> EnterRun()       : OutGame 언로드, Run 로드
 *  - InGame -> (아웃런 복귀) -> ReturnToOutGame(): Run 언로드, OutGame 재로드
 */
UCLASS()
class NEOSANCTUM_API UNSDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ================================================================
	// 데이터 접근 API
	// ================================================================

	template<typename T>
	T* GetData(const FPrimaryAssetId& Id) const;

	template<typename T>
	TArray<T*> GetAllDataOfType(const FPrimaryAssetType& AssetType) const;

	// ================================================================
	// UGameInstanceSubsystem 인터페이스
	// ================================================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ================================================================
	// 공개 API
	// ================================================================

	UFUNCTION(BlueprintCallable, Category = "NS|DataSubsystem", meta = (WorldContext = "WorldContextObject"))
	static UNSDataSubsystem* Get(const UObject* WorldContextObject);

	// 앱 시작 시 1회 호출 (인런/아웃런 공통 데이터 - 게임 종료 전까지 유지)
	UFUNCTION(BlueprintCallable, Category = "NS|DataSubsystem")
	void LoadCommonData();

	// 타이틀 -> 거점지역 진입 시 호출
	UFUNCTION(BlueprintCallable, Category = "NS|DataSubsystem")
	void LoadOutGameData();
	
	// Common→OutGame을 한 번에 선로딩, 호출자가 죽어도 자기 체인
	UFUNCTION(BlueprintCallable, Category = "NS|DataSubsystem")
	void PreloadOutGameData();

	/**
	 * 선택된 LevelConfig 기준으로 인런 데이터를 준비.
	 *
	 * 거점 -> 인런 첫 진입에서는 OutGame 데이터를 언로드.
	 * 인런 -> 다음 스테이지 전환에서는 기존 Run 데이터를 언로드한 뒤 새 Run 데이터를 로드.
	 */
	UFUNCTION(BlueprintCallable, Category = "NS|DataSubsystem")
	void EnterRun(
		TSoftObjectPtr<UNSRunConfig> RunConfig, TSoftObjectPtr<UNSLevelConfig> LevelConfig);
	
	// 인런 -> 거점지역 복귀 시 호출 (Run 언로드 -> OutGame 재로드)
	UFUNCTION(BlueprintCallable, Category = "NS|DataSubsystem")
	void ReturnToOutGame();

	UFUNCTION(BlueprintPure, Category = "NS|DataSubsystem")
	ENSDataLoadPhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category = "NS|DataSubsystem")
	bool IsCommonReady() const { return CurrentPhase >= ENSDataLoadPhase::CommonReady; }

	UFUNCTION(BlueprintPure, Category = "NS|DataSubsystem")
	bool IsOutGameReady() const { return CurrentPhase == ENSDataLoadPhase::OutGameReady; }

	UFUNCTION(BlueprintPure, Category = "NS|DataSubsystem")
	bool IsRunReady() const { return CurrentPhase == ENSDataLoadPhase::RunReady; }

	const UNSRewardTriggerData* FindRewardTriggerDataByTag(const FGameplayTag& TriggerTag) const;

	const UNSRewardDataRegistry* GetRewardDataRegistry() const;
	
	const UNSCommonDataConfig* GetCommonDataConfig() const;

	// CommonDataConfig가 가진 스킬 기본 스탯 테이블.
	// UNSCombatStatComponent는 이 테이블을 받아 BeginPlay 또는 CommonData로드 완료 시 캐싱.
	UDataTable* GetCommonAbilityBaseStatTable() const;

	UDataTable* GetCommonCharacterBaseStatTable() const;
	TSubclassOf<UGameplayEffect> GetCharacterBaseStatInitEffectClass() const;
	const FNSCharacterBaseStatRow* FindCharacterBaseStatRow(const FGameplayTag& CharacterTag) const;

	TSubclassOf<UGameplayEffect> GetCommonUpgradeInitEffectClass() const;

	// 회복 아이템 픽업 시 적용하는 공용 즉시 회복 GE
	TSubclassOf<UGameplayEffect> GetInstantHealEffectClass() const;

	// PotionTag로 회복/메시를 조회하는 회복 포션 정의 테이블
	UDataTable* GetHealPotionTable() const;

	UNSSoundData* GetCommonSoundData() const;
	UDataTable* GetCommonVFXDataTable() const;
	UDataTable* GetCommonHitReactionDataTable() const;
	UDataTable* GetCommonPlayerAttackFeedbackDataTable() const;

	UDataTable* GetCommonUIWidgetDataTable() const;
	// 캐릭터별 스킬 슬롯 구성을 정의하는 공용 UI 테이블.
	UDataTable* GetCommonCharacterSkillUISetTable() const;
	// 개별 스킬 아이콘/태그 표시 정보를 정의하는 공용 UI 테이블.
	UDataTable* GetCommonSkillUIDataTable() const;
	// 인런/아웃런 재화 아이콘 표시 정보를 정의하는 공용 UI 테이블.
	UDataTable* GetCommonGoodsUIDataTable() const;
	// 공통 UI 재화 테이블에서 태그에 해당하는 재화 표시 데이터를 찾음.
	const FNSGoodsUIData* FindCommonGoodsUIDataByTag(const FGameplayTag& GoodsTag) const;
	// 아웃런 목표 안내(웨이포인트) 텍스트 테이블
	UDataTable* GetCommonGuideTextDataTable() const;

	// 현재 프로젝트는 OutGameDataConfig를 하나만 운용.
	// 여러 개가 등록되면 첫 번째 로드 에셋을 사용하므로 Asset Manager 등록/에셋 수를 1개로 유지해야 함.
	const UNSOutGameDataConfig* GetOutGameDataConfig() const;

	// 거점 캐릭터 선택 UI에서 사용할 캐릭터 목록 Row 캐시. LoadOutGameData() 완료 이후 유효.
	const TArray<FNSCharacterSelectData>& GetCachedCharacterSelectRows() const { return CachedCharacterSelectRows; }
	
	const TArray<FNSHitReactionData>& GetCachedHitReactionRows() const { return CachedHitReactionRows; }
	const TArray<FNSPlayerAttackFeedbackData>& GetCachedPlayerAttackFeedbackRows() const { return CachedPlayerAttackFeedbackRows; }

	const UNSRunConfig* GetCurrentRunConfig() const { return CurrentRunConfig.Get(); }
	// 현재 런에서 사용하는 증강 후보 테이블.
	// EnterRun() 완료 이후 유효하며, 증강 선택/스탯 Modifier 캐싱의 기준 데이터로 사용.
	UDataTable* GetCurrentAugmentDefinitionTable() const;
	// 현재 런에서 사용하는 증강 희귀도/가중치 규칙.
	// EnterRun() 완료 이후 유효.
	const UNSAugmentRarityRuleSet* GetCurrentAugmentRarityRuleSet() const;

	// 방어력 계수를 반환.
	float GetDefenseMitigationConstant() const;

	// 최대 경험치를 반환.
	float GetMaxExperience() const;
	
	const UNSLevelConfig* GetCurrentRunLevelConfig() const { return CurrentRunLevelConfig.Get(); }
	
	// 공용 몬스터 UI 프로필 테이블을 반환하는 함수
	UDataTable* GetCommonMonsterUIDataTable() const;

	// EnemyId에 해당하는 몬스터 UI 프로필 Row를 찾는 함수
	const FNSMonsterUIData* FindMonsterUIData(const FGameplayTag& EnemyId) const;
	
	// 현재 스테이지의 스포너 DT가 필요할 때 한 번만 비동기 로드.
	// 이미 로드되어 있으면 즉시 완료 델리게이트를 호출.
	void LoadCurrentStageSpawnerTables();
	
	// 현재 스테이지에서 사용할 근접/원거리 스폰 테이블 캐시.
	// LoadCurrentStageSpawnerTables() 완료 이후 유효.
	UDataTable* GetCurrentMeleeSpawnerTable() const { return CurrentMeleeSpawnerTable.Get(); }
	UDataTable* GetCurrentRangeSpawnerTable() const { return CurrentRangeSpawnerTable.Get(); }
	bool AreCurrentStageSpawnerTablesLoaded() const { return bStageSpawnerTablesLoaded; }

	// DA PrimaryAssetId로 파츠 row 조회
	const FNSPartDefinitionRow* GetPartRow(const FPrimaryAssetId& DefId) const;

	// 필터 없이 전체 캐시 반환 (UI 목록 구성용)
	const TMap<FPrimaryAssetId, FNSPartDefinitionRow>& GetAllPartRows() const;

	// NodeId -> 공통 업그레이드 노드 Row 조회.
	const FNSCommonUpgradeNodeRow* GetCommonUpgradeNodeRow(FName NodeId) const;
	// 필터 없이 전체 캐시 반환 (업그레이드 콘솔 UI 노드 목록 구성용)
	const TMap<FName, FNSCommonUpgradeNodeRow>& GetAllCommonUpgradeNodeRows() const;

	const FNSPartSlotRow* GetSlotRow(FGameplayTag Slot) const;
	const TMap<FGameplayTag, FNSPartSlotRow>& GetAllSlotRows() const;

	// StatTag별 표시 이름/좋은 방향 테이블
	UDataTable* GetCommonStatDisplayInfoTable() const;
	const FNSStatDisplayInfoRow* FindStatDisplayInfoRow(const FGameplayTag& StatTag) const;

	// 등급별 파츠 업그레이드/상점 row 조회
	const FNSPartUpgradeRow* GetPartUpgradeRow(ENSPartRarity Rarity) const;
	const TMap<ENSPartRarity, FNSPartUpgradeRow>& GetAllPartUpgradeRows() const;

	// 드랍 파츠 튜닝값 조회
	const FNSDroppedPartConfigRow* GetDroppedPartConfigRow() const;

	//맵 이동 중 유지할 플레이어 진행 데이터 저장
	void SetCachedProgressPayload(const FNSProgressPayload& Payload);

	//저장된 플레이어 진행 데이터가 있으면 반환
	bool GetCachedProgressPayload(FNSProgressPayload& OutPayload) const;

	//저장된 플레이어 진행 데이터를 ProgressComponent에 적용
	void ApplyCachedProgressTo(class UNSPlayerProgressComponent* ProgressComponent) const;
	
	// ================================================================
	// 델리게이트
	// ================================================================

	UPROPERTY(BlueprintAssignable, Category = "NS|DataSubsystem")
	FOnNSCommonDataReady OnCommonDataReady;

	UPROPERTY(BlueprintAssignable, Category = "NS|DataSubsystem")
	FOnNSOutGameDataReady OnOutGameDataReady;

	UPROPERTY(BlueprintAssignable, Category = "NS|DataSubsystem")
	FOnNSRunGameDataReady OnRunGameDataReady;

	UPROPERTY(BlueprintAssignable, Category = "NS|DataSubsystem")
	FOnNSDataPhaseChanged OnPhaseChanged;
	
	// 현재 스테이지 스포너 DT 로드가 끝났음을 C++ 스포너들에게 알림.
	// 스포너는 이 알림 이후 GetCurrentMeleeSpawnerTable()/GetCurrentRangeSpawnerTable()을 사용할 수 있다.
	UPROPERTY(BlueprintAssignable, Category = "NS|DataSubsystem")
	FOnNSStageSpawnerTablesReady OnStageSpawnerTablesReady;

	// ================================================================
	// AssetType 상수 (Project Settings > Asset Manager 등록 이름과 일치)
	// ================================================================

	// Common (인런/아웃런 공통)
	static const FPrimaryAssetType CommonDataConfigAssetType;
	static const FPrimaryAssetType CharacterAssetType;

	// OutRun
	static const FPrimaryAssetType OutGameDataConfigAssetType;
	static const FPrimaryAssetType HubAssetType;
	static const FPrimaryAssetType PartAssetType;

	// InRun
	static const FPrimaryAssetType RunConfigAssetType;
	static const FPrimaryAssetType LevelConfigAssetType;
	static const FPrimaryAssetType MonsterAssetType;
	static const FPrimaryAssetType AugmentAssetType;
	static const FPrimaryAssetType AugmentRarityRuleSetAssetType;
	static const FPrimaryAssetType RewardTriggerAssetType;

private:
	// 로드 시 함께 끌어올 AssetBundle 목록 (DataAsset meta=(AssetBundles=...) 와 일치)
	static const TArray<FName> CommonBundles;
	static const TArray<FName> OutGameBundles;
	static const TArray<FName> RunBundles;

	// ================================================================
	// 내부 로드/언로드
	// ================================================================

	void StartLoadCommon();
	void OnCommonAssetsLoaded();

	// CommonDataConfig와 CharacterData PrimaryAsset 로드 후,
	// Row 내부 SoftObject까지 필요한 공용 참조 에셋을 추가로 로드.
	void StartLoadCommonReferenceAssets();

	// 공용 참조 에셋 로드가 끝난 뒤 런타임 조회용 캐시를 만들고 CommonReady를 알림.
	void OnCommonReferenceAssetsLoaded();

	// VFX DataTable Row가 가진 NiagaraSystem SoftObject를 수집.
	// VFX 재생 시 동기 로드를 피하기 위해 CommonDataReady 전에 선로드.
	void CollectVFXSystemPathsFromTable(const UDataTable* VFXTable, TArray<FSoftObjectPath>& OutPaths) const;

	// UIWidget DataTable Row가 가진 위젯 클래스 SoftClass를 수집.
	// CommonDataReady 이후 UIManager가 동기 로드 없이 캐시를 만들 수 있게 미리 로드.
	void CollectUIWidgetClassPathsFromTable(const UDataTable* UIWidgetTable, TArray<FSoftObjectPath>& OutPaths) const;

	// HUD UI DataTable Row 내부의 아이콘 SoftObject를 수집.
	// CommonDataReady 이후 HUD/Goods/Skill 위젯이 동기 로드 없이 아이콘을 사용할 수 있게 미리 로드.
	void CollectCommonUIIconPaths(
		const UDataTable* CharacterSkillUISetTable,
		const UDataTable* SkillUIDataTable,
		const UDataTable* GoodsUIDataTable,
		TArray<FSoftObjectPath>& OutPaths
	) const;

	// HitReaction/PlayerAttackFeedback 테이블을 매번 순회하지 않도록 CommonData 로드 시 1회 캐싱.
	void CacheCommonFeedbackRows();

	void StartLoadOutGame();
	// OutGame PrimaryAsset 로드 후, DT_CharacterList Row 내부 SoftObject까지 추가로 로드.
	// 캐릭터 선택 UI가 CharacterData/PreviewTexture를 동기 로드하지 않도록 OutGameReady 전에 끝냄.
	void StartLoadOutGameReferenceAssets();

	// OutGame 참조 에셋 로드가 끝난 뒤 캐릭터 선택 Row 캐시를 만들고 OutGameReady를 알림.
	void OnOutGameReferenceAssetsLoaded();
	void OnOutGameAssetsLoaded();

	// DT_CharacterList Row 내부 SoftReference를 수집.
	// 캐릭터 선택 UI에서 동기 로드가 발생하지 않도록 OutGameReady 전에 선로드.
	void CollectCharacterSelectPathsFromTable(
		const UDataTable* CharacterSelectTable,
		TArray<FSoftObjectPath>& OutPaths) const;

	// 캐릭터 선택 테이블을 매번 순회하지 않도록 OutGameData 로드 시 1회 캐싱.
	void CacheCharacterSelectRows();

	// RunConfig를 먼저 로드해 런 전체 유지 데이터를 준비한 뒤, 현재 스테이지 LevelConfig를 로드. 
	void StartLoadRunConfig();
	void OnRunConfigLoaded();
	void OnRunAssetsLoaded();
	void OnRunSlotTableLoaded();
	
	// 현재 스테이지에서만 필요한 LevelConfig와 번들 데이터를 로드.
	void StartLoadStageConfig();
	void OnStageConfigLoaded();
	
	// CurrentRunLevelConfig가 가진 스포너 Dt SoftPtr을 실제 UDataTable로 로드.
	void StartLoadStageSpawnerTables();
	void OnStageSpawnerTableLoaded();
	
	void BuildRewardDataRegistry();
	
	void UnloadCommon();
	void UnloadOutGame();
	void UnloadStage();
	void UnloadRun();
	void UnloadAll();

	// AssetType 목록의 PrimaryAssetId를 수집
	void GatherAssetIds(const TArray<FPrimaryAssetType>& Types, TArray<FPrimaryAssetId>& OutIds) const;
	
	// DT_AugmentDefinition이 참조하는 Definition DA를 NSAugmentData PrimaryAssetId로 수집
	 void CollectAugmentDefinitionIdsFromTable(
	 	const UDataTable* AugmentDefinitionTable, TArray<FPrimaryAssetId>& OutIds) const;
	
	// 로드된 PrimaryAsset들을 DataCache에 저장
	void CacheLoadedByIds(const TArray<FPrimaryAssetId>& Ids);
	void UnloadByIds(const TArray<FPrimaryAssetId>& Ids);
	void CacheLoaded(const TArray<FPrimaryAssetType>& Types);
	// 해당 타입의 캐시 엔트리 제거 및 UnloadPrimaryAssets
	void UnloadByTypes(const TArray<FPrimaryAssetType>& Types);

	void SetPhase(ENSDataLoadPhase NewPhase);
	
	//맵 이동 후 새 PlayerState에 다시 적용할 진행 데이터
	FNSProgressPayload CachedProgressPayload;

	bool bHasCachedProgressPayload = false;

	// DefId → row 캐시 (OnOutGameAssetsLoaded 시 빌드)
	TMap<FPrimaryAssetId, FNSPartDefinitionRow> CachedPartRowsByDefId;

	// NodeId -> row 캐시 (OnCommonAssetsLoaded 시 캐시 생성)
	TMap<FName, FNSCommonUpgradeNodeRow> CachedCommonUpgradeNodeRows;

	// Slot → row 캐시
	TMap<FGameplayTag, FNSPartSlotRow> CachedSlotRowsBySlot;

	// Rarity → row 캐시
	TMap<ENSPartRarity, FNSPartUpgradeRow> CachedUpgradeRowsByRarity;

	// 드랍 파츠 튜닝값 캐시
	FNSDroppedPartConfigRow CachedDroppedPartConfigRow;
	bool bDroppedPartConfigRowLoaded = false;

	void BuildPartRowCache();
	void BuildSlotRowCache();
	void BuildPartUpgradeRowCache();
	void BuildDroppedPartConfigCache();

	void CacheCommonUpgradeNodeRows();
	
	UFUNCTION()
	void HandlePreloadCommonReady();

	// ================================================================
	// 상태
	// ================================================================

	UPROPERTY()
	TMap<FPrimaryAssetId, TObjectPtr<UObject>> DataCache;
	
	TArray<FNSHitReactionData> CachedHitReactionRows;
	TArray<FNSPlayerAttackFeedbackData> CachedPlayerAttackFeedbackRows;

	TArray<FNSCharacterSelectData> CachedCharacterSelectRows;

	UPROPERTY(Transient)
	TObjectPtr<UNSRewardDataRegistry> RewardDataRegistry;

	// RunConfig에서 수집한 런 전체 유지 PrimaryAsset 목록.
	TArray<FPrimaryAssetId> PendingRunAssetIds;
	// 현재 스테이지 LevelConfig에서 수집한 스테이지 전용 PrimaryAsset 목록.
	TArray<FPrimaryAssetId> PendingStageAssetIds;

	// 이번 런 전체에서 유지할 데이터 설정입니다.
	TSoftObjectPtr<UNSRunConfig> PendingRunConfig;
	
	UPROPERTY(Transient)
	TObjectPtr<UNSRunConfig> CurrentRunConfig;
	
	// 현재 스테이지에서만 사용하는 LevelConfig입니다.
	TSoftObjectPtr<UNSLevelConfig> PendingStageLevelConfig;
	
	UPROPERTY(Transient)
	TObjectPtr<UNSLevelConfig> CurrentRunLevelConfig;
	
	// 현재 스테이지 안에서 여러 스포너가 재사용할 스폰 테이블 캐시.
	// UnloadStage()에서 해제되어 다음 스토에지로 넘어갈 때 교체.
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CurrentMeleeSpawnerTable;
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CurrentRangeSpawnerTable;
	
	// 현재 스테이지 스포너 DT 로드 요청이 완료됐는지 확인하는 플래그.
	// 특정 DT가 비어 있는 경우와 아직 로드하지 않은 경우를 구분하기 위해 사용.
	bool bStageSpawnerTablesLoaded = false;
	
	// 비동기 로드 핸들 관리
	TSharedPtr<FStreamableHandle> CommonHandle;
	TSharedPtr<FStreamableHandle> CommonReferencedAssetsHandle;
	TSharedPtr<FStreamableHandle> OutGameHandle;
	TSharedPtr<FStreamableHandle> OutGameReferencedAssetHandle;
	TSharedPtr<FStreamableHandle> RunConfigHandle;
	TSharedPtr<FStreamableHandle> RunHandle;
	TSharedPtr<FStreamableHandle> StageLevelConfigHandle;
	// 향후 스테이지 전용 보스/특수 몬스터 PrimaryAsset을 추가 로드할 때 사용할 핸들.
	TSharedPtr<FStreamableHandle> StageHandle;
	TSharedPtr<FStreamableHandle> StageSpawnerTableHandle;

	// 로드 페이즈 ENUM
	ENSDataLoadPhase CurrentPhase = ENSDataLoadPhase::NotStarted;
};

// ================================================================
// 템플릿 구현
// ================================================================

template<typename T>
T* UNSDataSubsystem::GetData(const FPrimaryAssetId& Id) const
{
	const TObjectPtr<UObject>* Found = DataCache.Find(Id);
	return Found ? Cast<T>(*Found) : nullptr;
}

template<typename T>
TArray<T*> UNSDataSubsystem::GetAllDataOfType(const FPrimaryAssetType& AssetType) const
{
	TArray<T*> Result;
	for (const auto& Pair : DataCache)
	{
		if (Pair.Key.PrimaryAssetType == AssetType)
		{
			if (T* Casted = Cast<T>(Pair.Value.Get()))
			{
				Result.Add(Casted);
			}
		}
	}
	return Result;
}
