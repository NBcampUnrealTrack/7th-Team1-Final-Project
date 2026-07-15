// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NSBossArtilleryTypes.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NSBossArtilleryComponent.generated.h"

class UNSEnemyPhaseComponent;
class UNSBossArtilleryPatternData;
class ANSBossArenaBounds;
class UNSEnemyTargetComponent;
class UNSEnemyThreatComponent;
class UAbilitySystemComponent;
class UGameplayEffect;
class UNSEnemyCosmeticComponent;
class UNSEnemyPartComponent;
struct FNSCosmeticEventNetData;

// 포격 실행이 정상 완료됐을 때 실행 ID를 전달하는 델리게이트
DECLARE_MULTICAST_DELEGATE_OneParam(FNSBossArtilleryExecutionFinishedDelegate, int32);

// 포격 패턴을 선택할 때 필요한 현재 전투 상태
USTRUCT(BlueprintType)
struct FNSBossArtillerySelectionContext
{
	GENERATED_BODY()

	// 현재 보스에게 적용된 페이즈 태그
	UPROPERTY(BlueprintReadWrite, Category = "Artillery|Selection")
	FGameplayTag CurrentPhaseTag;

	// 현재 보스 전투에 참여 중인 유효 플레이어 수
	UPROPERTY(BlueprintReadWrite, Category = "Artillery|Selection", meta = (ClampMin = "0"))
	int32 CombatantCount = 0;

	// 이번 선택에서 허용할 최대 위험도. 0이면 위험도 제한을 사용하지 않음
	UPROPERTY(BlueprintReadWrite, Category = "Artillery|Selection", meta = (ClampMin = "0"))
	int32 MaxDangerScore = 0;

	// 페이즈 전환 등으로 일반 패턴 선택이 잠긴 상태인지 여부
	UPROPERTY(BlueprintReadWrite, Category = "Artillery|Selection")
	bool bPatternLocked = false;
};

// 포격 패턴 후보 하나의 선택 가능 여부와 최종 가중치
USTRUCT(BlueprintType)
struct FNSBossArtilleryPatternCandidate
{
	GENERATED_BODY()

	// 이 후보가 가리키는 포격 패턴 DataAsset
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Artillery|Selection")
	TObjectPtr<UNSBossArtilleryPatternData> PatternData = nullptr;

	// 이 후보가 나타내는 A~E 포격 패턴 식별자
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Artillery|Selection")
	ENSBossArtilleryPatternId PatternId = ENSBossArtilleryPatternId::None;

	// DataAsset에 설정된 기본 선택 가중치
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Artillery|Selection")
	float BaseWeight = 0.0f;

	// 최근 사용 이력과 반복 정책을 반영한 최종 선택 가중치
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Artillery|Selection")
	float FinalWeight = 0.0f;

	// 이번 선택에서 실제 후보로 사용할 수 있는지 여부
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Artillery|Selection")
	bool bSelectable = false;

	// 선택 후보에서 제외된 이유를 디버그용으로 보관
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Artillery|Selection")
	FString RejectReason;
};

// 서버에서 실행 중인 포격 실행 데이터와 타이머 상태를 보관하는 구조체
struct FNSBossArtilleryRuntimeExecution
{
	// 서버 피해 판정과 클라이언트 연출에 사용할 포격 실행 데이터 변수
	FNSBossArtilleryExecutionData ExecutionData;

	// 아직 폭발 처리가 끝나지 않은 포탄 수 변수
	int32 PendingShotCount = 0;

	// 포탄 폭발 예약에 사용 중인 타이머 핸들 목록 변수
	TArray<FTimerHandle> ExplosionTimerHandles;

	// 같은 실행 안에서 이미 피해를 받은 대상을 기억하는 변수
	TSet<TObjectKey<AActor>> DamagedTargets;

	// 포격 경고/발사 코스메틱 예약에 사용 중인 타이머 핸들 목록 변수
	TArray<FTimerHandle> PresentationTimerHandles;
};

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.15
 * 
 * 클래스 개요 : 보스 포격 패턴 DataAsset 목록을 관리하고 실행 가능한 패턴을 선택하는 컴포넌트
 * Phase, 전투 참여자 수, 반복 정책, 하드 쿨다운을 기준으로 후보 필터링과 가중 랜덤 선택을 담당
