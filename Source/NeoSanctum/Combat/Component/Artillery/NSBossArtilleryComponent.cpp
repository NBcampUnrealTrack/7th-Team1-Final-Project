// Copyright 2026 One Team. All rights reserved.

#include "NSBossArtilleryComponent.h"

#include "GameFramework/Actor.h"
#include "NeoSanctum/AI/Enemy/HelperActor/NSBossArenaBounds.h"
#include "NeoSanctum/Combat/Component/NSEnemyTargetComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Data/AI/NSBossArtilleryPatternData.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "NeoSanctum/Combat/NSDamageRules.h"
#include "NeoSanctum/Combat/Component/NSEnemyPartComponent.h"
#include "NeoSanctum/Combat/Cosmetic/NSEnemyCosmeticComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"	
#include "NeoSanctum/Type/NSCosmeticEventTypes.h"

UNSBossArtilleryComponent::UNSBossArtilleryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UNSBossArtilleryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelActiveArtilleryExecutions();
	ResetArtillerySelectionState();
	ClearRegisteredCombatants();

	Super::EndPlay(EndPlayReason);
}

FNSBossArtillerySelectionContext UNSBossArtilleryComponent::MakeSelectionContext(int32 CombatantCount) const
{
	// 이번 선택에 사용할 전투 상태를 담는 컨텍스트
	FNSBossArtillerySelectionContext Context;

	// 외부에서 전달된 전투 참여자 수를 0 이상으로 보정
	Context.CombatantCount = FMath::Max(CombatantCount, 0);

	// 컴포넌트 기본 설정에 있는 위험도 제한을 컨텍스트에 복사
	Context.MaxDangerScore = MaxDangerScoreForSelection;

	if (const UNSEnemyPhaseComponent* PhaseComponent = GetPhaseComponent())
	{
		// 현재 보스 페이즈 태그를 패턴 필터 조건으로 사용
		Context.CurrentPhaseTag = PhaseComponent->GetCurrentPhaseTag();

		// 페이즈 전환 중 패턴 잠금 상태를 선택 차단 조건으로 사용
		Context.bPatternLocked = PhaseComponent->IsPatternLocked();
	}

	return Context;
}

void UNSBossArtilleryComponent::CollectPatternCandidates(
	const FNSBossArtillerySelectionContext& Context,
	TArray<FNSBossArtilleryPatternCandidate>& OutCandidates) const
{
	// 이전 호출 결과가 섞이지 않도록 후보 배열을 초기화
	OutCandidates.Reset();

	for (UNSBossArtilleryPatternData* PatternData : PatternDataAssets)
	{
		// 후보 하나의 선택 상태와 디버그 정보를 담는 구조체
		FNSBossArtilleryPatternCandidate Candidate;

		Candidate.PatternData = PatternData;

		if (!IsValid(PatternData))
		{
			Candidate.PatternId = ENSBossArtilleryPatternId::None;
			Candidate.BaseWeight = 0.0f;
			Candidate.FinalWeight = 0.0f;
			Candidate.bSelectable = false;
			Candidate.RejectReason = TEXT("PatternData가 유효하지 않습니다.");
			OutCandidates.Add(Candidate);
			continue;
		}

		Candidate.PatternId = PatternData->PatternId;
		Candidate.BaseWeight = PatternData->SelectionData.BaseWeight;

		// 현재 조건에서 패턴이 사용 가능한지 검사한 결과를 담음
		FString RejectReason;

		if (!CanUsePatternData(*PatternData, Context, RejectReason))
		{
			Candidate.FinalWeight = 0.0f;
			Candidate.bSelectable = false;
			Candidate.RejectReason = RejectReason;
			OutCandidates.Add(Candidate);
			continue;
		}

		Candidate.FinalWeight = ComputeFinalWeight(*PatternData);

		if (Candidate.FinalWeight <= 0.0f)
		{
			Candidate.bSelectable = false;
			Candidate.RejectReason = TEXT("최종 가중치가 0 이하입니다.");
			OutCandidates.Add(Candidate);
			continue;
		}

		Candidate.bSelectable = true;
		Candidate.RejectReason.Reset();
		OutCandidates.Add(Candidate);
	}
}

UNSBossArtilleryPatternData* UNSBossArtilleryComponent::SelectPattern(
	const FNSBossArtillerySelectionContext& Context,
	bool bRecordSelection)
{
	if (bServerOnlySelection)
	{
		// 포격 패턴 선택은 서버가 결정해야 하므로 클라이언트 호출을 차단
		const AActor* OwnerActor = GetOwner();
		if (OwnerActor && !OwnerActor->HasAuthority())
		{
			return nullptr;
		}
	}

	// 이번 선택에서 고려할 모든 후보 정보를 수집
	TArray<FNSBossArtilleryPatternCandidate> Candidates;
	CollectPatternCandidates(Context, Candidates);

	// 선택 가능한 후보들의 가중치 총합
	float TotalWeight = 0.0f;

	for (const FNSBossArtilleryPatternCandidate& Candidate : Candidates)
	{
		if (Candidate.bSelectable)
		{
			TotalWeight += Candidate.FinalWeight;
		}
	}

	if (TotalWeight <= 0.0f)
	{
		return nullptr;
	}

	// 가중치 총합 안에서 이번 선택 지점을 무작위로 고름
	const float Pick = FMath::FRandRange(0.0f, TotalWeight);

	// 현재까지 누적된 후보 가중치
	float AccumulatedWeight = 0.0f;

	// 부동소수점 경계 상황에서 사용할 마지막 유효 후보
	UNSBossArtilleryPatternData* FallbackPattern = nullptr;

	for (const FNSBossArtilleryPatternCandidate& Candidate : Candidates)
	{
		if (!Candidate.bSelectable || !IsValid(Candidate.PatternData))
		{
			continue;
		}

		FallbackPattern = Candidate.PatternData;
		AccumulatedWeight += Candidate.FinalWeight;

		if (Pick <= AccumulatedWeight)
		{
			if (bRecordSelection)
			{
				RecordPatternUsed(Candidate.PatternData);
			}

			return Candidate.PatternData;
		}
	}

	if (FallbackPattern && bRecordSelection)
	{
		RecordPatternUsed(FallbackPattern);
	}

	return FallbackPattern;
}

UNSBossArtilleryPatternData* UNSBossArtilleryComponent::SelectPatternByCombatantCount(
	int32 CombatantCount,
	bool bRecordSelection)
{
	// 전투 참여자 수만으로 현재 보스 상태가 반영된 선택 컨텍스트를 만듦
	const FNSBossArtillerySelectionContext Context = MakeSelectionContext(CombatantCount);

	return SelectPattern(Context, bRecordSelection);
}

void UNSBossArtilleryComponent::RecordPatternUsed(const UNSBossArtilleryPatternData* PatternData)
{
	if (!IsValid(PatternData))
	{
		return;
	}

	if (PatternData->PatternId == ENSBossArtilleryPatternId::None)
	{
		return;
	}

	// 다른 패턴을 한 번 선택했으므로 기존 하드 쿨다운을 한 단계 감소
	TickHardCooldowns();

	// 이번에 선택한 패턴을 최근 사용 이력 맨 앞에 기록
	PushRecentPattern(PatternData->PatternId);

	if (PatternData->SelectionData.RepeatPolicy == ENSBossArtilleryRepeatPolicy::HardCooldownUses &&
		PatternData->SelectionData.HardCooldownUseCount > 0)
	{
		// 하드 쿨다운 정책인 패턴은 지정 횟수만큼 이후 선택에서 제외
		HardCooldownRemainingByPattern.Add(
			PatternData->PatternId,
			PatternData->SelectionData.HardCooldownUseCount);
	}
}

void UNSBossArtilleryComponent::ResetArtillerySelectionState()
{
	// 최근 사용 패턴 이력을 모두 비움
	RecentPatternHistory.Reset();

	// 패턴별 하드 쿨다운 상태를 모두 비움
	HardCooldownRemainingByPattern.Reset();
}

void UNSBossArtilleryComponent::SetPatternDataAssets(
	const TArray<UNSBossArtilleryPatternData*>& InPatternDataAssets)
{
	// 기존 패턴 목록을 새 입력으로 교체하기 위해 초기화
	PatternDataAssets.Reset();

	for (UNSBossArtilleryPatternData* PatternData : InPatternDataAssets)
	{
		if (IsValid(PatternData))
		{
			PatternDataAssets.Add(PatternData);
		}
	}
}

bool UNSBossArtilleryComponent::HasPatternData() const
{
	// 유효한 DataAsset이 하나라도 있으면 true를 반환함
	for (const UNSBossArtilleryPatternData* PatternData : PatternDataAssets)
	{
		if (IsValid(PatternData))
		{
			return true;
		}
	}

	return false;
}

void UNSBossArtilleryComponent::GetRecentPatternHistory(
	TArray<ENSBossArtilleryPatternId>& OutPatternHistory) const
{
	// 외부에서 최근 사용 기록을 읽을 수 있도록 복사
	OutPatternHistory = RecentPatternHistory;
}

int32 UNSBossArtilleryComponent::GetHardCooldownRemaining(
	ENSBossArtilleryPatternId PatternId) const
{
	// 등록된 하드 쿨다운 값이 없으면 남은 쿨다운이 없는 것으로 봄
	const int32* RemainingCount = HardCooldownRemainingByPattern.Find(PatternId);

	return RemainingCount ? FMath::Max(*RemainingCount, 0) : 0;
}

FNSBossArtillerySelectionContext UNSBossArtilleryComponent::MakeSelectionContextFromRegisteredCombatants() const
{
	return MakeSelectionContext(GetRegisteredCombatantCount());
}

UNSBossArtilleryPatternData* UNSBossArtilleryComponent::SelectPatternByRegisteredCombatants(
	bool bRecordSelection)
{
	const FNSBossArtillerySelectionContext Context = MakeSelectionContextFromRegisteredCombatants();

	return SelectPattern(Context, bRecordSelection);
}

void UNSBossArtilleryComponent::SetRegisteredCombatants(const TArray<AActor*>& InCombatants)
{
	RegisteredCombatants.Reset();

	for (AActor* Combatant : InCombatants)
	{
		RegisterCombatant(Combatant);
	}
}

void UNSBossArtilleryComponent::RegisterCombatant(AActor* Combatant)
{
	if (!IsValidCombatant(Combatant))
	{
		return;
	}

	for (const TWeakObjectPtr<AActor>& RegisteredCombatant : RegisteredCombatants)
	{
		if (RegisteredCombatant.Get() == Combatant)
		{
			return;
		}
	}

	RegisteredCombatants.Add(Combatant);
}

void UNSBossArtilleryComponent::UnregisterCombatant(AActor* Combatant)
{
	RegisteredCombatants.RemoveAll(
		[Combatant](const TWeakObjectPtr<AActor>& RegisteredCombatant)
		{
			return !RegisteredCombatant.IsValid() || RegisteredCombatant.Get() == Combatant;
		});
}

void UNSBossArtilleryComponent::ClearRegisteredCombatants()
{
	RegisteredCombatants.Reset();
}

void UNSBossArtilleryComponent::CollectValidCombatants(TArray<AActor*>& OutCombatants) const
{
	OutCombatants.Reset();

	for (const TWeakObjectPtr<AActor>& RegisteredCombatant : RegisteredCombatants)
	{
		AActor* Combatant = RegisteredCombatant.Get();

		if (IsValidCombatant(Combatant))
		{
			OutCombatants.AddUnique(Combatant);
		}
	}

	if (!OutCombatants.IsEmpty() || !bUseThreatKnownTargetsWhenNoRegisteredCombatants)
	{
		return;
	}

	if (const UNSEnemyThreatComponent* ThreatComponent = GetThreatComponent())
	{
		TArray<AActor*> KnownTargets;
		ThreatComponent->GetKnownTargets(KnownTargets, false);

		for (AActor* KnownTarget : KnownTargets)
		{
			if (IsValidCombatant(KnownTarget))
			{
				OutCombatants.AddUnique(KnownTarget);
			}
		}
	}
}

