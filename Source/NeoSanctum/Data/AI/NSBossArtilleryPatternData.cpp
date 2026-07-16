// Copyright 2026 One Team. All rights reserved.


/*
 * 작성자 : 최준혁
- 
 * 파일 생성일 : 26.07.15
- 
 * 클래스 개요 : 보스 포격 패턴 DataAsset의 식별자 반환과 에디터 검증 구현
 * 잘못된 패턴 설정을 에디터 단계에서 발견하도록 데이터 검증을 담당
*/


#include "NSBossArtilleryPatternData.h"
#include "Misc/DataValidation.h"

FPrimaryAssetId UNSBossArtilleryPatternData::GetPrimaryAssetId() const
{
	// 포격 패턴 DataAsset을 AssetManager와 디버그에서 안정적으로 식별하기 위한 ID
	return FPrimaryAssetId(TEXT("BossArtilleryPattern"), GetFName());
}

#if WITH_EDITOR
EDataValidationResult UNSBossArtilleryPatternData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// 검증 실패 메시지를 추가하고 결과를 Invalid로 변경하는 로컬 헬퍼
	auto AddError = [&Context, &Result](const FString& Message)
	{
		Context.AddError(FText::FromString(Message));
		Result = EDataValidationResult::Invalid;
	};

	// 치명적이지 않지만 튜닝 확인이 필요한 경고 메시지를 추가하는 로컬 헬퍼
	auto AddWarning = [&Context](const FString& Message)
	{
		Context.AddWarning(FText::FromString(Message));
	};

	if (PatternId == ENSBossArtilleryPatternId::None)
	{
		AddError(TEXT("PatternId가 None입니다."));
	}

	if (SelectionData.BaseWeight <= 0.0f)
	{
		AddWarning(TEXT("BaseWeight가 0 이하입니다. 이 패턴은 가중치 선택에서 사실상 선택되지 않을 수 있습니다."));
	}

	for (int32 Index = 0; Index < SelectionData.RecentUseWeightMultipliers.Num(); ++Index)
	{
		if (SelectionData.RecentUseWeightMultipliers[Index] < 0.0f)
		{
			AddError(FString::Printf(TEXT("RecentUseWeightMultipliers[%d]가 0보다 작습니다."), Index));
		}
	}

	if (SelectionData.RepeatPolicy == ENSBossArtilleryRepeatPolicy::HardCooldownUses &&
		SelectionData.HardCooldownUseCount <= 0)
	{
		AddError(TEXT("HardCooldownUses 정책은 HardCooldownUseCount가 1 이상이어야 합니다."));
	}

	if (TargetData.TargetMode == ENSBossArtilleryTargetMode::None)
	{
		AddError(TEXT("TargetMode가 None입니다."));
	}

	if (TargetData.MaxCombatantCount > 0 &&
		TargetData.MaxCombatantCount < TargetData.MinCombatantCount)
	{
		AddError(TEXT("MaxCombatantCount가 MinCombatantCount보다 작습니다."));
	}

	if (TargetData.TargetMode == ENSBossArtilleryTargetMode::BetweenCombatants &&
		TargetData.MinCombatantCount < 2)
	{
		AddWarning(TEXT("BetweenCombatants 패턴은 보통 최소 전투 참여자 수가 2 이상이어야 의미가 있습니다."));
	}

	if (TargetData.TargetMode == ENSBossArtilleryTargetMode::BetweenCombatants &&
		TargetData.MaxPairCount <= 0)
	{
		AddError(TEXT("BetweenCombatants 패턴은 MaxPairCount가 1 이상이어야 합니다."));
	}

	if (TargetData.MaxPairDistance > 0.0f &&
		TargetData.MaxPairDistance < TargetData.MinPairDistance)
	{
		AddError(TEXT("MaxPairDistance가 MinPairDistance보다 작습니다."));
	}

	if (ShotBudgetData.BudgetMode == ENSBossArtilleryShotBudgetMode::None)
	{
		AddError(TEXT("BudgetMode가 None입니다."));
	}

	if (ShotBudgetData.MaxTotalShots <= 0)
	{
		AddError(TEXT("MaxTotalShots는 1 이상이어야 합니다."));
	}

	if (ShotBudgetData.BudgetMode == ENSBossArtilleryShotBudgetMode::PerTarget &&
		ShotBudgetData.ShotsPerTarget <= 0)
	{
		AddError(TEXT("PerTarget 방식은 ShotsPerTarget이 1 이상이어야 합니다."));
	}

	if (ShotBudgetData.BudgetMode == ENSBossArtilleryShotBudgetMode::FixedTotal &&
		ShotBudgetData.FixedTotalShots <= 0)
	{
		AddError(TEXT("FixedTotal 방식은 FixedTotalShots가 1 이상이어야 합니다."));
	}

	if (ShotBudgetData.BudgetMode == ENSBossArtilleryShotBudgetMode::PerRing &&
		(ShotBudgetData.RingCount <= 0 || ShotBudgetData.ShotsPerRing <= 0))
	{
		AddError(TEXT("PerRing 방식은 RingCount와 ShotsPerRing이 모두 1 이상이어야 합니다."));
	}

	if (ShotBudgetData.BudgetMode == ENSBossArtilleryShotBudgetMode::BetweenCombatants &&
		ShotBudgetData.ShotsPerPair <= 0)
	{
		AddError(TEXT("BetweenCombatants 발수 방식은 ShotsPerPair가 1 이상이어야 합니다."));
	}

	if (PlacementData.PlacementMode == ENSBossArtilleryPlacementMode::None)
	{
		AddError(TEXT("PlacementMode가 None입니다."));
	}

	if ((PlacementData.PlacementMode == ENSBossArtilleryPlacementMode::Ring ||
			PlacementData.PlacementMode == ENSBossArtilleryPlacementMode::WaveRings) &&
		PlacementData.RingStartRadius <= 0.0f)
	{
		AddError(TEXT("Ring 또는 WaveRings 배치는 RingStartRadius가 0보다 커야 합니다."));
	}

	if (PlacementData.PlacementMode == ENSBossArtilleryPlacementMode::WaveRings &&
		PlacementData.RingSpacing <= 0.0f)
	{
		AddError(TEXT("WaveRings 배치는 RingSpacing이 0보다 커야 합니다."));
	}

	if (TimingData.TimingMode == ENSBossArtilleryTimingMode::None)
	{
		AddError(TEXT("TimingMode가 None입니다."));
	}

	if (TimingData.MaxRandomDelay < TimingData.MinRandomDelay)
	{
		AddError(TEXT("MaxRandomDelay가 MinRandomDelay보다 작습니다."));
	}

	if (TimingData.TimingMode == ENSBossArtilleryTimingMode::Burst &&
		TimingData.ShotsPerBurst <= 0)
	{
		AddError(TEXT("Burst 타이밍은 ShotsPerBurst가 1 이상이어야 합니다."));
	}

	if (TimingData.TimingMode == ENSBossArtilleryTimingMode::Wave &&
		TimingData.WaveSpeed <= 0.0f)
	{
		AddError(TEXT("Wave 타이밍은 WaveSpeed가 0보다 커야 합니다."));
	}

	if (TimingData.TimingMode == ENSBossArtilleryTimingMode::OffBeat &&
		TimingData.OffBeatExtraDelays.IsEmpty())
	{
		AddError(TEXT("OffBeat 타이밍은 OffBeatExtraDelays가 하나 이상 필요합니다."));
	}

	for (int32 Index = 0; Index < TimingData.OffBeatExtraDelays.Num(); ++Index)
	{
		if (TimingData.OffBeatExtraDelays[Index] < 0.0f)
		{
			AddError(FString::Printf(TEXT("OffBeatExtraDelays[%d]가 0보다 작습니다."), Index));
		}
	}

	if (DamageData.DamageRadius <= 0.0f)
	{
		AddError(TEXT("DamageRadius는 0보다 커야 합니다."));
	}

	if (DamageData.DamageScale <= 0.0f)
	{
		AddWarning(TEXT("DamageScale이 0 이하입니다. 이 패턴은 피해를 주지 않는 연출용 패턴처럼 동작할 수 있습니다."));
	}

	if (PatternId == ENSBossArtilleryPatternId::PatternC_Separation &&
		TargetData.TargetMode != ENSBossArtilleryTargetMode::BetweenCombatants)
	{
		AddWarning(TEXT("Pattern C는 BetweenCombatants TargetMode와 함께 사용할 때 의도가 가장 명확합니다."));
	}

	if (PatternId == ENSBossArtilleryPatternId::PatternD_Wave &&
		ShotBudgetData.BudgetMode != ENSBossArtilleryShotBudgetMode::PerRing)
	{
		AddWarning(TEXT("Pattern D는 PerRing 발수 계산 방식과 함께 사용할 때 파동 포격 구성이 쉽습니다."));
	}

	if (PatternId == ENSBossArtilleryPatternId::PatternD_Wave &&
		TimingData.TimingMode != ENSBossArtilleryTimingMode::Wave)
	{
		AddWarning(TEXT("Pattern D는 Wave 타이밍과 함께 사용할 때 의도가 가장 명확합니다."));
	}

	return Result;
}
#endif
