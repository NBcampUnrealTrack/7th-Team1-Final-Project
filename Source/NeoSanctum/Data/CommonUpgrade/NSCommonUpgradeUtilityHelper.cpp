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