int32 UNSBossArtilleryComponent::GetRegisteredCombatantCount() const
{
	TArray<AActor*> ValidCombatants;
	CollectValidCombatants(ValidCombatants);

	return ValidCombatants.Num();
}

void UNSBossArtilleryComponent::SetArenaBounds(ANSBossArenaBounds* InArenaBounds)
{
	ArenaBounds = InArenaBounds;
}

bool UNSBossArtilleryComponent::CollectTargetPointsForPattern(
	const UNSBossArtilleryPatternData* PatternData,
	TArray<FNSBossArtilleryTargetPoint>& OutTargetPoints) const
{
	OutTargetPoints.Reset();

	if (!IsValid(PatternData))
	{
		return false;
	}

	switch (PatternData->TargetData.TargetMode)
	{
	case ENSBossArtilleryTargetMode::AllCombatants:
		return CollectAllCombatantTargetPoints(OutTargetPoints);

	case ENSBossArtilleryTargetMode::HighestThreat:
		return CollectHighestThreatTargetPoint(OutTargetPoints);

	case ENSBossArtilleryTargetMode::ArenaCenter:
		return CollectArenaCenterTargetPoint(OutTargetPoints);

	case ENSBossArtilleryTargetMode::BossLocation:
		return CollectBossLocationTargetPoint(OutTargetPoints);

	case ENSBossArtilleryTargetMode::BetweenCombatants:
		return CollectBetweenCombatantTargetPoints(PatternData->TargetData, OutTargetPoints);

	default:
		return false;
	}
}

bool UNSBossArtilleryComponent::BuildShotAllocationsForPattern(
	const UNSBossArtilleryPatternData* PatternData,
	const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
	TArray<FNSBossArtilleryShotAllocation>& OutShotAllocations,
	int32& OutTotalShotCount) const
{
	OutShotAllocations.Reset();
	OutTotalShotCount = 0;

	if (!IsValid(PatternData))
	{
		return false;
	}

	switch (PatternData->ShotBudgetData.BudgetMode)
	{
	case ENSBossArtilleryShotBudgetMode::PerTarget:
		return BuildPerTargetShotAllocations(
			PatternData->ShotBudgetData,
			TargetPoints,
			OutShotAllocations,
			OutTotalShotCount);

	case ENSBossArtilleryShotBudgetMode::FixedTotal:
		return BuildFixedTotalShotAllocations(
			PatternData->ShotBudgetData,
			TargetPoints,
			OutShotAllocations,
			OutTotalShotCount);

	case ENSBossArtilleryShotBudgetMode::PerRing:
		return BuildPerRingShotAllocations(
			PatternData->ShotBudgetData,
			TargetPoints,
			OutShotAllocations,
			OutTotalShotCount);

	case ENSBossArtilleryShotBudgetMode::BetweenCombatants:
		return BuildBetweenCombatantShotAllocations(
			PatternData->ShotBudgetData,
			TargetPoints,
			OutShotAllocations,
			OutTotalShotCount);

	default:
		return false;
	}
}

int32 UNSBossArtilleryComponent::CalculateRequestedShotCountForPattern(
	const UNSBossArtilleryPatternData* PatternData,
	const TArray<FNSBossArtilleryTargetPoint>& TargetPoints) const
{
	if (!IsValid(PatternData))
	{
		return 0;
	}

	TArray<FNSBossArtilleryTargetPoint> ValidTargetPoints;
	CollectValidTargetPoints(TargetPoints, ValidTargetPoints);

	switch (PatternData->ShotBudgetData.BudgetMode)
	{
	case ENSBossArtilleryShotBudgetMode::PerTarget:
		return ValidTargetPoints.Num() * FMath::Max(PatternData->ShotBudgetData.ShotsPerTarget, 0);

	case ENSBossArtilleryShotBudgetMode::FixedTotal:
		return FMath::Max(PatternData->ShotBudgetData.FixedTotalShots, 0);

	case ENSBossArtilleryShotBudgetMode::PerRing:
		return FMath::Max(PatternData->ShotBudgetData.RingCount, 0) *
			FMath::Max(PatternData->ShotBudgetData.ShotsPerRing, 0);

	case ENSBossArtilleryShotBudgetMode::BetweenCombatants:
		return ValidTargetPoints.Num() * FMath::Max(PatternData->ShotBudgetData.ShotsPerPair, 0);

	default:
		return 0;
	}
}

bool UNSBossArtilleryComponent::BuildImpactPointsForPattern(
	const UNSBossArtilleryPatternData* PatternData,
	const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
	TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const
{
	OutImpactPoints.Reset();

	if (!IsValid(PatternData))
	{
		return false;
	}

	switch (PatternData->PlacementData.PlacementMode)
	{
	case ENSBossArtilleryPlacementMode::TargetCurrent:
		return BuildTargetCurrentImpactPoints(
			PatternData->PlacementData,
			ShotAllocations,
			OutImpactPoints);

	case ENSBossArtilleryPlacementMode::TargetPrediction:
		return BuildTargetPredictionImpactPoints(
			PatternData->PlacementData,
			ShotAllocations,
			OutImpactPoints);

	case ENSBossArtilleryPlacementMode::RandomAroundTarget:
		return BuildRandomAroundTargetImpactPoints(
			PatternData->PlacementData,
			ShotAllocations,
			OutImpactPoints);

	case ENSBossArtilleryPlacementMode::ClusterAroundTarget:
		return BuildClusterAroundTargetImpactPoints(
			PatternData->PlacementData,
			ShotAllocations,
			OutImpactPoints);

	case ENSBossArtilleryPlacementMode::BetweenTargets:
		return BuildBetweenTargetsImpactPoints(
			PatternData->PlacementData,
			ShotAllocations,
			OutImpactPoints);

	case ENSBossArtilleryPlacementMode::Ring:
		return BuildRingImpactPoints(
			PatternData->PlacementData,
			ShotAllocations,
			OutImpactPoints);

	case ENSBossArtilleryPlacementMode::WaveRings:
		return BuildWaveRingImpactPoints(
			PatternData->ShotBudgetData,
			PatternData->PlacementData,
			ShotAllocations,
			OutImpactPoints);

	case ENSBossArtilleryPlacementMode::EscapeRouteBlock:
		return BuildEscapeRouteBlockImpactPoints(
			PatternData->PlacementData,
			ShotAllocations,
			OutImpactPoints);

	default:
		return false;
	}
}

bool UNSBossArtilleryComponent::BuildTimedShotsForPattern(
	const UNSBossArtilleryPatternData* PatternData,
	const TArray<FNSBossArtilleryImpactPoint>& ImpactPoints,
	float PatternStartServerTime,
	TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const
{
	OutTimedShots.Reset();

	if (!IsValid(PatternData) || ImpactPoints.IsEmpty())
	{
		return false;
	}

	bool bBuilt = false;

	switch (PatternData->TimingData.TimingMode)
	{
	case ENSBossArtilleryTimingMode::Sequential:
		bBuilt = BuildSequentialTimedShots(
			PatternData->TimingData,
			ImpactPoints,
			PatternStartServerTime,
			OutTimedShots);
		break;

	case ENSBossArtilleryTimingMode::RandomScatter:
		bBuilt = BuildRandomScatterTimedShots(
			PatternData->TimingData,
			ImpactPoints,
			PatternStartServerTime,
			OutTimedShots);
		break;

	case ENSBossArtilleryTimingMode::Simultaneous:
		bBuilt = BuildSimultaneousTimedShots(
			PatternData->TimingData,
			ImpactPoints,
			PatternStartServerTime,
			OutTimedShots);
		break;

	case ENSBossArtilleryTimingMode::Burst:
		bBuilt = BuildBurstTimedShots(
			PatternData->TimingData,
			ImpactPoints,
			PatternStartServerTime,
			OutTimedShots);
		break;

	case ENSBossArtilleryTimingMode::Wave:
		bBuilt = BuildWaveTimedShots(
			PatternData->TimingData,
			ImpactPoints,
			PatternStartServerTime,
			OutTimedShots);
		break;

	case ENSBossArtilleryTimingMode::OffBeat:
		bBuilt = BuildOffBeatTimedShots(
			PatternData->TimingData,
			ImpactPoints,
			PatternStartServerTime,
			OutTimedShots);
		break;

	default:
		return false;
	}

	if (bBuilt)
	{
		SortTimedShotsByExplosionTime(OutTimedShots);
	}

	return bBuilt && !OutTimedShots.IsEmpty();
}

bool UNSBossArtilleryComponent::BuildExecutionDataForPattern(
	const UNSBossArtilleryPatternData* PatternData,
	float PatternStartServerTime,
	FNSBossArtilleryExecutionData& OutExecutionData)
{
	OutExecutionData = FNSBossArtilleryExecutionData();

	if (!IsValid(PatternData))
	{
		return false;
	}

	TArray<FNSBossArtilleryTargetPoint> TargetPoints;
	if (!CollectTargetPointsForPattern(PatternData, TargetPoints))
	{
		return false;
	}

	TArray<FNSBossArtilleryShotAllocation> ShotAllocations;
	int32 TotalShotCount = 0;

	if (!BuildShotAllocationsForPattern(
		PatternData,
		TargetPoints,
		ShotAllocations,
		TotalShotCount))
	{
		return false;
	}

	TArray<FNSBossArtilleryImpactPoint> ImpactPoints;
	if (!BuildImpactPointsForPattern(
		PatternData,
		ShotAllocations,
		ImpactPoints))
	{
		return false;
	}

	TArray<FNSBossArtilleryTimedShot> TimedShots;
	if (!BuildTimedShotsForPattern(
		PatternData,
		ImpactPoints,
		PatternStartServerTime,
		TimedShots))
	{
		return false;
	}

	OutExecutionData.ExecutionId = AllocateArtilleryExecutionId();
	OutExecutionData.PatternId = PatternData->PatternId;
	OutExecutionData.PatternStartServerTime = PatternStartServerTime;
	OutExecutionData.TotalShotCount = TimedShots.Num();
	OutExecutionData.DamageData = PatternData->DamageData;
	OutExecutionData.DebugData = PatternData->DebugData;
	OutExecutionData.TimedShots = MoveTemp(TimedShots);

	FillExecutionTimeRange(OutExecutionData);
	BuildPresentationShotsFromExecutionData(
		OutExecutionData,
		OutExecutionData.PresentationShots);

	return OutExecutionData.TotalShotCount > 0;
}

bool UNSBossArtilleryComponent::SelectAndBuildExecutionDataFromRegisteredCombatants(
	float PatternStartServerTime,
	FNSBossArtilleryExecutionData& OutExecutionData,
	bool bRecordSelection)
{
	OutExecutionData = FNSBossArtilleryExecutionData();

	const FNSBossArtillerySelectionContext Context =
		MakeSelectionContextFromRegisteredCombatants();

	UNSBossArtilleryPatternData* SelectedPattern =
		SelectPattern(Context, false);

	if (!IsValid(SelectedPattern))
	{
		return false;
	}

	if (!BuildExecutionDataForPattern(
		SelectedPattern,
		PatternStartServerTime,
		OutExecutionData))
	{
		return false;
	}

	if (bRecordSelection)
	{
		RecordPatternUsed(SelectedPattern);
	}

	return true;
}

bool UNSBossArtilleryComponent::SelectAndBuildExecutionDataNowFromRegisteredCombatants(
	FNSBossArtilleryExecutionData& OutExecutionData,
	bool bRecordSelection)
{
	return SelectAndBuildExecutionDataFromRegisteredCombatants(
		GetCurrentServerTimeSeconds(),
		OutExecutionData,
		bRecordSelection);
}

void UNSBossArtilleryComponent::BuildPresentationShotsFromExecutionData(
	const FNSBossArtilleryExecutionData& ExecutionData,
	TArray<FNSBossArtilleryPresentationShot>& OutPresentationShots) const
{
	OutPresentationShots.Reset();
	OutPresentationShots.Reserve(ExecutionData.TimedShots.Num());

	for (const FNSBossArtilleryTimedShot& TimedShot : ExecutionData.TimedShots)
	{
		if (!IsValidImpactLocation(TimedShot.ImpactPoint.ImpactLocation))
		{
			continue;
		}

		FNSBossArtilleryPresentationShot PresentationShot;
		PresentationShot.ExecutionId = ExecutionData.ExecutionId;
		PresentationShot.PatternId = ExecutionData.PatternId;
		PresentationShot.GlobalShotIndex = TimedShot.ImpactPoint.GlobalShotIndex;
		PresentationShot.RingIndex = TimedShot.ImpactPoint.RingIndex;
		PresentationShot.ImpactLocation = TimedShot.ImpactPoint.ImpactLocation;
		PresentationShot.WarningStartServerTime = TimedShot.WarningStartServerTime;
		PresentationShot.ExplosionServerTime = TimedShot.ExplosionServerTime;
		PresentationShot.DamageRadius = ExecutionData.DamageData.DamageRadius;

		OutPresentationShots.Add(PresentationShot);
	}
}

float UNSBossArtilleryComponent::GetCurrentServerTimeSeconds() const
{
	const UWorld* World = GetWorld();

	return World ? World->GetTimeSeconds() : 0.0f;
}

bool UNSBossArtilleryComponent::ExecuteArtilleryFromRegisteredCombatants(bool bRecordSelection)
{
	FNSBossArtilleryExecutionData ExecutionData;

	if (!SelectAndBuildExecutionDataNowFromRegisteredCombatants(
		ExecutionData,
		bRecordSelection))
	{
		return false;
	}

	return ExecuteArtilleryExecutionData(ExecutionData);
}

bool UNSBossArtilleryComponent::ExecuteArtilleryExecutionData(
	const FNSBossArtilleryExecutionData& ExecutionData)
{
	if (!GetWorld())
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();

	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return false;
	}

	if (ExecutionData.ExecutionId <= 0 ||
		ExecutionData.TimedShots.IsEmpty() ||
		ExecutionData.PresentationShots.IsEmpty())
	{
		return false;
	}

	if (ActiveExecutions.Contains(ExecutionData.ExecutionId))
	{
		return false;
	}

	FNSBossArtilleryRuntimeExecution RuntimeExecution;
	RuntimeExecution.ExecutionData = ExecutionData;
	RuntimeExecution.PendingShotCount = ExecutionData.TimedShots.Num();

	ActiveExecutions.Add(ExecutionData.ExecutionId, MoveTemp(RuntimeExecution));

	ScheduleExecutionPresentation(ExecutionData);
	ScheduleExecutionDamage(ExecutionData);

	return true;
}

void UNSBossArtilleryComponent::CancelActiveArtilleryExecutions()
{
	UWorld* World = GetWorld();

	for (TPair<int32, FNSBossArtilleryRuntimeExecution>& Pair : ActiveExecutions)
	{
		if (!World)
		{
			continue;
		}

		for (FTimerHandle& TimerHandle : Pair.Value.ExplosionTimerHandles)
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
		}

		for (FTimerHandle& TimerHandle : Pair.Value.PresentationTimerHandles)
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
		}
	}

	ActiveExecutions.Reset();
}

