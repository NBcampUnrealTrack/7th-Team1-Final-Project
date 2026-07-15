// Copyright 2026 One Team. All rights reserved.

#include "NSBossArtilleryComponent.h"

#include "GameFramework/Actor.h"
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
