// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyData.h"

const FNSEnemyAttackRow* UNSEnemyData::FindAttackRow(FName AttackId) const
{
	if (AttackId.IsNone() || !AttackTable)
	{
		return nullptr;
	}

	if (const FNSEnemyAttackRow* DirectRow =
		AttackTable->FindRow<FNSEnemyAttackRow>(AttackId, TEXT("EnemyAttackRow"), false))
	{
		return DirectRow;
	}

	TArray<FNSEnemyAttackRow*> Rows;
	AttackTable->GetAllRows<FNSEnemyAttackRow>(TEXT("EnemyAttackRows"), Rows);

	for (const FNSEnemyAttackRow* Row : Rows)
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
	DefaultRow.AttackId = AttackId;
	return DefaultRow;
}