bool UNSBossArtilleryComponent::HasActiveArtilleryExecution() const
{
	return !ActiveExecutions.IsEmpty();
}

void UNSBossArtilleryComponent::CancelArtilleryExecution(int32 ExecutionId)
{
	RemoveActiveExecution(ExecutionId, false);
}

void UNSBossArtilleryComponent::ScheduleExecutionDamage(
	const FNSBossArtilleryExecutionData& ExecutionData)
{
	UWorld* World = GetWorld();

	if (!World)
	{
		RemoveActiveExecution(ExecutionData.ExecutionId, false);
		return;
	}

	FNSBossArtilleryRuntimeExecution* RuntimeExecution =
		ActiveExecutions.Find(ExecutionData.ExecutionId);

	if (!RuntimeExecution)
	{
		return;
	}

	const float CurrentServerTime = GetCurrentServerTimeSeconds();
	TArray<int32> ImmediateShotIndices;

	for (int32 TimedShotIndex = 0; TimedShotIndex < ExecutionData.TimedShots.Num(); ++TimedShotIndex)
	{
		const FNSBossArtilleryTimedShot& TimedShot = ExecutionData.TimedShots[TimedShotIndex];
		const float Delay = TimedShot.ExplosionServerTime - CurrentServerTime;

		if (Delay <= 0.0f)
		{
			ImmediateShotIndices.Add(TimedShotIndex);
			continue;
		}

		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(
			this,
			&ThisClass::HandleTimedShotExplosion,
			ExecutionData.ExecutionId,
			TimedShotIndex);

		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(
			TimerHandle,
			TimerDelegate,
			Delay,
			false);

		RuntimeExecution->ExplosionTimerHandles.Add(TimerHandle);
	}

	for (const int32 TimedShotIndex : ImmediateShotIndices)
	{
		if (!ActiveExecutions.Contains(ExecutionData.ExecutionId))
		{
			break;
		}

		HandleTimedShotExplosion(ExecutionData.ExecutionId, TimedShotIndex);
	}
}

void UNSBossArtilleryComponent::HandleTimedShotExplosion(
	int32 ExecutionId,
	int32 TimedShotIndex)
{
	FNSBossArtilleryRuntimeExecution* RuntimeExecution =
		ActiveExecutions.Find(ExecutionId);

	if (!RuntimeExecution)
	{
		return;
	}

	if (!RuntimeExecution->ExecutionData.TimedShots.IsValidIndex(TimedShotIndex))
	{
		RuntimeExecution->PendingShotCount =
			FMath::Max(RuntimeExecution->PendingShotCount - 1, 0);

		FinishExecutionIfComplete(ExecutionId);
		return;
	}

	const FNSBossArtilleryTimedShot& TimedShot =
		RuntimeExecution->ExecutionData.TimedShots[TimedShotIndex];

	ApplyTimedShotDamage(
		RuntimeExecution->ExecutionData,
		TimedShot,
		RuntimeExecution->DamagedTargets);

	FNSBossArtilleryPresentationShot PresentationShot;
	if (TryGetPresentationShotByGlobalShotIndex(
		RuntimeExecution->ExecutionData,
		TimedShot.ImpactPoint.GlobalShotIndex,
		PresentationShot))
	{
		SendArtilleryImpactCosmeticEvent(PresentationShot);
	}

	RuntimeExecution->PendingShotCount =
		FMath::Max(RuntimeExecution->PendingShotCount - 1, 0);

	FinishExecutionIfComplete(ExecutionId);
}

void UNSBossArtilleryComponent::ApplyTimedShotDamage(
	const FNSBossArtilleryExecutionData& ExecutionData,
	const FNSBossArtilleryTimedShot& TimedShot,
	TSet<TObjectKey<AActor>>& InOutDamagedTargets) const
{
	TArray<AActor*> DamageTargets;
	CollectDamageTargetsAtImpact(
		ExecutionData,
		TimedShot,
		DamageTargets);

	for (AActor* TargetActor : DamageTargets)
	{
		if (!IsValid(TargetActor))
		{
			continue;
		}

		const TObjectKey<AActor> TargetKey(TargetActor);

		if (!ExecutionData.DamageData.bAllowMultipleHitsPerTarget &&
			InOutDamagedTargets.Contains(TargetKey))
		{
			continue;
		}

		if (ApplyArtilleryDamageToTarget(
			TargetActor,
			ExecutionData,
			TimedShot))
		{
			InOutDamagedTargets.Add(TargetKey);
		}
	}
}

void UNSBossArtilleryComponent::CollectDamageTargetsAtImpact(
	const FNSBossArtilleryExecutionData& ExecutionData,
	const FNSBossArtilleryTimedShot& TimedShot,
	TArray<AActor*>& OutTargets) const
{
	OutTargets.Reset();

	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();

	if (!World || !IsValid(OwnerActor))
	{
		return;
	}

	const float DamageRadius = FMath::Max(ExecutionData.DamageData.DamageRadius, 0.0f);

	if (DamageRadius <= 0.0f)
	{
		return;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	TArray<FOverlapResult> Overlaps;
	const bool bHasOverlap = World->OverlapMultiByChannel(
		Overlaps,
		TimedShot.ImpactPoint.ImpactLocation,
		FQuat::Identity,
		DamageOverlapChannel,
		FCollisionShape::MakeSphere(DamageRadius),
		QueryParams);

	if (!bHasOverlap)
	{
		return;
	}

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();

		if (!NSDamageRules::CanApplyDamage(OwnerActor, TargetActor))
		{
			continue;
		}

		if (ExecutionData.DamageData.bRequireLineOfSight &&
			!HasDamageLineOfSight(
				TimedShot.ImpactPoint.ImpactLocation,
				TargetActor))
		{
			continue;
		}

		OutTargets.AddUnique(TargetActor);
	}
}

bool UNSBossArtilleryComponent::HasDamageLineOfSight(
	const FVector& ImpactLocation,
	AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();

	if (!World)
	{
		return false;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	const FVector TargetLocation = GetDamageCheckLocation(TargetActor);

	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		ImpactLocation + FVector::UpVector * 30.0f,
		TargetLocation,
		DamageLineOfSightChannel,
		QueryParams);

	if (!bHit)
	{
		return true;
	}

	AActor* HitActor = HitResult.GetActor();

	return HitActor == TargetActor ||
		(IsValid(HitActor) && HitActor->IsAttachedTo(TargetActor));
}

bool UNSBossArtilleryComponent::ApplyArtilleryDamageToTarget(
	AActor* TargetActor,
	const FNSBossArtilleryExecutionData& ExecutionData,
	const FNSBossArtilleryTimedShot& TimedShot) const
{
	AActor* OwnerActor = GetOwner();
	UAbilitySystemComponent* SourceASC = GetOwnerAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = GetTargetAbilitySystemComponent(TargetActor);

	if (!IsValid(OwnerActor) ||
		!IsValid(SourceASC) ||
		!IsValid(TargetASC) ||
		!DamageEffectClass)
	{
		return false;
	}

	FHitResult HitResult;
	HitResult.Location = TimedShot.ImpactPoint.ImpactLocation;
	HitResult.ImpactPoint = TimedShot.ImpactPoint.ImpactLocation;
	HitResult.TraceStart = OwnerActor->GetActorLocation();
	HitResult.TraceEnd = TimedShot.ImpactPoint.ImpactLocation;

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(OwnerActor);
	EffectContext.AddHitResult(HitResult);

	FGameplayEffectSpecHandle SpecHandle =
		SourceASC->MakeOutgoingSpec(
			DamageEffectClass,
			1.0f,
			EffectContext);

	if (!SpecHandle.IsValid())
	{
		return false;
	}

	const float Damage = CalculateArtilleryDamage(ExecutionData.DamageData);

	SpecHandle.Data->SetSetByCallerMagnitude(
		NSGameplayTags::Effect_Damage_Base,
		Damage);

	SourceASC->ApplyGameplayEffectSpecToTarget(
		*SpecHandle.Data.Get(),
		TargetASC);

	return true;
}

