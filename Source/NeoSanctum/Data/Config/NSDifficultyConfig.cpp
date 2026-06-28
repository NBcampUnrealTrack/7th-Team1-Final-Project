// Copyright 2026 One Team. All rights reserved.


#include "NSDifficultyConfig.h"
#include "Curves/CurveFloat.h"


FNSDifficultyScale UNSDifficultyConfig::Evaluate(float ElapsedSeconds, int32 StageNumber, int32 PlayerCount) const
{
	float TimeAddPct;
	if (TimeMultiplierCurve)
	{
		TimeAddPct = TimeMultiplierCurve->GetFloatValue(ElapsedSeconds);
	}
	else
	{
		const int32 Steps = FMath::FloorToInt(
			ElapsedSeconds / FMath::Max(TimeStepInterval,
				1.0f));
		TimeAddPct = Steps * TimeIncreasePerStep;    
	}
	
	const int32 StageStep   = FMath::Max(0, StageNumber - 1);
	const float StageAddPct = StageStep * StageIncreasePerStage;

	FNSDifficultyScale Out;
	// 같은 증가분끼리 더한 뒤 곱함
	Out.Multiply = 1.0f + TimeAddPct + StageAddPct;       
	// 값이 너무 커질 것을 대비한 상한 (필요없으면 삭제해도 됨)
	if (MaxMultiply > 0.0f)
	{
		Out.Multiply = FMath::Min(Out.Multiply, MaxMultiply); 
	}

	const int32 ExtraPlayers = FMath::Max(0, PlayerCount - 1);
	// 인원 비율 계산
	Out.HealthAddRatio = ExtraPlayers * PlayerHealthAddRatioPerExtra;  
	Out.DamageAddRatio = ExtraPlayers * PlayerDamageAddRatioPerExtra;
	Out.DefenseAddRatio = ExtraPlayers * PlayerDefenseAddRatioPerExtra;
	
	return Out;
}
