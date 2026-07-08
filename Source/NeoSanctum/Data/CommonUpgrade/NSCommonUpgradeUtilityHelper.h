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
	extern const FName NodeId_AugmentChoiceCount;

	// Row.ValuePerLevel * 현재 레벨(부호 있는 퍼센트). Row 없음 / 레벨 0 / Operation != Multiply면 0.0
	double GetPercent(
		const UNSDataSubsystem* DataSubsystem, const UNSPlayerProgressComponent* ProgressComponent, FName NodeId);

	// Base에 Percent를 적용한 최종값. Base<=0이거나 Percent==0이면 Base 그대로. 항상 내림 처리.
	int64 ApplyPercent(int64 Base, double Percent);

	// Row.ValuePerLevel * 현재 레벨(정수 가산치). Row 없음 / 레벨 0 / Operation != Add면 0.
	int32 GetAddBonus(
		const UNSDataSubsystem* DataSubsystem, const UNSPlayerProgressComponent* ProgressComponent, FName NodeId);

	// 리롤/구매 같은 "비용" 전용. ApplyPercent 결과를 최소 1로 클램프(원가가 원래 양수였을 때만).
	int64 ApplyPercentAsCost(int64 Base, double Percent);
}
