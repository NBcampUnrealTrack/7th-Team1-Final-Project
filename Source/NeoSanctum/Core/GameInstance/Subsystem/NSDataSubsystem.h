// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/StreamableManager.h"
#include "NeoSanctum/Core/PlayerState/NSProgressTypes.h"
#include "NSDataSubsystem.generated.h"

class UDataTable;
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

	// 거점지역 -> 인런 진입 시 호출 (OutGame 언로드 -> Run 로드)
	UFUNCTION(BlueprintCallable, Category = "NS|DataSubsystem")
	void EnterRun();
	
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

	// ================================================================
	// AssetType 상수 (Project Settings > Asset Manager 등록 이름과 일치)
	// ================================================================

	// Common (인런/아웃런 공통)
	static const FPrimaryAssetType PlayerAssetType;

	// OutGame
	static const FPrimaryAssetType HubAssetType;
	static const FPrimaryAssetType PartAssetType;

	// Run
	static const FPrimaryAssetType MonsterAssetType;
	static const FPrimaryAssetType AugmentAssetType;
	static const FPrimaryAssetType AugmentPoolAssetType;
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

	void StartLoadOutGame();
	void OnOutGameAssetsLoaded();

	void StartLoadRun();
	void OnRunAssetsLoaded();
	void BuildRewardDataRegistry();
	
	void UnloadCommon();
	void UnloadOutGame();
	void UnloadRun();
	void UnloadAll();

	// AssetType 목록의 PrimaryAssetId를 수집
	void GatherAssetIds(const TArray<FPrimaryAssetType>& Types, TArray<FPrimaryAssetId>& OutIds) const;
	
	// DT_AugmentDefinition이 참조하는 Definition DA를 NSAugmentData PrimaryAssetId로 수집
	 void CollectAugmentDefinitionIdsFromTable(
	 	const UDataTable* AugmentDefinitionTable, TArray<FPrimaryAssetId>& OutIds) const;
	
	// 로드된 PrimaryAsset들을 DataCache에 저장
	void CacheLoaded(const TArray<FPrimaryAssetType>& Types);
	// 해당 타입의 캐시 엔트리 제거 및 UnloadPrimaryAssets
	void UnloadByTypes(const TArray<FPrimaryAssetType>& Types);

	void SetPhase(ENSDataLoadPhase NewPhase);
	
	//맵 이동 후 새 PlayerState에 다시 적용할 진행 데이터
	FNSProgressPayload CachedProgressPayload;

	bool bHasCachedProgressPayload = false;

	// ================================================================
	// 상태
	// ================================================================

	UPROPERTY()
	TMap<FPrimaryAssetId, TObjectPtr<UObject>> DataCache;
	
	UPROPERTY(Transient)
	TObjectPtr<UNSRewardDataRegistry> RewardDataRegistry;

	// 비동기 로드 핸들 관리
	TSharedPtr<FStreamableHandle> CommonHandle;
	TSharedPtr<FStreamableHandle> OutGameHandle;
	TSharedPtr<FStreamableHandle> RunHandle;

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
