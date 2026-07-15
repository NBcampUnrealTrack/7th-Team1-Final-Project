// Copyright 2026 One Team. All rights reserved.

#include "NSBossArtilleryComponent.h"

#include "GameFramework/Actor.h"
#include "NeoSanctum/AI/Enemy/HelperActor/NSBossArenaBounds.h"
#include "NeoSanctum/Combat/Component/NSEnemyTargetComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Data/AI/NSBossArtilleryPatternData.h"

UNSBossArtilleryComponent::UNSBossArtilleryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UNSBossArtilleryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 보스 제거 또는 레벨 종료 시 런타임 선택 상태를 정리
	ResetArtillerySelectionState();

	// 보스 제거 또는 레벨 종료 시 등록 전투 참여자 상태를 정리
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