*/
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSBossArtilleryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSBossArtilleryComponent();

	// 컴포넌트 종료 시 포격 선택 런타임 상태를 정리하는 함수
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 전투 참여자 수를 기준으로 포격 패턴 선택 컨텍스트를 생성하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery")
	FNSBossArtillerySelectionContext MakeSelectionContext(int32 CombatantCount) const;

	// 현재 선택 컨텍스트에서 각 패턴의 선택 가능 여부와 최종 가중치를 수집하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery")
	void CollectPatternCandidates(
		const FNSBossArtillerySelectionContext& Context,
		TArray<FNSBossArtilleryPatternCandidate>& OutCandidates) const;

	// 현재 선택 컨텍스트에서 사용할 포격 패턴을 가중 랜덤으로 선택하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery")
	UNSBossArtilleryPatternData* SelectPattern(
		const FNSBossArtillerySelectionContext& Context,
		bool bRecordSelection = true);

	// 전투 참여자 수만 입력해 현재 페이즈 기준 포격 패턴을 선택하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery")
	UNSBossArtilleryPatternData* SelectPatternByCombatantCount(
		int32 CombatantCount,
		bool bRecordSelection = true);

	// 지정한 포격 패턴이 사용됐음을 최근 사용 이력과 하드 쿨다운에 기록하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery")
	void RecordPatternUsed(const UNSBossArtilleryPatternData* PatternData);

	// 포격 패턴 목록과 무관한 최근 사용 이력 및 쿨다운 상태를 초기화하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery")
	void ResetArtillerySelectionState();

	// 런타임에서 사용할 포격 패턴 DataAsset 목록을 교체하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery")
	void SetPatternDataAssets(const TArray<UNSBossArtilleryPatternData*>& InPatternDataAssets);

	// 현재 등록된 포격 패턴 DataAsset이 하나 이상 있는지 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Artillery")
	bool HasPatternData() const;

	// 최근 사용된 포격 패턴 ID 목록을 최신순으로 반환하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery")
	void GetRecentPatternHistory(TArray<ENSBossArtilleryPatternId>& OutPatternHistory) const;

	// 지정 패턴에 남아 있는 하드 쿨다운 선택 횟수를 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Artillery")
	int32 GetHardCooldownRemaining(ENSBossArtilleryPatternId PatternId) const;

	// 등록된 전투 참여자 수로 포격 패턴 선택 컨텍스트를 생성하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery")
	FNSBossArtillerySelectionContext MakeSelectionContextFromRegisteredCombatants() const;

	// 등록된 전투 참여자 수를 기준으로 포격 패턴을 선택하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery")
	UNSBossArtilleryPatternData* SelectPatternByRegisteredCombatants(bool bRecordSelection = true);

	// 현재 포격 시스템이 사용할 전투 참여자 목록을 교체하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Target")
	void SetRegisteredCombatants(const TArray<AActor*>& InCombatants);

	// 포격 시스템이 사용할 전투 참여자를 하나 추가하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Target")
	void RegisterCombatant(AActor* Combatant);

	// 포격 시스템이 사용하던 전투 참여자 하나를 제거하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Target")
	void UnregisterCombatant(AActor* Combatant);

	// 포격 시스템이 관리 중인 전투 참여자 목록을 모두 비우는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Target")
	void ClearRegisteredCombatants();

	// 현재 유효한 전투 참여자 목록을 반환하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Target")
	void CollectValidCombatants(TArray<AActor*>& OutCombatants) const;

	// 현재 유효한 전투 참여자 수를 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Artillery|Target")
	int32 GetRegisteredCombatantCount() const;

	// 파동 포격과 아레나 기준 포격에 사용할 보스룸 Bounds를 지정하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Target")
	void SetArenaBounds(ANSBossArenaBounds* InArenaBounds);

	// 선택된 포격 패턴의 TargetMode에 맞는 포격 기준점 목록을 수집하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Target")
	bool CollectTargetPointsForPattern(
		const UNSBossArtilleryPatternData* PatternData,
		TArray<FNSBossArtilleryTargetPoint>& OutTargetPoints) const;

	// 선택된 패턴과 기준점 목록으로 기준점별 포탄 배정 목록을 생성하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Shot Budget")
	bool BuildShotAllocationsForPattern(
		const UNSBossArtilleryPatternData* PatternData,
		const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
		TArray<FNSBossArtilleryShotAllocation>& OutShotAllocations,
		int32& OutTotalShotCount) const;

	// 선택된 패턴과 기준점 목록으로 MaxTotalShots 적용 전 요청 포탄 수를 계산하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Shot Budget")
	int32 CalculateRequestedShotCountForPattern(
		const UNSBossArtilleryPatternData* PatternData,
		const TArray<FNSBossArtilleryTargetPoint>& TargetPoints) const;

	// 선택된 패턴과 포탄 배정 목록으로 실제 착탄 위치 목록을 생성하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Placement")
	bool BuildImpactPointsForPattern(
		const UNSBossArtilleryPatternData* PatternData,
		const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
		TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const;

	// 선택된 패턴과 착탄 위치 목록으로 서버 기준 폭발 시간 목록을 생성하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Timing")
	bool BuildTimedShotsForPattern(
		const UNSBossArtilleryPatternData* PatternData,
		const TArray<FNSBossArtilleryImpactPoint>& ImpactPoints,
		float PatternStartServerTime,
		TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const;

	// 지정 패턴을 포격 실행 데이터로 변환하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Execution")
	bool BuildExecutionDataForPattern(
		const UNSBossArtilleryPatternData* PatternData,
		float PatternStartServerTime,
		FNSBossArtilleryExecutionData& OutExecutionData);

	// 등록된 전투 참여자 기준으로 패턴을 선택하고 포격 실행 데이터를 생성하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Execution")
	bool SelectAndBuildExecutionDataFromRegisteredCombatants(
		float PatternStartServerTime,
		FNSBossArtilleryExecutionData& OutExecutionData,
		bool bRecordSelection = true);

	// 현재 서버 시간을 시작 시간으로 사용해 포격 실행 데이터를 생성하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Execution")
	bool SelectAndBuildExecutionDataNowFromRegisteredCombatants(
		FNSBossArtilleryExecutionData& OutExecutionData,
		bool bRecordSelection = true);

	// 포격 실행 데이터에서 클라이언트 표시용 포탄 목록을 생성하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Execution")
	void BuildPresentationShotsFromExecutionData(
		const FNSBossArtilleryExecutionData& ExecutionData,
		TArray<FNSBossArtilleryPresentationShot>& OutPresentationShots) const;

	// 현재 월드의 서버 시간을 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Artillery|Execution")
	float GetCurrentServerTimeSeconds() const;

	// 등록된 전투 참여자 기준으로 포격 데이터를 만들고 즉시 실행하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Execution")
	bool ExecuteArtilleryFromRegisteredCombatants(bool bRecordSelection = true);

	// 이미 생성된 포격 실행 데이터를 서버 기준으로 실행하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Execution")
	bool ExecuteArtilleryExecutionData(const FNSBossArtilleryExecutionData& ExecutionData);

	// 현재 실행 중인 모든 포격 실행을 취소하고 타이머를 정리하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Execution")
	void CancelActiveArtilleryExecutions();

	// 현재 실행 중인 포격이 하나 이상 있는지 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Artillery|Execution")
	bool HasActiveArtilleryExecution() const;

	// 지정 포격 실행의 타이머와 런타임 상태를 취소하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Artillery|Execution")
	void CancelArtilleryExecution(int32 ExecutionId);

	// 포격 실행이 정상 완료됐을 때 C++ 구독자에게 알리는 델리게이트 변수
	FNSBossArtilleryExecutionFinishedDelegate OnArtilleryExecutionFinished;

private:
	// 지정 패턴이 현재 선택 컨텍스트에서 사용 가능한지 검사하는 함수
	bool CanUsePatternData(
		const UNSBossArtilleryPatternData& PatternData,
		const FNSBossArtillerySelectionContext& Context,
		FString& OutRejectReason) const;

	// 지정 패턴의 최근 사용 이력 감점까지 반영한 최종 가중치를 계산하는 함수
	float ComputeFinalWeight(const UNSBossArtilleryPatternData& PatternData) const;

	// 지정 패턴이 현재 페이즈 태그에서 허용되는지 검사하는 함수
	bool IsPatternAllowedByPhase(
		const UNSBossArtilleryPatternData& PatternData,
		const FGameplayTag& CurrentPhaseTag) const;

	// 지정 패턴이 직전 반복 금지 정책에 의해 막혔는지 검사하는 함수
	bool IsImmediateRepeatBlocked(const UNSBossArtilleryPatternData& PatternData) const;

	// 지정 패턴이 하드 쿨다운 정책에 의해 막혔는지 검사하는 함수
	bool IsBlockedByHardCooldown(const UNSBossArtilleryPatternData& PatternData) const;

	// 지정 패턴이 최근 사용 이력에서 몇 번째 위치에 있는지 반환하는 함수
	int32 FindMostRecentPatternIndex(ENSBossArtilleryPatternId PatternId) const;

	// 지정 패턴의 최근 사용 위치에 맞는 가중치 배율을 반환하는 함수
	float GetRecentUseMultiplier(const UNSBossArtilleryPatternData& PatternData) const;

	// 모든 하드 쿨다운 카운트를 선택 1회만큼 감소시키는 함수
	void TickHardCooldowns();

	// 최근 사용 패턴 이력의 맨 앞에 새 패턴 ID를 추가하는 함수
	void PushRecentPattern(ENSBossArtilleryPatternId PatternId);

	// Owner가 가진 페이즈 컴포넌트를 반환하는 함수
	UNSEnemyPhaseComponent* GetPhaseComponent() const;

	// AllCombatants 대상 모드의 기준점 목록을 수집하는 함수
	bool CollectAllCombatantTargetPoints(TArray<FNSBossArtilleryTargetPoint>& OutTargetPoints) const;

	// HighestThreat 대상 모드의 기준점 목록을 수집하는 함수
	bool CollectHighestThreatTargetPoint(TArray<FNSBossArtilleryTargetPoint>& OutTargetPoints) const;

	// ArenaCenter 대상 모드의 기준점 목록을 수집하는 함수
	bool CollectArenaCenterTargetPoint(TArray<FNSBossArtilleryTargetPoint>& OutTargetPoints) const;

	// BossLocation 대상 모드의 기준점 목록을 수집하는 함수
	bool CollectBossLocationTargetPoint(TArray<FNSBossArtilleryTargetPoint>& OutTargetPoints) const;

	// BetweenCombatants 대상 모드의 기준점 목록을 수집하는 함수
	bool CollectBetweenCombatantTargetPoints(
		const FNSBossArtilleryTargetData& TargetData,
		TArray<FNSBossArtilleryTargetPoint>& OutTargetPoints) const;

	// 지정 Actor가 포격 대상 기준점으로 사용할 수 있는 유효 전투 참여자인지 확인하는 함수
	bool IsValidCombatant(const AActor* Combatant) const;

	// 단일 Actor 기준점을 생성하는 함수
	FNSBossArtilleryTargetPoint MakeActorTargetPoint(AActor* TargetActor) const;

	// 두 Actor 사이의 중간 기준점을 생성하는 함수
	FNSBossArtilleryTargetPoint MakePairTargetPoint(AActor* FirstTarget, AActor* SecondTarget) const;

	// 위치 기반 기준점을 생성하는 함수
	FNSBossArtilleryTargetPoint MakeLocationTargetPoint(
		ENSBossArtilleryTargetPointType PointType,
		const FVector& Location) const;

	// 보스룸 중심 위치를 반환하는 함수
	FVector GetArenaCenterLocation() const;

	// Owner가 가진 타깃 검증 컴포넌트를 반환하는 함수
	UNSEnemyTargetComponent* GetTargetComponent() const;

	// Owner가 가진 Threat 컴포넌트를 반환하는 함수
	UNSEnemyThreatComponent* GetThreatComponent() const;

	// PerTarget 발수 계산 방식으로 기준점별 포탄 배정을 생성하는 함수
	bool BuildPerTargetShotAllocations(
		const FNSBossArtilleryShotBudgetData& ShotBudgetData,
		const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
		TArray<FNSBossArtilleryShotAllocation>& OutShotAllocations,
		int32& OutTotalShotCount) const;

	// FixedTotal 발수 계산 방식으로 기준점별 포탄 배정을 생성하는 함수
	bool BuildFixedTotalShotAllocations(
		const FNSBossArtilleryShotBudgetData& ShotBudgetData,
		const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
		TArray<FNSBossArtilleryShotAllocation>& OutShotAllocations,
		int32& OutTotalShotCount) const;

	// PerRing 발수 계산 방식으로 기준점별 포탄 배정을 생성하는 함수
	bool BuildPerRingShotAllocations(
		const FNSBossArtilleryShotBudgetData& ShotBudgetData,
		const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
		TArray<FNSBossArtilleryShotAllocation>& OutShotAllocations,
		int32& OutTotalShotCount) const;

	// BetweenCombatants 발수 계산 방식으로 기준점별 포탄 배정을 생성하는 함수
	bool BuildBetweenCombatantShotAllocations(
		const FNSBossArtilleryShotBudgetData& ShotBudgetData,
		const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
		TArray<FNSBossArtilleryShotAllocation>& OutShotAllocations,
		int32& OutTotalShotCount) const;

	// 기준점별 희망 포탄 수와 MaxTotalShots를 이용해 최종 포탄 배정을 생성하는 함수
	bool BuildShotAllocationsFromDesiredCounts(
		const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
		const TArray<int32>& DesiredShotCounts,
		int32 MaxTotalShots,
		TArray<FNSBossArtilleryShotAllocation>& OutShotAllocations,
		int32& OutTotalShotCount) const;

	// 고정 전체 포탄 수를 유효 기준점에 순환 배분하는 함수
	bool BuildFixedShotAllocationsFromTotal(
		const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
		int32 RequestedTotalShots,
		int32 MaxTotalShots,
		TArray<FNSBossArtilleryShotAllocation>& OutShotAllocations,
		int32& OutTotalShotCount) const;

	// 지정 포격 기준점이 포탄 배정에 사용할 수 있는지 확인하는 함수
	bool IsValidTargetPoint(const FNSBossArtilleryTargetPoint& TargetPoint) const;

	// 입력 기준점 목록에서 유효한 기준점만 수집하는 함수
	void CollectValidTargetPoints(
		const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
		TArray<FNSBossArtilleryTargetPoint>& OutValidTargetPoints) const;

	// MaxTotalShots 설정을 최소 1 이상으로 보정해 반환하는 함수
	int32 GetMaxTotalShotCount(const FNSBossArtilleryShotBudgetData& ShotBudgetData) const;


	// TargetCurrent 배치 방식으로 착탄 위치를 생성하는 함수
	bool BuildTargetCurrentImpactPoints(
		const FNSBossArtilleryPlacementData& PlacementData,
		const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
		TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const;

	// TargetPrediction 배치 방식으로 착탄 위치를 생성하는 함수
	bool BuildTargetPredictionImpactPoints(
		const FNSBossArtilleryPlacementData& PlacementData,
		const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
		TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const;

	// RandomAroundTarget 배치 방식으로 착탄 위치를 생성하는 함수
	bool BuildRandomAroundTargetImpactPoints(
		const FNSBossArtilleryPlacementData& PlacementData,
		const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
		TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const;

	// ClusterAroundTarget 배치 방식으로 착탄 위치를 생성하는 함수
	bool BuildClusterAroundTargetImpactPoints(
		const FNSBossArtilleryPlacementData& PlacementData,
		const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
		TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const;

	// BetweenTargets 배치 방식으로 착탄 위치를 생성하는 함수
	bool BuildBetweenTargetsImpactPoints(
		const FNSBossArtilleryPlacementData& PlacementData,
		const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
		TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const;

	// Ring 배치 방식으로 착탄 위치를 생성하는 함수
	bool BuildRingImpactPoints(
		const FNSBossArtilleryPlacementData& PlacementData,
		const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
		TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const;

	// WaveRings 배치 방식으로 착탄 위치를 생성하는 함수
	bool BuildWaveRingImpactPoints(
		const FNSBossArtilleryShotBudgetData& ShotBudgetData,
		const FNSBossArtilleryPlacementData& PlacementData,
		const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
		TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const;

	// EscapeRouteBlock 배치 방식으로 착탄 위치를 생성하는 함수
	bool BuildEscapeRouteBlockImpactPoints(
		const FNSBossArtilleryPlacementData& PlacementData,
		const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
		TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const;

	// 지정 위치와 메타데이터로 착탄 위치 데이터를 추가하는 함수
	void AddImpactPoint(
		const FNSBossArtilleryTargetPoint& SourceTargetPoint,
		const FVector& ImpactLocation,
		int32 GlobalShotIndex,
		int32 LocalShotIndex,
		int32 RingIndex,
		TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const;

	// 지정 기준점의 최신 월드 위치를 반환하는 함수
	FVector ResolveTargetPointLocation(const FNSBossArtilleryTargetPoint& TargetPoint) const;

	// 지정 기준점의 예측 월드 위치를 반환하는 함수
	FVector ResolvePredictedTargetPointLocation(
		const FNSBossArtilleryTargetPoint& TargetPoint,
		float PredictionTime) const;

	// 지정 기준 위치 주변의 랜덤 원형 오프셋 위치를 반환하는 함수
	FVector MakeRandomPointAroundLocation(const FVector& BaseLocation, float Radius) const;

	// 지정 기준 위치와 방향으로 이동 경로 차단 위치를 반환하는 함수
	FVector MakeEscapeRouteBlockLocation(
		const FNSBossArtilleryTargetPoint& TargetPoint,
		int32 LocalShotIndex,
		const FNSBossArtilleryPlacementData& PlacementData) const;

	// 착탄 위치를 보스룸 Bounds 안으로 보정하는 함수
	FVector ClampImpactLocationToArena(const FVector& ImpactLocation) const;

	// 착탄 위치가 NaN이 아니고 포격에 사용할 수 있는지 확인하는 함수
	bool IsValidImpactLocation(const FVector& ImpactLocation) const;

	// 착탄 위치 생성 결과가 기존 착탄 위치와 너무 가까운지 확인하는 함수
	bool IsImpactLocationTooClose(
		const FVector& CandidateLocation,
		const TArray<FNSBossArtilleryImpactPoint>& ExistingImpactPoints,
		float MinDistance) const;

	// Sequential 타이밍 방식으로 포탄별 폭발 시간을 생성하는 함수
	bool BuildSequentialTimedShots(
		const FNSBossArtilleryTimingData& TimingData,
		const TArray<FNSBossArtilleryImpactPoint>& ImpactPoints,
		float PatternStartServerTime,
		TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const;

	// RandomScatter 타이밍 방식으로 포탄별 폭발 시간을 생성하는 함수
	bool BuildRandomScatterTimedShots(
		const FNSBossArtilleryTimingData& TimingData,
		const TArray<FNSBossArtilleryImpactPoint>& ImpactPoints,
		float PatternStartServerTime,
		TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const;

	// Simultaneous 타이밍 방식으로 포탄별 폭발 시간을 생성하는 함수
	bool BuildSimultaneousTimedShots(
		const FNSBossArtilleryTimingData& TimingData,
		const TArray<FNSBossArtilleryImpactPoint>& ImpactPoints,
		float PatternStartServerTime,
		TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const;

	// Burst 타이밍 방식으로 포탄별 폭발 시간을 생성하는 함수
	bool BuildBurstTimedShots(
		const FNSBossArtilleryTimingData& TimingData,
		const TArray<FNSBossArtilleryImpactPoint>& ImpactPoints,
		float PatternStartServerTime,
		TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const;

	// Wave 타이밍 방식으로 포탄별 폭발 시간을 생성하는 함수
	bool BuildWaveTimedShots(
		const FNSBossArtilleryTimingData& TimingData,
		const TArray<FNSBossArtilleryImpactPoint>& ImpactPoints,
		float PatternStartServerTime,
		TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const;

	// OffBeat 타이밍 방식으로 포탄별 폭발 시간을 생성하는 함수
	bool BuildOffBeatTimedShots(
		const FNSBossArtilleryTimingData& TimingData,
		const TArray<FNSBossArtilleryImpactPoint>& ImpactPoints,
		float PatternStartServerTime,
		TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const;

	// 지정 착탄 위치와 지연 시간으로 서버 기준 포탄 시간 데이터를 추가하는 함수
	void AddTimedShot(
		const FNSBossArtilleryImpactPoint& ImpactPoint,
		float PatternStartServerTime,
		float ExplosionDelayFromStart,
		TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const;

	// 생성된 시간 데이터 목록을 폭발 서버 시간 기준으로 정렬하는 함수
	void SortTimedShotsByExplosionTime(TArray<FNSBossArtilleryTimedShot>& InOutTimedShots) const;

	// 타이밍 계산에서 사용할 경고 시간을 0 이상으로 보정하는 함수
	float GetClampedWarningDuration(const FNSBossArtilleryTimingData& TimingData) const;

	// 새로운 포격 실행 ID를 발급하는 함수
	int32 AllocateArtilleryExecutionId();

	// 시간 적용 포탄 목록으로 포격 실행 데이터의 시간 범위를 채우는 함수
	void FillExecutionTimeRange(FNSBossArtilleryExecutionData& InOutExecutionData) const;

	// 포격 실행 데이터의 포탄 폭발 타이머를 예약하는 함수
	void ScheduleExecutionDamage(const FNSBossArtilleryExecutionData& ExecutionData);

	// 지정 포격 실행의 지정 포탄 폭발을 서버에서 처리하는 함수
	void HandleTimedShotExplosion(int32 ExecutionId, int32 TimedShotIndex);

	// 지정 포탄의 범위 피해를 서버에서 적용하는 함수
	void ApplyTimedShotDamage(
		const FNSBossArtilleryExecutionData& ExecutionData,
		const FNSBossArtilleryTimedShot& TimedShot,
		TSet<TObjectKey<AActor>>& InOutDamagedTargets) const;

	// 착탄 위치 주변에서 피해를 받을 수 있는 대상을 수집하는 함수
	void CollectDamageTargetsAtImpact(
		const FNSBossArtilleryExecutionData& ExecutionData,
		const FNSBossArtilleryTimedShot& TimedShot,
		TArray<AActor*>& OutTargets) const;

	// 지정 대상이 착탄 위치에서 시야 차폐 없이 피해를 받을 수 있는지 확인하는 함수
	bool HasDamageLineOfSight(
		const FVector& ImpactLocation,
		AActor* TargetActor) const;

	// 포격 피해 GameplayEffect를 지정 대상에게 적용하는 함수
	bool ApplyArtilleryDamageToTarget(
		AActor* TargetActor,
		const FNSBossArtilleryExecutionData& ExecutionData,
		const FNSBossArtilleryTimedShot& TimedShot) const;

	// 포격 피해량을 Source ASC와 포격 피해 설정으로 계산하는 함수
	float CalculateArtilleryDamage(const FNSBossArtilleryDamageData& DamageData) const;

	// 지정 Actor의 피해 판정 기준 위치를 반환하는 함수
	FVector GetDamageCheckLocation(const AActor* TargetActor) const;

	// Owner의 AbilitySystemComponent를 반환하는 함수
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;

	// 지정 Actor의 AbilitySystemComponent를 반환하는 함수
	UAbilitySystemComponent* GetTargetAbilitySystemComponent(AActor* TargetActor) const;

	// 포탄 표시 데이터에서 지정 인덱스에 해당하는 항목을 찾는 함수
	bool TryGetPresentationShotByGlobalShotIndex(
		const FNSBossArtilleryExecutionData& ExecutionData,
		int32 GlobalShotIndex,
		FNSBossArtilleryPresentationShot& OutPresentationShot) const;

	// 지정 포격 실행의 남은 포탄 수가 0이면 실행 상태를 정리하는 함수
	void FinishExecutionIfComplete(int32 ExecutionId);

	// 지정 포격 실행의 타이머와 런타임 상태를 제거하는 함수
	void RemoveActiveExecution(int32 ExecutionId, bool bBroadcastFinished);

	// 포격 실행 데이터의 경고/발사 코스메틱 이벤트 타이머를 예약하는 함수
	void ScheduleExecutionPresentation(const FNSBossArtilleryExecutionData& ExecutionData);

	// 지정 포탄의 경고/발사 코스메틱 이벤트를 서버에서 전송하는 함수
	void HandlePresentationShotWarning(int32 ExecutionId, int32 PresentationShotIndex);

	// 기존 Bombard Launch 코스메틱 이벤트를 전송하는 함수
	void SendArtilleryLaunchCosmeticEvent(const FNSBossArtilleryPresentationShot& PresentationShot) const;

	// 기존 Bombard Warning 코스메틱 이벤트를 전송하는 함수
	void SendArtilleryWarningCosmeticEvent(const FNSBossArtilleryPresentationShot& PresentationShot) const;

	// 기존 Bombard Impact 코스메틱 이벤트를 전송하는 함수
	void SendArtilleryImpactCosmeticEvent(const FNSBossArtilleryPresentationShot& PresentationShot) const;

	// EnemyCosmeticComponent를 통해 코스메틱 이벤트를 전송하는 함수
	void SendArtilleryCosmeticEvent(const FNSCosmeticEventNetData& EventData, bool bReliable) const;

	// Owner가 가진 EnemyCosmeticComponent를 반환하는 함수
	UNSEnemyCosmeticComponent* GetEnemyCosmeticComponent() const;

	// Owner가 가진 EnemyPartComponent를 반환하는 함수
	UNSEnemyPartComponent* GetEnemyPartComponent() const;

	// 포격 발사 연출에 사용할 머즐 위치를 계산하는 함수
	FVector ResolveArtilleryMuzzleLocation(const FNSBossArtilleryPresentationShot& PresentationShot) const;

private:
	// 이 보스가 사용할 수 있는 포격 패턴 DataAsset 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Artillery|Patterns",
		meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UNSBossArtilleryPatternData>> PatternDataAssets;

	// 최근 사용한 패턴을 몇 개까지 기억할지 정함
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Artillery|Selection",
		meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 RecentPatternHistorySize = 3;

	// 이번 선택에서 허용할 최대 위험도. 0이면 위험도 제한을 사용하지 않음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Artillery|Selection",
		meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 MaxDangerScoreForSelection = 0;

	// 포격 패턴 선택을 서버에서만 허용할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Artillery|Network",
		meta = (AllowPrivateAccess = "true"))
	bool bServerOnlySelection = true;

	// 최근 사용한 포격 패턴 ID 목록. 0번이 가장 최근
	UPROPERTY(Transient)
	TArray<ENSBossArtilleryPatternId> RecentPatternHistory;

	// 패턴별로 남아 있는 하드 쿨다운 선택 횟수
	UPROPERTY(Transient)
	TMap<ENSBossArtilleryPatternId, int32> HardCooldownRemainingByPattern;

	// 현재 보스 포격 시스템이 대상으로 삼을 수 있는 전투 참여자 목록
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> RegisteredCombatants;

	// 아레나 중심 기준 포격에 사용할 보스룸 Bounds
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Boss|Artillery|Target",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ANSBossArenaBounds> ArenaBounds;

	// 등록 전투 참여자가 없을 때 ThreatComponent의 KnownTargets를 임시 fallback으로 사용할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Artillery|Target",
		meta = (AllowPrivateAccess = "true"))
	bool bUseThreatKnownTargetsWhenNoRegisteredCombatants = false;

	// 생성된 착탄 위치를 보스룸 Bounds 안으로 보정할지 정하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Artillery|Placement",
		meta = (AllowPrivateAccess = "true"))
	bool bClampImpactLocationsToArenaBounds = true;

	// 착탄 위치 중복 방지를 시도할 최대 횟수를 정하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Artillery|Placement",
		meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MaxPlacementRetryCount = 8;

	// 다음 포격 실행에 부여할 런타임 실행 ID 변수
	UPROPERTY(Transient)
	int32 NextArtilleryExecutionId = 1;

	// 포격 피해를 적용할 GameplayEffect 클래스 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Artillery|Damage",
		meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 포격 범위 피해 대상 수집에 사용할 충돌 채널 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Artillery|Damage",
		meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<ECollisionChannel> DamageOverlapChannel = NSCollisionChannels::ExplosionTrace;

	// 포격 시야 차폐 검사에 사용할 충돌 채널 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Artillery|Damage",
		meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<ECollisionChannel> DamageLineOfSightChannel = NSCollisionChannels::ExplosionTrace;

	// 현재 서버에서 실행 중인 포격 목록 변수
	TMap<int32, FNSBossArtilleryRuntimeExecution> ActiveExecutions;
};
