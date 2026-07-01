// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyData.h"

void UNSEnemyData::CacheAttackRows() const
{
	if (bAttackRowsCached)
	{
		return;
	}

	bAttackRowsCached = true;
	CachedAttackRows.Reset();

	if (!AttackTable || !EnemyId.IsValid())
	{
		return;
	}

	if (AttackTable->GetRowStruct() != FNSEnemyAttackRow::StaticStruct())
	{
		return;
	}

	TArray<FNSEnemyAttackRow*> Rows;
	AttackTable->GetAllRows<FNSEnemyAttackRow>(TEXT("EnemyAttackRows"), Rows);

	TSet<FName> SeenAttackIds;

	for (const FNSEnemyAttackRow* Row : Rows)
	{
		if (!Row || Row->EnemyId != EnemyId || Row->AttackId.IsNone())
		{
			continue;
		}

		if (SeenAttackIds.Contains(Row->AttackId))
		{
			continue;
		}

		SeenAttackIds.Add(Row->AttackId);
		CachedAttackRows.Add(Row);
	}
}

void UNSEnemyData::CachePhaseRows() const
{
	if (bPhaseRowsCached)
	{
		return;
	}

	bPhaseRowsCached = true;
	CachedPhaseRows.Reset();

	if (!PhaseTable || !EnemyId.IsValid())
	{
		return;
	}

	if (PhaseTable->GetRowStruct() != FNSEnemyPhaseRow::StaticStruct())
	{
		return;
	}

	TArray<FNSEnemyPhaseRow*> Rows;
	PhaseTable->GetAllRows<FNSEnemyPhaseRow>(TEXT("EnemyPhaseRows"), Rows);

	for (const FNSEnemyPhaseRow* Row : Rows)
	{
		if (!Row || Row->EnemyId != EnemyId || Row->PhaseId.IsNone())
		{
			continue;
		}

		CachedPhaseRows.Add(Row);
	}

	CachedPhaseRows.Sort(
		[](const FNSEnemyPhaseRow& A, const FNSEnemyPhaseRow& B)
		{
			return A.HPThreshold < B.HPThreshold;
		});
}

void UNSEnemyData::InvalidateCachedRows() const
{
	CachedAttackRows.Reset();
	CachedPhaseRows.Reset();

	bAttackRowsCached = false;
	bPhaseRowsCached = false;
}

const TArray<const FNSEnemyAttackRow*>& UNSEnemyData::GetAttackRows() const
{
	CacheAttackRows();
	return CachedAttackRows;
}

const TArray<const FNSEnemyPhaseRow*>& UNSEnemyData::GetPhaseRows() const
{
	CachePhaseRows();
	return CachedPhaseRows;
}

#if WITH_EDITOR
void UNSEnemyData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	InvalidateCachedRows();
}
#endif

const FNSEnemyPhaseRow* UNSEnemyData::FindPhaseRowByHealthRatio(float HealthRatio) const
{
	const TArray<const FNSEnemyPhaseRow*>& PhaseRows = GetPhaseRows();

	if (PhaseRows.IsEmpty())
	{
		return nullptr;
	}

	const float ClampedHealthRatio = FMath::Clamp(HealthRatio, 0.0f, 1.0f);

	for (const FNSEnemyPhaseRow* PhaseRow : PhaseRows)
	{
		if (PhaseRow && ClampedHealthRatio <= PhaseRow->HPThreshold)
		{
			return PhaseRow;
		}
	}

	return PhaseRows.Last();
}

bool UNSEnemyData::IsAttackAllowedByPhase(FName AttackId, float HealthRatio) const
{
	if (AttackId.IsNone())
	{
		return false;
	}

	const FNSEnemyPhaseRow* PhaseRow = FindPhaseRowByHealthRatio(HealthRatio);

	// Phase Row가 없으면 Phase 시스템을 사용하지 않는 몬스터로 보고 모든 공격을 허용함
	if (!PhaseRow)
	{
		return true;
	}

	return PhaseRow->AttackIds.Contains(AttackId);
}

