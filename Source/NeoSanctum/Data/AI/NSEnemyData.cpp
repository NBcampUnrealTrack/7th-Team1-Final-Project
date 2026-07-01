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
			return A.HPThreshold > B.HPThreshold;
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

const FNSEnemyAttackRow* UNSEnemyData::FindAttackRow(FName AttackId) const
{
	if (AttackId.IsNone())
	{
		return nullptr;
	}

	CacheAttackRows();

	for (const FNSEnemyAttackRow* Row : CachedAttackRows)
	{
		if (Row && Row->AttackId == AttackId)
		{
			return Row;
		}
	}

	return nullptr;
}

FNSEnemyAttackRow UNSEnemyData::ResolveAttackRow(FName AttackId) const
{
	if (const FNSEnemyAttackRow* Row = FindAttackRow(AttackId))
	{
		return *Row;
	}

	FNSEnemyAttackRow DefaultRow;
	DefaultRow.EnemyId = EnemyId;
	DefaultRow.AttackId = AttackId;
	return DefaultRow;
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
