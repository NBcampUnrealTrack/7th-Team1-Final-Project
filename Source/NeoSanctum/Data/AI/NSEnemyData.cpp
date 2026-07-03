// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyData.h"

#include "Misc/DataValidation.h"



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

#if WITH_EDITOR
void UNSEnemyData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	InvalidateCachedRows();
}

EDataValidationResult FNSEnemyPartRow::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = FTableRowBase::IsDataValid(Context);

	auto AddError = [&Context, &Result](const FString& Message)
	{
		Context.AddError(FText::FromString(Message));
		Result = EDataValidationResult::Invalid;
	};

	auto AddWarning = [&Context](const FString& Message)
	{
		Context.AddWarning(FText::FromString(Message));
	};

	if (!EnemyId.IsValid())
	{
		AddError(TEXT("PartRow EnemyId가 비어 있음"));
	}

	if (PartId.IsNone())
	{
		AddError(TEXT("PartRow PartId가 비어 있음"));
	}

	TSet<FName> SeenAttackIds;
	for (const FName AttackId : AttackIds)
	{
		if (AttackId.IsNone())
		{
			AddError(FString::Printf(TEXT("PartId=%s AttackIds에 None 값 포함"), *PartId.ToString()));
			continue;
		}

		if (SeenAttackIds.Contains(AttackId))
		{
			AddError(FString::Printf(
				TEXT("PartId=%s AttackIds에 중복 값 포함: %s"),
				*PartId.ToString(),
				*AttackId.ToString()));
		}

		SeenAttackIds.Add(AttackId);
	}

	const bool bHasOnlyOneTracePairSocket =
		TraceStartSocket.IsNone() != TraceEndSocket.IsNone();

	if (bHasOnlyOneTracePairSocket)
	{
		AddError(FString::Printf(
			TEXT("PartId=%s TraceStartSocket과 TraceEndSocket은 함께 입력 필요"),
			*PartId.ToString()));
	}

	if (bUseLeftHandIKWhileEquipped && LeftHandIKSocket.IsNone())
	{
		AddError(FString::Printf(
			TEXT("PartId=%s LeftHand IK 사용 시 LeftHandIKSocket 필요"),
			*PartId.ToString()));
	}

	if ((YawLimit > 0.0f || PitchLimit > 0.0f) &&
		AimBone.IsNone() &&
		AimControl.IsNone())
	{
		AddWarning(FString::Printf(
			TEXT("PartId=%s 회전 제한값이 있지만 AimBone/AimControl이 모두 비어 있음"),
			*PartId.ToString()));
	}

	switch (PartType)
	{
	case ENSEnemyPartType::SpawnedWeapon:
	case ENSEnemyPartType::SpawnedPart:
		if (!ActorClass)
		{
			AddError(FString::Printf(
				TEXT("PartId=%s Spawned 타입은 ActorClass 필요"),
				*PartId.ToString()));
		}

		if (AttachSocket.IsNone())
		{
			AddError(FString::Printf(
				TEXT("PartId=%s Spawned 타입은 AttachSocket 필요"),
				*PartId.ToString()));
		}
		break;

	case ENSEnemyPartType::IntegratedWeapon:
		if (ActorClass)
		{
			AddError(FString::Printf(
				TEXT("PartId=%s IntegratedWeapon은 ActorClass를 사용하지 않음"),
				*PartId.ToString()));
		}
		break;

	case ENSEnemyPartType::VisualOnly:
		if (ActorClass)
		{
			AddError(FString::Printf(
				TEXT("PartId=%s VisualOnly는 ActorClass를 스폰하지 않음. Actor가 필요하면 SpawnedPart 사용"),
				*PartId.ToString()));
		}
		break;

	default:
		break;
	}

	return Result;
}

