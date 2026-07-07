// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UNSPlayerProgressComponent;
class UNSDataSubsystem;
/**
 * 공용 업그레이드 유틸 4종의 NodeId와, Row+레벨을 조합해 퍼센트를 계산하는 최소 조회 헬퍼.
 */
namespace NSCommonUpgradeUtility
{
	extern const FName NodeId_CommonCurrencyGainRate;
	extern const FName NodeId_TempCurrencyGainRate;
	extern const FName NodeId_PartRerollDiscount;
	extern const FName NodeId_AugmentRerollDiscount;
	extern const FName NodeId_PartShopDiscount;

	// Row.ValuePerLevel * 현재 레벨(부호 있는 퍼센트). Row 없음 / 레벨 0 / Operation != Multiply면 0.0
	double GetPercent(
		const UNSDataSubsystem* DataSubsystem, const UNSPlayerProgressComponent* ProgressComponent, FName NodeId);
}