float UNSBossArtilleryComponent::CalculateArtilleryDamage(
	const FNSBossArtilleryDamageData& DamageData) const
{
	const UAbilitySystemComponent* SourceASC = GetOwnerAbilitySystemComponent();

	if (!SourceASC)
	{
		return 0.0f;
	}

	const float SourceBaseDamage =
		SourceASC->GetNumericAttribute(
			UNSBaseAttributeSet::GetBaseDamageAttribute());

	return FMath::Max(SourceBaseDamage * DamageData.DamageScale, 0.0f);
}

FVector UNSBossArtilleryComponent::GetDamageCheckLocation(
	const AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return FVector::ZeroVector;
	}

	if (const UPrimitiveComponent* PrimitiveComponent =
		Cast<UPrimitiveComponent>(TargetActor->GetRootComponent()))
	{
		return PrimitiveComponent->Bounds.Origin;
	}

	return TargetActor->GetActorLocation();
}

UAbilitySystemComponent* UNSBossArtilleryComponent::GetOwnerAbilitySystemComponent() const
{
	const IAbilitySystemInterface* AbilitySystemInterface =
		Cast<IAbilitySystemInterface>(GetOwner());

	return AbilitySystemInterface
		       ? AbilitySystemInterface->GetAbilitySystemComponent()
		       : nullptr;
}

UAbilitySystemComponent* UNSBossArtilleryComponent::GetTargetAbilitySystemComponent(
	AActor* TargetActor) const
{
	const IAbilitySystemInterface* AbilitySystemInterface =
		Cast<IAbilitySystemInterface>(TargetActor);

	return AbilitySystemInterface
		       ? AbilitySystemInterface->GetAbilitySystemComponent()
		       : nullptr;
}

bool UNSBossArtilleryComponent::TryGetPresentationShotByGlobalShotIndex(
	const FNSBossArtilleryExecutionData& ExecutionData,
	int32 GlobalShotIndex,
	FNSBossArtilleryPresentationShot& OutPresentationShot) const
{
	for (const FNSBossArtilleryPresentationShot& PresentationShot : ExecutionData.PresentationShots)
	{
		if (PresentationShot.GlobalShotIndex == GlobalShotIndex)
		{
			OutPresentationShot = PresentationShot;
			return true;
		}
	}

	return false;
}

void UNSBossArtilleryComponent::FinishExecutionIfComplete(int32 ExecutionId)
{
	const FNSBossArtilleryRuntimeExecution* RuntimeExecution =
		ActiveExecutions.Find(ExecutionId);

	if (!RuntimeExecution)
	{
		return;
	}

	if (RuntimeExecution->PendingShotCount > 0)
	{
		return;
	}

	RemoveActiveExecution(ExecutionId, true);
}

void UNSBossArtilleryComponent::RemoveActiveExecution(int32 ExecutionId, bool bBroadcastFinished)
{
	UWorld* World = GetWorld();

	FNSBossArtilleryRuntimeExecution* RuntimeExecution =
		ActiveExecutions.Find(ExecutionId);

	if (!RuntimeExecution)
	{
		return;
	}

	if (World)
	{
		for (FTimerHandle& TimerHandle : RuntimeExecution->ExplosionTimerHandles)
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
		}

		for (FTimerHandle& TimerHandle : RuntimeExecution->PresentationTimerHandles)
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
		}
	}

	ActiveExecutions.Remove(ExecutionId);

	if (bBroadcastFinished)
	{
		OnArtilleryExecutionFinished.Broadcast(ExecutionId);
	}
}

void UNSBossArtilleryComponent::ScheduleExecutionPresentation(
	const FNSBossArtilleryExecutionData& ExecutionData)
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	FNSBossArtilleryRuntimeExecution* RuntimeExecution =
		ActiveExecutions.Find(ExecutionData.ExecutionId);

	if (!RuntimeExecution)
	{
		return;
	}

	const float CurrentServerTime = GetCurrentServerTimeSeconds();
	TArray<int32> ImmediatePresentationIndices;

	for (int32 PresentationShotIndex = 0; PresentationShotIndex < ExecutionData.PresentationShots.Num(); ++
	     PresentationShotIndex)
	{
		const FNSBossArtilleryPresentationShot& PresentationShot =
			ExecutionData.PresentationShots[PresentationShotIndex];

		const float Delay = PresentationShot.WarningStartServerTime - CurrentServerTime;

		if (Delay <= 0.0f)
		{
			ImmediatePresentationIndices.Add(PresentationShotIndex);
			continue;
		}

		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(
			this,
			&ThisClass::HandlePresentationShotWarning,
			ExecutionData.ExecutionId,
			PresentationShotIndex);

		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(
			TimerHandle,
			TimerDelegate,
			Delay,
			false);

		RuntimeExecution->PresentationTimerHandles.Add(TimerHandle);
	}

	for (const int32 PresentationShotIndex : ImmediatePresentationIndices)
	{
		if (!ActiveExecutions.Contains(ExecutionData.ExecutionId))
		{
			break;
		}

		HandlePresentationShotWarning(ExecutionData.ExecutionId, PresentationShotIndex);
	}
}

void UNSBossArtilleryComponent::HandlePresentationShotWarning(
	int32 ExecutionId,
	int32 PresentationShotIndex)
{
	FNSBossArtilleryRuntimeExecution* RuntimeExecution =
		ActiveExecutions.Find(ExecutionId);

	if (!RuntimeExecution)
	{
		return;
	}

	if (!RuntimeExecution->ExecutionData.PresentationShots.IsValidIndex(PresentationShotIndex))
	{
		return;
	}

	const FNSBossArtilleryPresentationShot& PresentationShot =
		RuntimeExecution->ExecutionData.PresentationShots[PresentationShotIndex];

	if (PresentationShot.ExplosionServerTime <= GetCurrentServerTimeSeconds())
	{
		return;
	}

	SendArtilleryLaunchCosmeticEvent(PresentationShot);
	SendArtilleryWarningCosmeticEvent(PresentationShot);
}

void UNSBossArtilleryComponent::SendArtilleryLaunchCosmeticEvent(
	const FNSBossArtilleryPresentationShot& PresentationShot) const
{
	const FVector MuzzleLocation = ResolveArtilleryMuzzleLocation(PresentationShot);
	const FVector ImpactLocation = PresentationShot.ImpactLocation;
	const FVector Direction = (ImpactLocation - MuzzleLocation).GetSafeNormal();

	FNSCosmeticEventNetData EventData;
	EventData.EventTag = NSGameplayTags::Cosmetic_Enemy_TitanWalker_Bombard_Launch;
	EventData.Phase = ENSCosmeticEventPhase::OneShot;
	EventData.Location = MuzzleLocation;
	EventData.EndLocation = ImpactLocation;
	EventData.Direction = Direction;
	EventData.Range = FVector::Dist(MuzzleLocation, ImpactLocation);
	EventData.Duration = FMath::Max(PresentationShot.ExplosionServerTime - GetCurrentServerTimeSeconds(), 0.0f);

	SendArtilleryCosmeticEvent(EventData, true);
}

void UNSBossArtilleryComponent::SendArtilleryWarningCosmeticEvent(
	const FNSBossArtilleryPresentationShot& PresentationShot) const
{
	FNSCosmeticEventNetData EventData;
	EventData.EventTag = NSGameplayTags::Cosmetic_Enemy_TitanWalker_Bombard_Warning;
	EventData.Phase = ENSCosmeticEventPhase::OneShot;
	EventData.Location = PresentationShot.ImpactLocation;
	EventData.Direction = FVector::UpVector;
	EventData.Radius = PresentationShot.DamageRadius;
	EventData.Duration = FMath::Max(PresentationShot.ExplosionServerTime - GetCurrentServerTimeSeconds(), 0.01f);

	SendArtilleryCosmeticEvent(EventData, true);
}

void UNSBossArtilleryComponent::SendArtilleryImpactCosmeticEvent(
	const FNSBossArtilleryPresentationShot& PresentationShot) const
{
	FNSCosmeticEventNetData EventData;
	EventData.EventTag = NSGameplayTags::Cosmetic_Enemy_TitanWalker_Bombard_Impact;
	EventData.Phase = ENSCosmeticEventPhase::OneShot;
	EventData.Location = PresentationShot.ImpactLocation;
	EventData.Direction = FVector::UpVector;
	EventData.Radius = PresentationShot.DamageRadius;

	SendArtilleryCosmeticEvent(EventData, true);
}

void UNSBossArtilleryComponent::SendArtilleryCosmeticEvent(
	const FNSCosmeticEventNetData& EventData,
	bool bReliable) const
{
	if (UNSEnemyCosmeticComponent* CosmeticComponent = GetEnemyCosmeticComponent())
	{
		CosmeticComponent->SendCosmeticEvent(EventData, bReliable);
	}
}

UNSEnemyCosmeticComponent* UNSBossArtilleryComponent::GetEnemyCosmeticComponent() const
{
	const AActor* OwnerActor = GetOwner();

	return OwnerActor
		       ? OwnerActor->FindComponentByClass<UNSEnemyCosmeticComponent>()
		       : nullptr;
}

UNSEnemyPartComponent* UNSBossArtilleryComponent::GetEnemyPartComponent() const
{
	const AActor* OwnerActor = GetOwner();

	return OwnerActor
		       ? OwnerActor->FindComponentByClass<UNSEnemyPartComponent>()
		       : nullptr;
}

FVector UNSBossArtilleryComponent::ResolveArtilleryMuzzleLocation(
	const FNSBossArtilleryPresentationShot& PresentationShot) const
{
	if (const UNSEnemyPartComponent* PartComponent = GetEnemyPartComponent())
	{
		FTransform MuzzleTransform;
		if (PartComponent->TryGetAimMuzzleTransform(MuzzleTransform))
		{
			return MuzzleTransform.GetLocation();
		}
	}

	const AActor* OwnerActor = GetOwner();
	constexpr float FallbackMuzzleHeight = 300.0f;

	return OwnerActor
		       ? OwnerActor->GetActorLocation() + FVector::UpVector * FallbackMuzzleHeight
		       : FVector(PresentationShot.ImpactLocation) + FVector::UpVector * FallbackMuzzleHeight;
}