EDataValidationResult UNSEnemyData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	auto AddError = [&Context, &Result](const FString& Message)
	{
		Context.AddError(FText::FromString(Message));
		Result = EDataValidationResult::Invalid;
	};

	auto AddWarning = [&Context](const FString& Message)
	{
		Context.AddWarning(FText::FromString(Message));
	};

	if (!EnemyId.IsValid())
	{
		AddError(TEXT("EnemyData EnemyId가 비어 있음"));
		return Result;
	}

	if (!PartsTable)
	{
		AddWarning(TEXT("PartsTable이 비어 있음. Part 기반 공격 위치를 사용하지 않는 Enemy라면 무시 가능"));
		return Result;
	}

	if (PartsTable->GetRowStruct() != FNSEnemyPartRow::StaticStruct())
	{
		AddError(TEXT("PartsTable RowStruct가 FNSEnemyPartRow가 아님"));
		return Result;
	}

	TArray<FNSEnemyPartRow*> PartRows;
	PartsTable->GetAllRows<FNSEnemyPartRow>(TEXT("EnemyPartRowsValidation"), PartRows);

	TMap<FName, const FNSEnemyPartRow*> PartById;
	TMultiMap<FName, const FNSEnemyPartRow*> PartsByAttackId;

	for (const FNSEnemyPartRow* PartRow : PartRows)
	{
		if (!PartRow || PartRow->EnemyId != EnemyId)
		{
			continue;
		}

		if (PartRow->PartId.IsNone())
		{
			continue;
		}

		if (PartById.Contains(PartRow->PartId))
		{
			AddError(FString::Printf(
				TEXT("EnemyId=%s PartId 중복: %s"),
				*EnemyId.ToString(),
				*PartRow->PartId.ToString()));
		}

		PartById.Add(PartRow->PartId, PartRow);

		for (const FName AttackId : PartRow->AttackIds)
		{
			if (!AttackId.IsNone())
			{
				PartsByAttackId.Add(AttackId, PartRow);
			}
		}
	}

	if (!AttackTable)
	{
		if (PartsByAttackId.Num() > 0)
		{
			AddWarning(TEXT("PartRow에 AttackIds가 있지만 AttackTable이 없어 교차 검증 불가"));
		}

		return Result;
	}

	if (AttackTable->GetRowStruct() != FNSEnemyAttackRow::StaticStruct())
	{
		AddError(TEXT("AttackTable RowStruct가 FNSEnemyAttackRow가 아님"));
		return Result;
	}

	TArray<FNSEnemyAttackRow*> AttackRows;
	AttackTable->GetAllRows<FNSEnemyAttackRow>(TEXT("EnemyAttackRowsValidation"), AttackRows);

	TMap<FName, const FNSEnemyAttackRow*> AttackById;

	for (const FNSEnemyAttackRow* AttackRow : AttackRows)
	{
		if (!AttackRow || AttackRow->EnemyId != EnemyId || AttackRow->AttackId.IsNone())
		{
			continue;
		}

		if (AttackById.Contains(AttackRow->AttackId))
		{
			AddError(FString::Printf(
				TEXT("EnemyId=%s AttackId 중복: %s"),
				*EnemyId.ToString(),
				*AttackRow->AttackId.ToString()));
		}

		AttackById.Add(AttackRow->AttackId, AttackRow);
	}

	for (const TPair<FName, const FNSEnemyPartRow*>& Pair : PartsByAttackId)
	{
		if (!AttackById.Contains(Pair.Key))
		{
			AddError(FString::Printf(
				TEXT("PartId=%s 존재하지 않는 AttackId 참조: %s"),
				*Pair.Value->PartId.ToString(),
				*Pair.Key.ToString()));
		}
	}

	for (const TPair<FName, const FNSEnemyAttackRow*>& Pair : AttackById)
	{
		const FName AttackId = Pair.Key;
		const FNSEnemyAttackRow* AttackRow = Pair.Value;

		TArray<const FNSEnemyPartRow*> LinkedParts;
		PartsByAttackId.MultiFind(AttackId, LinkedParts);

		if (AttackRow->AttackType == ENSEnemyAttackType::Projectile ||
			AttackRow->AttackType == ENSEnemyAttackType::Hitscan)
		{
			bool bHasMuzzleSocket = false;

			for (const FNSEnemyPartRow* PartRow : LinkedParts)
			{
				if (PartRow && !PartRow->MuzzleSocket.IsNone())
				{
					bHasMuzzleSocket = true;
					break;
				}
			}

			if (!bHasMuzzleSocket)
			{
				AddError(FString::Printf(
					TEXT("AttackId=%s Projectile/Hitscan 공격은 MuzzleSocket이 있는 PartRow 필요"),
					*AttackId.ToString()));
			}
		}

		if (AttackRow->AttackType == ENSEnemyAttackType::MeleeSweep)
		{
			bool bHasTraceSocket = false;

			for (const FNSEnemyPartRow* PartRow : LinkedParts)
			{
				if (!PartRow)
				{
					continue;
				}

				const bool bHasTracePair =
					!PartRow->TraceStartSocket.IsNone() &&
					!PartRow->TraceEndSocket.IsNone();

				if (bHasTracePair || !PartRow->TraceSocket.IsNone())
				{
					bHasTraceSocket = true;
					break;
				}
			}

			if (!LinkedParts.IsEmpty() && !bHasTraceSocket)
			{
				AddWarning(FString::Printf(
					TEXT("AttackId=%s MeleeSweep 공격에 연결된 PartRow가 있지만 TraceSocket 정보가 없음. Actor Forward fallback 사용"),
					*AttackId.ToString()));
			}
		}
	}

	return Result;
}
#endif