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

void UNSEnemyData::CachePartRows() const
{
	if (bPartRowsCached)
	{
		return;
	}

	bPartRowsCached = true;
	CachedPartRows.Reset();

	if (!PartsTable || !EnemyId.IsValid())
	{
		return;
	}

	if (PartsTable->GetRowStruct() != FNSEnemyPartRow::StaticStruct())
	{
		return;
	}

	TArray<FNSEnemyPartRow*> Rows;
	PartsTable->GetAllRows<FNSEnemyPartRow>(TEXT("EnemyPartRows"), Rows);

	TSet<FName> SeenPartIds;

	for (const FNSEnemyPartRow* Row : Rows)
	{
		if (!Row || Row->EnemyId != EnemyId || Row->PartId.IsNone())
		{
			continue;
		}

		if (SeenPartIds.Contains(Row->PartId))
		{
			continue;
		}

		SeenPartIds.Add(Row->PartId);
		CachedPartRows.Add(Row);
	}
}

void UNSEnemyData::InvalidateCachedRows() const
{
	CachedAttackRows.Reset();
	CachedPhaseRows.Reset();
	CachedPartRows.Reset();

	bAttackRowsCached = false;
	bPhaseRowsCached = false;
	bPartRowsCached = false;
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

const TArray<const FNSEnemyPartRow*>& UNSEnemyData::GetPartRows() const
{
	CachePartRows();
	return CachedPartRows;
}

bool UNSEnemyData::HasPartRows() const
{
	return !GetPartRows().IsEmpty();
}

const FNSEnemyPartRow* UNSEnemyData::FindPartRowById(FName PartId) const
{
	if (PartId.IsNone())
	{
		return nullptr;
	}

	for (const FNSEnemyPartRow* PartRow : GetPartRows())
	{
		if (PartRow && PartRow->PartId == PartId)
		{
			return PartRow;
		}
	}

	return nullptr;
}

void UNSEnemyData::GetPartRowsByAttackId(
	FName AttackId,
	TArray<const FNSEnemyPartRow*>& OutPartRows) const
{
	OutPartRows.Reset();

	if (AttackId.IsNone())
	{
		return;
	}

	for (const FNSEnemyPartRow* PartRow : GetPartRows())
	{
		if (PartRow && PartRow->AttackIds.Contains(AttackId))
		{
			OutPartRows.Add(PartRow);
		}
	}
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

float UNSEnemyData::GetPhaseAttackWeight(
	const FNSEnemyAttackRow& AttackRow,
	float HealthRatio) const
{
	const FNSEnemyPhaseRow* PhaseRow = FindPhaseRowByHealthRatio(HealthRatio);
	if (!PhaseRow)
	{
		return FMath::Max(AttackRow.Weight, 0.0f);
	}

	return GetPhaseAttackOverrideValue(
		PhaseRow->WeightOverrides,
		AttackRow.AttackId,
		AttackRow.Weight);
}

float UNSEnemyData::GetPhaseAttackCooldown(
	const FNSEnemyAttackRow& AttackRow,
	float HealthRatio) const
{
	const FNSEnemyPhaseRow* PhaseRow = FindPhaseRowByHealthRatio(HealthRatio);
	if (!PhaseRow)
	{
		return FMath::Max(AttackRow.Cooldown, 0.0f);
	}

	return GetPhaseAttackOverrideValue(
		PhaseRow->CooldownOverrides,
		AttackRow.AttackId,
		AttackRow.Cooldown);
}

float UNSEnemyData::GetPhaseAttackOverrideValue(
	const TArray<FNSEnemyAttackValue>& Overrides,
	FName AttackId,
	float DefaultValue) const
{
	const float ClampedDefaultValue = FMath::Max(DefaultValue, 0.0f);

	if (AttackId.IsNone())
	{
		return ClampedDefaultValue;
	}

	for (const FNSEnemyAttackValue& Override : Overrides)
	{
		if (Override.AttackId == AttackId)
		{
			return FMath::Max(Override.Value, 0.0f);
		}
	}

	return ClampedDefaultValue;
}