FVector UNSBossArtilleryComponent::ProjectImpactLocationToGround(
	const FVector& ImpactLocation) const
{
	if (!bProjectImpactLocationsToGround)
	{
		return ImpactLocation;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return ImpactLocation;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	if (AActor* OwnerActor = GetOwner())
	{
		TArray<AActor*> AttachedActors;
		OwnerActor->GetAttachedActors(AttachedActors);

		for (AActor* AttachedActor : AttachedActors)
		{
			if (IsValid(AttachedActor))
			{
				QueryParams.AddIgnoredActor(AttachedActor);
			}
		}
	}

	const FVector TraceStart = ImpactLocation + FVector::UpVector * ImpactGroundTraceHeight;
	const FVector TraceEnd = ImpactLocation - FVector::UpVector * ImpactGroundTraceDepth;

	FHitResult HitResult;
	const bool bHitGround = World->LineTraceSingleByObjectType(
		HitResult,
		TraceStart,
		TraceEnd,
		FCollisionObjectQueryParams(ECC_WorldStatic),
		QueryParams);

	if (!bHitGround)
	{
		return ImpactLocation;
	}

	return HitResult.ImpactPoint + FVector::UpVector * ImpactGroundZOffset;
}

int32 UNSBossArtilleryComponent::AllocateArtilleryExecutionId()
{
	const int32 AllocatedExecutionId = NextArtilleryExecutionId;

	++NextArtilleryExecutionId;

	if (NextArtilleryExecutionId <= 0)
	{
		NextArtilleryExecutionId = 1;
	}

	return AllocatedExecutionId;
}

void UNSBossArtilleryComponent::FillExecutionTimeRange(
	FNSBossArtilleryExecutionData& InOutExecutionData) const
{
	if (InOutExecutionData.TimedShots.IsEmpty())
	{
		InOutExecutionData.FirstExplosionServerTime = 0.0f;
		InOutExecutionData.LastExplosionServerTime = 0.0f;
		return;
	}

	InOutExecutionData.FirstExplosionServerTime =
		InOutExecutionData.TimedShots[0].ExplosionServerTime;

	InOutExecutionData.LastExplosionServerTime =
		InOutExecutionData.TimedShots[0].ExplosionServerTime;

	for (const FNSBossArtilleryTimedShot& TimedShot : InOutExecutionData.TimedShots)
	{
		InOutExecutionData.FirstExplosionServerTime =
			FMath::Min(
				InOutExecutionData.FirstExplosionServerTime,
				TimedShot.ExplosionServerTime);

		InOutExecutionData.LastExplosionServerTime =
			FMath::Max(
				InOutExecutionData.LastExplosionServerTime,
				TimedShot.ExplosionServerTime);
	}
}

bool UNSBossArtilleryComponent::BuildSequentialTimedShots(
	const FNSBossArtilleryTimingData& TimingData,
	const TArray<FNSBossArtilleryImpactPoint>& ImpactPoints,
	float PatternStartServerTime,
	TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const
{
	const float WarningDuration = GetClampedWarningDuration(TimingData);
	const float Interval = FMath::Max(TimingData.SequentialInterval, 0.0f);

	for (int32 Index = 0; Index < ImpactPoints.Num(); ++Index)
	{
		AddTimedShot(
			ImpactPoints[Index],
			PatternStartServerTime,
			WarningDuration + Interval * static_cast<float>(Index),
			OutTimedShots);
	}

	return !OutTimedShots.IsEmpty();
}

bool UNSBossArtilleryComponent::BuildRandomScatterTimedShots(
	const FNSBossArtilleryTimingData& TimingData,
	const TArray<FNSBossArtilleryImpactPoint>& ImpactPoints,
	float PatternStartServerTime,
	TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const
{
	const float WarningDuration = GetClampedWarningDuration(TimingData);
	const float MinDelay = FMath::Max(TimingData.MinRandomDelay, 0.0f);
	const float MaxDelay = FMath::Max(TimingData.MaxRandomDelay, MinDelay);

	for (const FNSBossArtilleryImpactPoint& ImpactPoint : ImpactPoints)
	{
		AddTimedShot(
			ImpactPoint,
			PatternStartServerTime,
			WarningDuration + FMath::FRandRange(MinDelay, MaxDelay),
			OutTimedShots);
	}

	return !OutTimedShots.IsEmpty();
}

bool UNSBossArtilleryComponent::BuildSimultaneousTimedShots(
	const FNSBossArtilleryTimingData& TimingData,
	const TArray<FNSBossArtilleryImpactPoint>& ImpactPoints,
	float PatternStartServerTime,
	TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const
{
	const float WarningDuration = GetClampedWarningDuration(TimingData);
	const float Jitter = FMath::Max(TimingData.SimultaneousJitter, 0.0f);

	for (const FNSBossArtilleryImpactPoint& ImpactPoint : ImpactPoints)
	{
		AddTimedShot(
			ImpactPoint,
			PatternStartServerTime,
			FMath::Max(WarningDuration + FMath::FRandRange(-Jitter, Jitter), 0.0f),
			OutTimedShots);
	}

	return !OutTimedShots.IsEmpty();
}

bool UNSBossArtilleryComponent::BuildBurstTimedShots(
	const FNSBossArtilleryTimingData& TimingData,
	const TArray<FNSBossArtilleryImpactPoint>& ImpactPoints,
	float PatternStartServerTime,
	TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const
{
	const float WarningDuration = GetClampedWarningDuration(TimingData);
	const int32 ShotsPerBurst = FMath::Max(TimingData.ShotsPerBurst, 1);
	const float BurstInterval = FMath::Max(TimingData.BurstInterval, 0.0f);
	const float IntraBurstJitter = FMath::Max(TimingData.IntraBurstJitter, 0.0f);

	for (int32 Index = 0; Index < ImpactPoints.Num(); ++Index)
	{
		const int32 BurstIndex = Index / ShotsPerBurst;

		AddTimedShot(
			ImpactPoints[Index],
			PatternStartServerTime,
			WarningDuration +
			BurstInterval * static_cast<float>(BurstIndex) +
			FMath::FRandRange(0.0f, IntraBurstJitter),
			OutTimedShots);
	}

	return !OutTimedShots.IsEmpty();
}

bool UNSBossArtilleryComponent::BuildWaveTimedShots(
	const FNSBossArtilleryTimingData& TimingData,
	const TArray<FNSBossArtilleryImpactPoint>& ImpactPoints,
	float PatternStartServerTime,
	TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const
{
	const float WarningDuration = GetClampedWarningDuration(TimingData);
	const float WaveSpeed = FMath::Max(TimingData.WaveSpeed, 1.0f);

	for (const FNSBossArtilleryImpactPoint& ImpactPoint : ImpactPoints)
	{
		AddTimedShot(
			ImpactPoint,
			PatternStartServerTime,
			WarningDuration + FMath::Max(ImpactPoint.DistanceFromOrigin, 0.0f) / WaveSpeed,
			OutTimedShots);
	}

	return !OutTimedShots.IsEmpty();
}

bool UNSBossArtilleryComponent::BuildOffBeatTimedShots(
	const FNSBossArtilleryTimingData& TimingData,
	const TArray<FNSBossArtilleryImpactPoint>& ImpactPoints,
	float PatternStartServerTime,
	TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const
{
	if (TimingData.OffBeatExtraDelays.IsEmpty())
	{
		return false;
	}

	const float WarningDuration = GetClampedWarningDuration(TimingData);

	for (const FNSBossArtilleryImpactPoint& ImpactPoint : ImpactPoints)
	{
		const int32 DelayIndex = ImpactPoint.LocalShotIndex % TimingData.OffBeatExtraDelays.Num();
		const float ExtraDelay = FMath::Max(TimingData.OffBeatExtraDelays[DelayIndex], 0.0f);

		AddTimedShot(
			ImpactPoint,
			PatternStartServerTime,
			WarningDuration + ExtraDelay,
			OutTimedShots);
	}

	return !OutTimedShots.IsEmpty();
}

void UNSBossArtilleryComponent::AddTimedShot(
	const FNSBossArtilleryImpactPoint& ImpactPoint,
	float PatternStartServerTime,
	float ExplosionDelayFromStart,
	TArray<FNSBossArtilleryTimedShot>& OutTimedShots) const
{
	if (!IsValidImpactLocation(ImpactPoint.ImpactLocation))
	{
		return;
	}

	const float ClampedDelay = FMath::Max(ExplosionDelayFromStart, 0.0f);

	FNSBossArtilleryTimedShot TimedShot;
	TimedShot.ImpactPoint = ImpactPoint;
	TimedShot.WarningStartServerTime = PatternStartServerTime;
	TimedShot.ExplosionDelayFromStart = ClampedDelay;
	TimedShot.ExplosionServerTime = PatternStartServerTime + ClampedDelay;

	OutTimedShots.Add(TimedShot);
}

void UNSBossArtilleryComponent::SortTimedShotsByExplosionTime(
	TArray<FNSBossArtilleryTimedShot>& InOutTimedShots) const
{
	InOutTimedShots.Sort(
		[](const FNSBossArtilleryTimedShot& A, const FNSBossArtilleryTimedShot& B)
		{
			if (FMath::IsNearlyEqual(A.ExplosionServerTime, B.ExplosionServerTime))
			{
				return A.ImpactPoint.GlobalShotIndex < B.ImpactPoint.GlobalShotIndex;
			}

			return A.ExplosionServerTime < B.ExplosionServerTime;
		});
}

float UNSBossArtilleryComponent::GetClampedWarningDuration(
	const FNSBossArtilleryTimingData& TimingData) const
{
	return FMath::Max(TimingData.WarningDuration, 0.0f);
}

bool UNSBossArtilleryComponent::BuildTargetCurrentImpactPoints(
	const FNSBossArtilleryPlacementData& PlacementData,
	const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
	TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const
{
	for (const FNSBossArtilleryShotAllocation& Allocation : ShotAllocations)
	{
		for (int32 LocalShotIndex = 0; LocalShotIndex < Allocation.ShotCount; ++LocalShotIndex)
		{
			AddImpactPoint(
				Allocation.TargetPoint,
				ResolveTargetPointLocation(Allocation.TargetPoint),
				Allocation.FirstShotIndex + LocalShotIndex,
				LocalShotIndex,
				INDEX_NONE,
				OutImpactPoints);
		}
	}

	return !OutImpactPoints.IsEmpty();
}

bool UNSBossArtilleryComponent::BuildTargetPredictionImpactPoints(
	const FNSBossArtilleryPlacementData& PlacementData,
	const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
	TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const
{
	for (const FNSBossArtilleryShotAllocation& Allocation : ShotAllocations)
	{
		for (int32 LocalShotIndex = 0; LocalShotIndex < Allocation.ShotCount; ++LocalShotIndex)
		{
			const float PredictionAlpha = Allocation.ShotCount > 1
				                              ? static_cast<float>(LocalShotIndex) / static_cast<float>(Allocation.
					                              ShotCount - 1)
				                              : 1.0f;

			AddImpactPoint(
				Allocation.TargetPoint,
				ResolvePredictedTargetPointLocation(
					Allocation.TargetPoint,
					PlacementData.PredictionTime * PredictionAlpha),
				Allocation.FirstShotIndex + LocalShotIndex,
				LocalShotIndex,
				INDEX_NONE,
				OutImpactPoints);
		}
	}

	return !OutImpactPoints.IsEmpty();
}

bool UNSBossArtilleryComponent::BuildRandomAroundTargetImpactPoints(
	const FNSBossArtilleryPlacementData& PlacementData,
	const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
	TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const
{
	for (const FNSBossArtilleryShotAllocation& Allocation : ShotAllocations)
	{
		const FVector BaseLocation = ResolveTargetPointLocation(Allocation.TargetPoint);

		for (int32 LocalShotIndex = 0; LocalShotIndex < Allocation.ShotCount; ++LocalShotIndex)
		{
			FVector ImpactLocation = BaseLocation;

			for (int32 RetryIndex = 0; RetryIndex < MaxPlacementRetryCount; ++RetryIndex)
			{
				ImpactLocation = MakeRandomPointAroundLocation(BaseLocation, PlacementData.ScatterRadius);

				if (!IsImpactLocationTooClose(
					ImpactLocation,
					OutImpactPoints,
					PlacementData.MinImpactLocationDistance))
				{
					break;
				}
			}

			AddImpactPoint(
				Allocation.TargetPoint,
				ImpactLocation,
				Allocation.FirstShotIndex + LocalShotIndex,
				LocalShotIndex,
				INDEX_NONE,
				OutImpactPoints);
		}
	}

	return !OutImpactPoints.IsEmpty();
}

bool UNSBossArtilleryComponent::BuildClusterAroundTargetImpactPoints(
	const FNSBossArtilleryPlacementData& PlacementData,
	const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
	TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const
{
	for (const FNSBossArtilleryShotAllocation& Allocation : ShotAllocations)
	{
		const FVector BaseLocation = ResolveTargetPointLocation(Allocation.TargetPoint);

		for (int32 LocalShotIndex = 0; LocalShotIndex < Allocation.ShotCount; ++LocalShotIndex)
		{
			const FVector ImpactLocation =
				LocalShotIndex == 0
					? BaseLocation
					: MakeRandomPointAroundLocation(BaseLocation, PlacementData.ClusterRadius);

			AddImpactPoint(
				Allocation.TargetPoint,
				ImpactLocation,
				Allocation.FirstShotIndex + LocalShotIndex,
				LocalShotIndex,
				INDEX_NONE,
				OutImpactPoints);
		}
	}

	return !OutImpactPoints.IsEmpty();
}

bool UNSBossArtilleryComponent::BuildBetweenTargetsImpactPoints(
	const FNSBossArtilleryPlacementData& PlacementData,
	const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
	TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const
{
	for (const FNSBossArtilleryShotAllocation& Allocation : ShotAllocations)
	{
		const FVector BaseLocation = ResolveTargetPointLocation(Allocation.TargetPoint);

		for (int32 LocalShotIndex = 0; LocalShotIndex < Allocation.ShotCount; ++LocalShotIndex)
		{
			const FVector ImpactLocation =
				LocalShotIndex == 0
					? BaseLocation
					: MakeRandomPointAroundLocation(BaseLocation, PlacementData.ScatterRadius);

			AddImpactPoint(
				Allocation.TargetPoint,
				ImpactLocation,
				Allocation.FirstShotIndex + LocalShotIndex,
				LocalShotIndex,
				INDEX_NONE,
				OutImpactPoints);
		}
	}

	return !OutImpactPoints.IsEmpty();
}

bool UNSBossArtilleryComponent::BuildRingImpactPoints(
	const FNSBossArtilleryPlacementData& PlacementData,
	const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
	TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const
{
	for (const FNSBossArtilleryShotAllocation& Allocation : ShotAllocations)
	{
		const FVector Origin = ResolveTargetPointLocation(Allocation.TargetPoint);

		for (int32 LocalShotIndex = 0; LocalShotIndex < Allocation.ShotCount; ++LocalShotIndex)
		{
			const float AngleRadians =
				Allocation.ShotCount > 0
					? (2.0f * PI * static_cast<float>(LocalShotIndex)) / static_cast<float>(Allocation.ShotCount)
					: 0.0f;

			const FVector Offset(
				FMath::Cos(AngleRadians) * PlacementData.RingStartRadius,
				FMath::Sin(AngleRadians) * PlacementData.RingStartRadius,
				0.0f);

			AddImpactPoint(
				Allocation.TargetPoint,
				Origin + Offset,
				Allocation.FirstShotIndex + LocalShotIndex,
				LocalShotIndex,
				0,
				OutImpactPoints);
		}
	}

	return !OutImpactPoints.IsEmpty();
}

bool UNSBossArtilleryComponent::BuildWaveRingImpactPoints(
	const FNSBossArtilleryShotBudgetData& ShotBudgetData,
	const FNSBossArtilleryPlacementData& PlacementData,
	const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
	TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const
{
	const int32 RingCount = FMath::Max(ShotBudgetData.RingCount, 1);
	const int32 ShotsPerRing = FMath::Max(ShotBudgetData.ShotsPerRing, 1);

	for (const FNSBossArtilleryShotAllocation& Allocation : ShotAllocations)
	{
		const FVector Origin = ResolveTargetPointLocation(Allocation.TargetPoint);

		for (int32 LocalShotIndex = 0; LocalShotIndex < Allocation.ShotCount; ++LocalShotIndex)
		{
			const int32 RingIndex = FMath::Clamp(LocalShotIndex / ShotsPerRing, 0, RingCount - 1);
			const int32 ShotIndexInRing = LocalShotIndex % ShotsPerRing;
			const int32 CurrentRingShotCount =
				FMath::Min(ShotsPerRing, Allocation.ShotCount - RingIndex * ShotsPerRing);

			const float Radius =
				PlacementData.RingStartRadius +
				PlacementData.RingSpacing * static_cast<float>(RingIndex);

			const float AngleRadians =
				CurrentRingShotCount > 0
					? (2.0f * PI * static_cast<float>(ShotIndexInRing)) / static_cast<float>(CurrentRingShotCount)
					: 0.0f;

			const FVector Offset(
				FMath::Cos(AngleRadians) * Radius,
				FMath::Sin(AngleRadians) * Radius,
				0.0f);

			AddImpactPoint(
				Allocation.TargetPoint,
				Origin + Offset,
				Allocation.FirstShotIndex + LocalShotIndex,
				LocalShotIndex,
				RingIndex,
				OutImpactPoints);
		}
	}

	return !OutImpactPoints.IsEmpty();
}

bool UNSBossArtilleryComponent::BuildEscapeRouteBlockImpactPoints(
	const FNSBossArtilleryPlacementData& PlacementData,
	const TArray<FNSBossArtilleryShotAllocation>& ShotAllocations,
	TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const
{
	for (const FNSBossArtilleryShotAllocation& Allocation : ShotAllocations)
	{
		for (int32 LocalShotIndex = 0; LocalShotIndex < Allocation.ShotCount; ++LocalShotIndex)
		{
			AddImpactPoint(
				Allocation.TargetPoint,
				MakeEscapeRouteBlockLocation(
					Allocation.TargetPoint,
					LocalShotIndex,
					PlacementData),
				Allocation.FirstShotIndex + LocalShotIndex,
				LocalShotIndex,
				INDEX_NONE,
				OutImpactPoints);
		}
	}

	return !OutImpactPoints.IsEmpty();
}

void UNSBossArtilleryComponent::AddImpactPoint(
	const FNSBossArtilleryTargetPoint& SourceTargetPoint,
	const FVector& ImpactLocation,
	int32 GlobalShotIndex,
	int32 LocalShotIndex,
	int32 RingIndex,
	TArray<FNSBossArtilleryImpactPoint>& OutImpactPoints) const
{
	const FVector ClampedImpactLocation = ClampImpactLocationToArena(ImpactLocation);
	const FVector GroundedImpactLocation = ProjectImpactLocationToGround(ClampedImpactLocation);

	if (!IsValidImpactLocation(GroundedImpactLocation))
	{
		return;
	}

	FNSBossArtilleryImpactPoint ImpactPoint;
	ImpactPoint.SourceTargetPoint = SourceTargetPoint;
	ImpactPoint.ImpactLocation = GroundedImpactLocation;
	ImpactPoint.GlobalShotIndex = GlobalShotIndex;
	ImpactPoint.LocalShotIndex = LocalShotIndex;
	ImpactPoint.RingIndex = RingIndex;
	ImpactPoint.DistanceFromOrigin =
		FVector::Dist2D(
			ResolveTargetPointLocation(SourceTargetPoint),
			GroundedImpactLocation);

	OutImpactPoints.Add(ImpactPoint);
}

FVector UNSBossArtilleryComponent::ResolveTargetPointLocation(
	const FNSBossArtilleryTargetPoint& TargetPoint) const
{
	switch (TargetPoint.PointType)
	{
	case ENSBossArtilleryTargetPointType::Actor:
		return IsValid(TargetPoint.PrimaryTarget)
			       ? TargetPoint.PrimaryTarget->GetActorLocation()
			       : TargetPoint.Location;

	case ENSBossArtilleryTargetPointType::PairMidpoint:
		if (IsValid(TargetPoint.PrimaryTarget) && IsValid(TargetPoint.SecondaryTarget))
		{
			return (TargetPoint.PrimaryTarget->GetActorLocation() + TargetPoint.SecondaryTarget->GetActorLocation()) *
				0.5f;
		}
		return TargetPoint.Location;

	case ENSBossArtilleryTargetPointType::ArenaCenter:
	case ENSBossArtilleryTargetPointType::BossLocation:
	default:
		return TargetPoint.Location;
	}
}

FVector UNSBossArtilleryComponent::ResolvePredictedTargetPointLocation(
	const FNSBossArtilleryTargetPoint& TargetPoint,
	float PredictionTime) const
{
	switch (TargetPoint.PointType)
	{
	case ENSBossArtilleryTargetPointType::Actor:
		if (IsValid(TargetPoint.PrimaryTarget))
		{
			return TargetPoint.PrimaryTarget->GetActorLocation() +
				TargetPoint.PrimaryTarget->GetVelocity() * FMath::Max(PredictionTime, 0.0f);
		}
		return TargetPoint.Location;

	case ENSBossArtilleryTargetPointType::PairMidpoint:
		if (IsValid(TargetPoint.PrimaryTarget) && IsValid(TargetPoint.SecondaryTarget))
		{
			const FVector FirstPredictedLocation =
				TargetPoint.PrimaryTarget->GetActorLocation() +
				TargetPoint.PrimaryTarget->GetVelocity() * FMath::Max(PredictionTime, 0.0f);

			const FVector SecondPredictedLocation =
				TargetPoint.SecondaryTarget->GetActorLocation() +
				TargetPoint.SecondaryTarget->GetVelocity() * FMath::Max(PredictionTime, 0.0f);

			return (FirstPredictedLocation + SecondPredictedLocation) * 0.5f;
		}
		return TargetPoint.Location;

	case ENSBossArtilleryTargetPointType::ArenaCenter:
	case ENSBossArtilleryTargetPointType::BossLocation:
	default:
		return TargetPoint.Location;
	}
}

FVector UNSBossArtilleryComponent::MakeRandomPointAroundLocation(
	const FVector& BaseLocation,
	float Radius) const
{
	const FVector2D RandomOffset = FMath::RandPointInCircle(FMath::Max(Radius, 0.0f));

	return BaseLocation + FVector(RandomOffset.X, RandomOffset.Y, 0.0f);
}

FVector UNSBossArtilleryComponent::MakeEscapeRouteBlockLocation(
	const FNSBossArtilleryTargetPoint& TargetPoint,
	int32 LocalShotIndex,
	const FNSBossArtilleryPlacementData& PlacementData) const
{
	const FVector BaseLocation = ResolveTargetPointLocation(TargetPoint);

	FVector MoveDirection = FVector::ForwardVector;

	if (IsValid(TargetPoint.PrimaryTarget))
	{
		MoveDirection = TargetPoint.PrimaryTarget->GetVelocity().GetSafeNormal2D();
	}

	if (MoveDirection.IsNearlyZero())
	{
		MoveDirection = TargetPoint.Direction.GetSafeNormal2D();
	}

	if (MoveDirection.IsNearlyZero())
	{
		MoveDirection = FVector::ForwardVector;
	}

	const FVector SideDirection = FVector::CrossProduct(FVector::UpVector, MoveDirection).GetSafeNormal2D();

	float SideSign = 0.0f;

	if (LocalShotIndex % 3 == 1)
	{
		SideSign = -1.0f;
	}
	else if (LocalShotIndex % 3 == 2)
	{
		SideSign = 1.0f;
	}

	return BaseLocation +
		MoveDirection * PlacementData.ForwardBlockDistance +
		SideDirection * PlacementData.SideBlockOffset * SideSign;
}

FVector UNSBossArtilleryComponent::ClampImpactLocationToArena(const FVector& ImpactLocation) const
{
	if (bClampImpactLocationsToArenaBounds && IsValid(ArenaBounds))
	{
		return ArenaBounds->ClampPointToBounds(ImpactLocation);
	}

	return ImpactLocation;
}

bool UNSBossArtilleryComponent::IsValidImpactLocation(const FVector& ImpactLocation) const
{
	return !ImpactLocation.ContainsNaN();
}

bool UNSBossArtilleryComponent::IsImpactLocationTooClose(
	const FVector& CandidateLocation,
	const TArray<FNSBossArtilleryImpactPoint>& ExistingImpactPoints,
	float MinDistance) const
{
	if (MinDistance <= 0.0f)
	{
		return false;
	}

	for (const FNSBossArtilleryImpactPoint& ExistingImpactPoint : ExistingImpactPoints)
	{
		if (FVector::Dist2D(CandidateLocation, ExistingImpactPoint.ImpactLocation) < MinDistance)
		{
			return true;
		}
	}

	return false;
}

bool UNSBossArtilleryComponent::BuildPerTargetShotAllocations(
	const FNSBossArtilleryShotBudgetData& ShotBudgetData,
	const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
	TArray<FNSBossArtilleryShotAllocation>& OutShotAllocations,
	int32& OutTotalShotCount) const
{
	TArray<FNSBossArtilleryTargetPoint> ValidTargetPoints;
	CollectValidTargetPoints(TargetPoints, ValidTargetPoints);

	TArray<int32> DesiredShotCounts;
	DesiredShotCounts.Reserve(ValidTargetPoints.Num());

	for (int32 Index = 0; Index < ValidTargetPoints.Num(); ++Index)
	{
		DesiredShotCounts.Add(FMath::Max(ShotBudgetData.ShotsPerTarget, 0));
	}

	return BuildShotAllocationsFromDesiredCounts(
		ValidTargetPoints,
		DesiredShotCounts,
		GetMaxTotalShotCount(ShotBudgetData),
		OutShotAllocations,
		OutTotalShotCount);
}

bool UNSBossArtilleryComponent::BuildFixedTotalShotAllocations(
	const FNSBossArtilleryShotBudgetData& ShotBudgetData,
	const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
	TArray<FNSBossArtilleryShotAllocation>& OutShotAllocations,
	int32& OutTotalShotCount) const
{
	return BuildFixedShotAllocationsFromTotal(
		TargetPoints,
		FMath::Max(ShotBudgetData.FixedTotalShots, 0),
		GetMaxTotalShotCount(ShotBudgetData),
		OutShotAllocations,
		OutTotalShotCount);
}

bool UNSBossArtilleryComponent::BuildPerRingShotAllocations(
	const FNSBossArtilleryShotBudgetData& ShotBudgetData,
	const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
	TArray<FNSBossArtilleryShotAllocation>& OutShotAllocations,
	int32& OutTotalShotCount) const
{
	const int32 RequestedTotalShots =
		FMath::Max(ShotBudgetData.RingCount, 0) *
		FMath::Max(ShotBudgetData.ShotsPerRing, 0);

	return BuildFixedShotAllocationsFromTotal(
		TargetPoints,
		RequestedTotalShots,
		GetMaxTotalShotCount(ShotBudgetData),
		OutShotAllocations,
		OutTotalShotCount);
}

bool UNSBossArtilleryComponent::BuildBetweenCombatantShotAllocations(
	const FNSBossArtilleryShotBudgetData& ShotBudgetData,
	const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
	TArray<FNSBossArtilleryShotAllocation>& OutShotAllocations,
	int32& OutTotalShotCount) const
{
	TArray<FNSBossArtilleryTargetPoint> ValidTargetPoints;
	CollectValidTargetPoints(TargetPoints, ValidTargetPoints);

	TArray<int32> DesiredShotCounts;
	DesiredShotCounts.Reserve(ValidTargetPoints.Num());

	for (int32 Index = 0; Index < ValidTargetPoints.Num(); ++Index)
	{
		DesiredShotCounts.Add(FMath::Max(ShotBudgetData.ShotsPerPair, 0));
	}

	return BuildShotAllocationsFromDesiredCounts(
		ValidTargetPoints,
		DesiredShotCounts,
		GetMaxTotalShotCount(ShotBudgetData),
		OutShotAllocations,
		OutTotalShotCount);
}

bool UNSBossArtilleryComponent::BuildShotAllocationsFromDesiredCounts(
	const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
	const TArray<int32>& DesiredShotCounts,
	int32 MaxTotalShots,
	TArray<FNSBossArtilleryShotAllocation>& OutShotAllocations,
	int32& OutTotalShotCount) const
{
	OutShotAllocations.Reset();
	OutTotalShotCount = 0;

	if (TargetPoints.IsEmpty() || TargetPoints.Num() != DesiredShotCounts.Num())
	{
		return false;
	}

	int32 RequestedTotalShots = 0;

	for (const int32 DesiredShotCount : DesiredShotCounts)
	{
		RequestedTotalShots += FMath::Max(DesiredShotCount, 0);
	}

	const int32 ActualTotalShots =
		FMath::Min(RequestedTotalShots, FMath::Max(MaxTotalShots, 0));

	if (ActualTotalShots <= 0)
	{
		return false;
	}

	TArray<int32> AllocatedShotCounts;
	AllocatedShotCounts.Init(0, TargetPoints.Num());

	int32 RemainingShots = ActualTotalShots;

	while (RemainingShots > 0)
	{
		bool bAssignedAnyShot = false;

		for (int32 Index = 0; Index < TargetPoints.Num(); ++Index)
		{
			if (AllocatedShotCounts[Index] >= DesiredShotCounts[Index])
			{
				continue;
			}

			++AllocatedShotCounts[Index];
			--RemainingShots;
			bAssignedAnyShot = true;

			if (RemainingShots <= 0)
			{
				break;
			}
		}

		if (!bAssignedAnyShot)
		{
			break;
		}
	}

	for (int32 Index = 0; Index < TargetPoints.Num(); ++Index)
	{
		const int32 ShotCount = AllocatedShotCounts[Index];

		if (ShotCount <= 0)
		{
			continue;
		}

		FNSBossArtilleryShotAllocation Allocation;
		Allocation.TargetPoint = TargetPoints[Index];
		Allocation.ShotCount = ShotCount;
		Allocation.FirstShotIndex = OutTotalShotCount;

		OutShotAllocations.Add(Allocation);
		OutTotalShotCount += ShotCount;
	}

	return OutTotalShotCount > 0;
}

bool UNSBossArtilleryComponent::BuildFixedShotAllocationsFromTotal(
	const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
	int32 RequestedTotalShots,
	int32 MaxTotalShots,
	TArray<FNSBossArtilleryShotAllocation>& OutShotAllocations,
	int32& OutTotalShotCount) const
{
	OutShotAllocations.Reset();
	OutTotalShotCount = 0;

	TArray<FNSBossArtilleryTargetPoint> ValidTargetPoints;
	CollectValidTargetPoints(TargetPoints, ValidTargetPoints);

	if (ValidTargetPoints.IsEmpty())
	{
		return false;
	}

	const int32 ActualTotalShots =
		FMath::Min(FMath::Max(RequestedTotalShots, 0), FMath::Max(MaxTotalShots, 0));

	if (ActualTotalShots <= 0)
	{
		return false;
	}

	TArray<int32> AllocatedShotCounts;
	AllocatedShotCounts.Init(0, ValidTargetPoints.Num());

	for (int32 ShotIndex = 0; ShotIndex < ActualTotalShots; ++ShotIndex)
	{
		const int32 TargetPointIndex = ShotIndex % ValidTargetPoints.Num();
		++AllocatedShotCounts[TargetPointIndex];
	}

	for (int32 Index = 0; Index < ValidTargetPoints.Num(); ++Index)
	{
		const int32 ShotCount = AllocatedShotCounts[Index];

		if (ShotCount <= 0)
		{
			continue;
		}

		FNSBossArtilleryShotAllocation Allocation;
		Allocation.TargetPoint = ValidTargetPoints[Index];
		Allocation.ShotCount = ShotCount;
		Allocation.FirstShotIndex = OutTotalShotCount;

		OutShotAllocations.Add(Allocation);
		OutTotalShotCount += ShotCount;
	}

	return OutTotalShotCount > 0;
}

bool UNSBossArtilleryComponent::IsValidTargetPoint(
	const FNSBossArtilleryTargetPoint& TargetPoint) const
{
	switch (TargetPoint.PointType)
	{
	case ENSBossArtilleryTargetPointType::Actor:
		return IsValid(TargetPoint.PrimaryTarget);

	case ENSBossArtilleryTargetPointType::PairMidpoint:
		return IsValid(TargetPoint.PrimaryTarget) && IsValid(TargetPoint.SecondaryTarget);

	case ENSBossArtilleryTargetPointType::ArenaCenter:
	case ENSBossArtilleryTargetPointType::BossLocation:
		return !TargetPoint.Location.ContainsNaN();

	default:
		return false;
	}
}

void UNSBossArtilleryComponent::CollectValidTargetPoints(
	const TArray<FNSBossArtilleryTargetPoint>& TargetPoints,
	TArray<FNSBossArtilleryTargetPoint>& OutValidTargetPoints) const
{
	OutValidTargetPoints.Reset();

	for (const FNSBossArtilleryTargetPoint& TargetPoint : TargetPoints)
	{
		if (IsValidTargetPoint(TargetPoint))
		{
			OutValidTargetPoints.Add(TargetPoint);
		}
	}
}

int32 UNSBossArtilleryComponent::GetMaxTotalShotCount(
	const FNSBossArtilleryShotBudgetData& ShotBudgetData) const
{
	return FMath::Max(ShotBudgetData.MaxTotalShots, 1);
}

bool UNSBossArtilleryComponent::CollectAllCombatantTargetPoints(
	TArray<FNSBossArtilleryTargetPoint>& OutTargetPoints) const
{
	TArray<AActor*> ValidCombatants;
	CollectValidCombatants(ValidCombatants);

	for (AActor* Combatant : ValidCombatants)
	{
		OutTargetPoints.Add(MakeActorTargetPoint(Combatant));
	}

	return !OutTargetPoints.IsEmpty();
}

bool UNSBossArtilleryComponent::CollectHighestThreatTargetPoint(
	TArray<FNSBossArtilleryTargetPoint>& OutTargetPoints) const
{
	const UNSEnemyThreatComponent* ThreatComponent = GetThreatComponent();
	AActor* ThreatTarget = ThreatComponent ? ThreatComponent->GetCurrentTarget() : nullptr;

	if (!IsValidCombatant(ThreatTarget))
	{
		return false;
	}

	OutTargetPoints.Add(MakeActorTargetPoint(ThreatTarget));
	return true;
}

bool UNSBossArtilleryComponent::CollectArenaCenterTargetPoint(
	TArray<FNSBossArtilleryTargetPoint>& OutTargetPoints) const
{
	OutTargetPoints.Add(
		MakeLocationTargetPoint(
			ENSBossArtilleryTargetPointType::ArenaCenter,
			GetArenaCenterLocation()));

	return true;
}

bool UNSBossArtilleryComponent::CollectBossLocationTargetPoint(
	TArray<FNSBossArtilleryTargetPoint>& OutTargetPoints) const
{
	const AActor* OwnerActor = GetOwner();

	if (!IsValid(OwnerActor))
	{
		return false;
	}

	OutTargetPoints.Add(
		MakeLocationTargetPoint(
			ENSBossArtilleryTargetPointType::BossLocation,
			OwnerActor->GetActorLocation()));

	return true;
}

bool UNSBossArtilleryComponent::CollectBetweenCombatantTargetPoints(
	const FNSBossArtilleryTargetData& TargetData,
	TArray<FNSBossArtilleryTargetPoint>& OutTargetPoints) const
{
	TArray<AActor*> ValidCombatants;
	CollectValidCombatants(ValidCombatants);

	if (ValidCombatants.Num() < 2)
	{
		return false;
	}

	struct FNSLocalPairCandidate
	{
		AActor* FirstTarget = nullptr;
		AActor* SecondTarget = nullptr;

		float Distance = 0.0f;
	};

	TArray<FNSLocalPairCandidate> PairCandidates;

	for (int32 FirstIndex = 0; FirstIndex < ValidCombatants.Num(); ++FirstIndex)
	{
		for (int32 SecondIndex = FirstIndex + 1; SecondIndex < ValidCombatants.Num(); ++SecondIndex)
		{
			AActor* FirstTarget = ValidCombatants[FirstIndex];
			AActor* SecondTarget = ValidCombatants[SecondIndex];

			if (!IsValidCombatant(FirstTarget) || !IsValidCombatant(SecondTarget))
			{
				continue;
			}

			const float PairDistance =
				FVector::Dist2D(
					FirstTarget->GetActorLocation(),
					SecondTarget->GetActorLocation());

			if (PairDistance < TargetData.MinPairDistance)
			{
				continue;
			}

			if (TargetData.MaxPairDistance > 0.0f &&
				PairDistance > TargetData.MaxPairDistance)
			{
				continue;
			}

			FNSLocalPairCandidate PairCandidate;
			PairCandidate.FirstTarget = FirstTarget;
			PairCandidate.SecondTarget = SecondTarget;
			PairCandidate.Distance = PairDistance;

			PairCandidates.Add(PairCandidate);
		}
	}

	PairCandidates.Sort(
		[](const FNSLocalPairCandidate& A, const FNSLocalPairCandidate& B)
		{
			return A.Distance < B.Distance;
		});

	const int32 MaxPairCount = FMath::Max(TargetData.MaxPairCount, 0);
	const int32 PairCount = MaxPairCount > 0
		                        ? FMath::Min(MaxPairCount, PairCandidates.Num())
		                        : PairCandidates.Num();

	for (int32 PairIndex = 0; PairIndex < PairCount; ++PairIndex)
	{
		const FNSLocalPairCandidate& PairCandidate = PairCandidates[PairIndex];

		OutTargetPoints.Add(
			MakePairTargetPoint(
				PairCandidate.FirstTarget,
				PairCandidate.SecondTarget));
	}

	return !OutTargetPoints.IsEmpty();
}

bool UNSBossArtilleryComponent::IsValidCombatant(const AActor* Combatant) const
{
	if (!IsValid(Combatant))
	{
		return false;
	}

	if (Combatant == GetOwner())
	{
		return false;
	}

	if (const UNSEnemyTargetComponent* TargetComponent = GetTargetComponent())
	{
		return TargetComponent->IsValidLivingTarget(Combatant);
	}

	return true;
}

FNSBossArtilleryTargetPoint UNSBossArtilleryComponent::MakeActorTargetPoint(AActor* TargetActor) const
{
	FNSBossArtilleryTargetPoint TargetPoint;

	if (!IsValid(TargetActor))
	{
		return TargetPoint;
	}

	TargetPoint.PointType = ENSBossArtilleryTargetPointType::Actor;
	TargetPoint.PrimaryTarget = TargetActor;
	TargetPoint.Location = TargetActor->GetActorLocation();

	if (const AActor* OwnerActor = GetOwner())
	{
		TargetPoint.Direction = (TargetPoint.Location - OwnerActor->GetActorLocation()).GetSafeNormal2D();
		TargetPoint.Distance = FVector::Dist2D(TargetPoint.Location, OwnerActor->GetActorLocation());
	}

	return TargetPoint;
}

FNSBossArtilleryTargetPoint UNSBossArtilleryComponent::MakePairTargetPoint(
	AActor* FirstTarget,
	AActor* SecondTarget) const
{
	FNSBossArtilleryTargetPoint TargetPoint;

	if (!IsValid(FirstTarget) || !IsValid(SecondTarget))
	{
		return TargetPoint;
	}

	const FVector FirstLocation = FirstTarget->GetActorLocation();
	const FVector SecondLocation = SecondTarget->GetActorLocation();

	TargetPoint.PointType = ENSBossArtilleryTargetPointType::PairMidpoint;
	TargetPoint.PrimaryTarget = FirstTarget;
	TargetPoint.SecondaryTarget = SecondTarget;
	TargetPoint.Location = (FirstLocation + SecondLocation) * 0.5f;
	TargetPoint.Direction = (SecondLocation - FirstLocation).GetSafeNormal2D();
	TargetPoint.Distance = FVector::Dist2D(FirstLocation, SecondLocation);

	return TargetPoint;
}

FNSBossArtilleryTargetPoint UNSBossArtilleryComponent::MakeLocationTargetPoint(
	ENSBossArtilleryTargetPointType PointType,
	const FVector& Location) const
{
	FNSBossArtilleryTargetPoint TargetPoint;

	TargetPoint.PointType = PointType;
	TargetPoint.Location = Location;

	if (const AActor* OwnerActor = GetOwner())
	{
		TargetPoint.Direction = (Location - OwnerActor->GetActorLocation()).GetSafeNormal2D();
		TargetPoint.Distance = FVector::Dist2D(Location, OwnerActor->GetActorLocation());
	}

	return TargetPoint;
}

FVector UNSBossArtilleryComponent::GetArenaCenterLocation() const
{
	const AActor* OwnerActor = GetOwner();

	if (IsValid(ArenaBounds))
	{
		const float CenterZ = OwnerActor ? OwnerActor->GetActorLocation().Z : ArenaBounds->GetActorLocation().Z;
		return ArenaBounds->GetArenaCenter(CenterZ);
	}

	return OwnerActor ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
}

UNSEnemyTargetComponent* UNSBossArtilleryComponent::GetTargetComponent() const
{
	const AActor* OwnerActor = GetOwner();

	return OwnerActor
		       ? OwnerActor->FindComponentByClass<UNSEnemyTargetComponent>()
		       : nullptr;
}

UNSEnemyThreatComponent* UNSBossArtilleryComponent::GetThreatComponent() const
{
	const AActor* OwnerActor = GetOwner();

	return OwnerActor
		       ? OwnerActor->FindComponentByClass<UNSEnemyThreatComponent>()
		       : nullptr;
}

bool UNSBossArtilleryComponent::CanUsePatternData(
	const UNSBossArtilleryPatternData& PatternData,
	const FNSBossArtillerySelectionContext& Context,
	FString& OutRejectReason) const
{
	if (Context.bPatternLocked)
	{
		OutRejectReason = TEXT("현재 페이즈가 패턴 선택을 잠그고 있습니다.");
		return false;
	}

	if (PatternData.PatternId == ENSBossArtilleryPatternId::None)
	{
		OutRejectReason = TEXT("PatternId가 None입니다.");
		return false;
	}

	if (PatternData.SelectionData.BaseWeight <= 0.0f)
	{
		OutRejectReason = TEXT("BaseWeight가 0 이하입니다.");
		return false;
	}

	if (!IsPatternAllowedByPhase(PatternData, Context.CurrentPhaseTag))
	{
		OutRejectReason = TEXT("현재 페이즈 태그에서 허용되지 않는 패턴입니다.");
		return false;
	}

	if (Context.CombatantCount < PatternData.TargetData.MinCombatantCount)
	{
		OutRejectReason = TEXT("전투 참여자 수가 패턴의 최소 요구 수보다 적습니다.");
		return false;
	}

	if (PatternData.TargetData.MaxCombatantCount > 0 &&
		Context.CombatantCount > PatternData.TargetData.MaxCombatantCount)
	{
		OutRejectReason = TEXT("전투 참여자 수가 패턴의 최대 허용 수보다 많습니다.");
		return false;
	}

	if (Context.MaxDangerScore > 0 &&
		PatternData.SelectionData.DangerScore > Context.MaxDangerScore)
	{
		OutRejectReason = TEXT("패턴 위험도가 현재 선택 예산을 초과했습니다.");
		return false;
	}

	if (IsImmediateRepeatBlocked(PatternData))
	{
		OutRejectReason = TEXT("직전 반복 금지 정책으로 제외됐습니다.");
		return false;
	}

	if (IsBlockedByHardCooldown(PatternData))
	{
		OutRejectReason = TEXT("하드 쿨다운이 남아 있어 제외됐습니다.");
		return false;
	}

	OutRejectReason.Reset();
	return true;
}

float UNSBossArtilleryComponent::ComputeFinalWeight(
	const UNSBossArtilleryPatternData& PatternData) const
{
	// DataAsset에 설정된 기본 선택 가중치
	const float BaseWeight = FMath::Max(PatternData.SelectionData.BaseWeight, 0.0f);

	// 최근 사용 이력에 따라 적용할 가중치 배율
	const float RecentUseMultiplier = GetRecentUseMultiplier(PatternData);

	return BaseWeight * RecentUseMultiplier;
}

bool UNSBossArtilleryComponent::IsPatternAllowedByPhase(
	const UNSBossArtilleryPatternData& PatternData,
	const FGameplayTag& CurrentPhaseTag) const
{
	if (PatternData.SelectionData.AllowedPhaseTags.IsEmpty())
	{
		return true;
	}

	if (!CurrentPhaseTag.IsValid())
	{
		return false;
	}

	return PatternData.SelectionData.AllowedPhaseTags.HasTagExact(CurrentPhaseTag);
}

bool UNSBossArtilleryComponent::IsImmediateRepeatBlocked(
	const UNSBossArtilleryPatternData& PatternData) const
{
	if (PatternData.SelectionData.RepeatPolicy != ENSBossArtilleryRepeatPolicy::BlockImmediateRepeat)
	{
		return false;
	}

	if (RecentPatternHistory.IsEmpty())
	{
		return false;
	}

	return RecentPatternHistory[0] == PatternData.PatternId;
}

bool UNSBossArtilleryComponent::IsBlockedByHardCooldown(
	const UNSBossArtilleryPatternData& PatternData) const
{
	// 하드 쿨다운 맵에 남은 카운트가 있으면 선택 후보에서 제외
	const int32* RemainingCount = HardCooldownRemainingByPattern.Find(PatternData.PatternId);

	return RemainingCount && *RemainingCount > 0;
}

int32 UNSBossArtilleryComponent::FindMostRecentPatternIndex(
	ENSBossArtilleryPatternId PatternId) const
{
	for (int32 Index = 0; Index < RecentPatternHistory.Num(); ++Index)
	{
		if (RecentPatternHistory[Index] == PatternId)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

float UNSBossArtilleryComponent::GetRecentUseMultiplier(
	const UNSBossArtilleryPatternData& PatternData) const
{
	if (PatternData.SelectionData.RepeatPolicy == ENSBossArtilleryRepeatPolicy::None)
	{
		return 1.0f;
	}

	// 최근 사용 이력에서 이 패턴이 발견되는 위치
	const int32 RecentIndex = FindMostRecentPatternIndex(PatternData.PatternId);

	if (RecentIndex == INDEX_NONE)
	{
		return 1.0f;
	}

	if (!PatternData.SelectionData.RecentUseWeightMultipliers.IsValidIndex(RecentIndex))
	{
		return 1.0f;
	}

	return FMath::Max(
		PatternData.SelectionData.RecentUseWeightMultipliers[RecentIndex],
		0.0f);
}

void UNSBossArtilleryComponent::TickHardCooldowns()
{
	// 순회 중 제거를 안전하게 처리하기 위한 키 복사본
	TArray<ENSBossArtilleryPatternId> PatternIds;
	HardCooldownRemainingByPattern.GetKeys(PatternIds);

	for (const ENSBossArtilleryPatternId PatternId : PatternIds)
	{
		int32* RemainingCount = HardCooldownRemainingByPattern.Find(PatternId);
		if (!RemainingCount)
		{
			continue;
		}

		*RemainingCount = FMath::Max(*RemainingCount - 1, 0);

		if (*RemainingCount <= 0)
		{
			HardCooldownRemainingByPattern.Remove(PatternId);
		}
	}
}

void UNSBossArtilleryComponent::PushRecentPattern(ENSBossArtilleryPatternId PatternId)
{
	if (PatternId == ENSBossArtilleryPatternId::None)
	{
		return;
	}

	if (RecentPatternHistorySize <= 0)
	{
		return;
	}

	RecentPatternHistory.Insert(PatternId, 0);

	while (RecentPatternHistory.Num() > RecentPatternHistorySize)
	{
		RecentPatternHistory.RemoveAt(RecentPatternHistory.Num() - 1);
	}
}

UNSEnemyPhaseComponent* UNSBossArtilleryComponent::GetPhaseComponent() const
{
	const AActor* OwnerActor = GetOwner();

	return OwnerActor
		       ? OwnerActor->FindComponentByClass<UNSEnemyPhaseComponent>()
		       : nullptr;
}
