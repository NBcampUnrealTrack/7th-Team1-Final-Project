// Copyright 2026 One Team. All rights reserved.


#include "NSCommonUpgradeUtilityHelper.h"

#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "NeoSanctum/Data/CommonUpgrade/NSCommonUpgradeTypes.h"
#include "NeoSanctum/Debug/Logging/NSLogCategories.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"

const FName NSCommonUpgradeUtility::NodeId_CommonCurrencyGainRate(TEXT("CommonCurrencyGainRate"));
const FName NSCommonUpgradeUtility::NodeId_TempCurrencyGainRate(TEXT("TempCurrencyGainRate"));
const FName NSCommonUpgradeUtility::NodeId_PartRerollDiscount(TEXT("PartRerollDiscount"));
const FName NSCommonUpgradeUtility::NodeId_AugmentRerollDiscount(TEXT("AugmentRerollDiscount"));
const FName NSCommonUpgradeUtility::NodeId_PartShopDiscount(TEXT("PartShopDiscount"));
const FName NSCommonUpgradeUtility::NodeId_AugmentChoiceCount(TEXT("AugmentChoiceCount"));

double NSCommonUpgradeUtility::GetPercent(
	const UNSDataSubsystem* DataSubsystem, const UNSPlayerProgressComponent* ProgressComponent, FName NodeId)
{
	if (!DataSubsystem || !ProgressComponent)
	{
		return 0.0;
	}

	const FNSCommonUpgradeNodeRow* Row = DataSubsystem->GetCommonUpgradeNodeRow(NodeId);
	if (!Row)
	{
		// 기획자가 아직 DT_CommonUpgrade에 이 NodeId를 안 넣은 상태일 수 있어 Warning으로만 남김.
		NS_LOG(LogNS, Warning,
			"[CommonUpgradeUtility] Row를 찾을 수 없습니다. NodeId={NodeId}",
			("NodeId", NodeId.ToString())
		);
		return 0.0;
	}

	const int32 Level = ProgressComponent->GetCommonSkillLevel(NodeId);
	if (Level <= 0)
	{
		return 0.0;
	}

	if (Row->Operation != ENSCombatStatModifierOperation::Multiply)
	{
		NS_LOG(LogNS, Warning,
			"[CommonUpgradeUtility] Multiply가 아닌 Operation은 아직 지원하지 않습니다. NodeId={NodeId}",
			("NodeId", NodeId.ToString())
		);
		return 0.0;
	}

	return static_cast<double>(Row->ValuePerLevel) * Level;
}

int64 NSCommonUpgradeUtility::ApplyPercent(int64 Base, double Percent)
{
	if (Base <= 0 || Percent == 0.0)
	{
		return Base;
	}

	return FMath::FloorToInt64(static_cast<double>(Base) * (1.0 + Percent * 0.01));
}

int32 NSCommonUpgradeUtility::GetAddBonus(
	const UNSDataSubsystem* DataSubsystem, const UNSPlayerProgressComponent* ProgressComponent, FName NodeId)
{
	if (!DataSubsystem || !ProgressComponent)
	{
		return 0;
	}

	const FNSCommonUpgradeNodeRow* Row = DataSubsystem->GetCommonUpgradeNodeRow(NodeId);
	if (!Row)
	{
		// 기획자가 아직 DT_CommonUpgrade에 이 NodeId를 안 넣은 상태일 수 있어 Warning으로만 남김.
		NS_LOG(LogNS, Warning,
			"[CommonUpgradeUtility] Row를 찾을 수 없습니다. NodeId={NodeId}",
			("NodeId", NodeId.ToString())
		);
		return 0;
	}

	const int32 Level = ProgressComponent->GetCommonSkillLevel(NodeId);
	if (Level <= 0)
	{
		return 0;
	}

	if (Row->Operation != ENSCombatStatModifierOperation::Add)
	{
		NS_LOG(LogNS, Warning,
			"[CommonUpgradeUtility] Add가 아닌 Operation은 GetAddBonus에서 지원하지 않습니다. NodeId={NodeId}",
			("NodeId", NodeId.ToString())
		);
		return 0;
	}

	return FMath::FloorToInt32(Row->ValuePerLevel * Level);
}

int64 NSCommonUpgradeUtility::ApplyPercentAsCost(int64 Base, double Percent)
{
	// TrySpendTemp(0)은 무조건 실패라서, 할인이 세게 들어가도 최소 1은 받아야 함
	return FMath::Max<int64>(ApplyPercent(Base, Percent), 1);
}
