// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NSBossArtilleryTypes.h"
#include "NSBossArtilleryComponent.generated.h"

class UNSEnemyPhaseComponent;
class UNSBossArtilleryPatternData;
class ANSBossArenaBounds;
class UNSEnemyTargetComponent;
class UNSEnemyThreatComponent;

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
};
