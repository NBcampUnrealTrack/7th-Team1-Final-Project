// Copyright 2026 One Team. All rights reserved.


#include "NSRunConfig.h"

#include "Misc/DataValidation.h"
#include "NeoSanctum/Data/Augment/NSAugmentRarityRuleSet.h"

FPrimaryAssetId UNSRunConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("NSRunConfig"), GetFName());
}

#if WITH_EDITOR
EDataValidationResult UNSRunConfig::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	TFunction<void(const FString&)> AddError = [&Context, &Result](const FString& Message)
	{
		Context.AddError(FText::FromString(Message));
		Result = EDataValidationResult::Invalid;
	};
	TFunction<void(const FString&)> AddWarning = [&Context](const FString& Message)
	{
		Context.AddWarning(FText::FromString(Message));
	};

	// Reward.Trigger 자체는 네이티브로 등록되지 않았지만, 자식 태그(Reward.Trigger.LevelUp 등)가
	// 등록되면 상위 태그가 트리에 자동 등록되므로 이 방식으로 조회 가능.
	static const FGameplayTag RewardTriggerRoot =
		FGameplayTag::RequestGameplayTag(FName(TEXT("Reward.Trigger")), false);

	TSet<FGameplayTag> SeenTriggerTags;
	for (int32 Index = 0; Index < AugmentRerollRules.Num(); ++Index)
	{
		const FNSAugmentRerollRule& Rule = AugmentRerollRules[Index];

		if (!Rule.RewardTriggerTag.IsValid())
		{
			AddError(FString::Printf(TEXT("AugmentRerollRules[%d]의 RewardTriggerTag가 비어 있습니다."),Index));
		}
		else
		{
			if (RewardTriggerRoot.IsValid() && !Rule.RewardTriggerTag.MatchesTag(RewardTriggerRoot))
			{
				AddError(FString::Printf(
					TEXT("AugmentRerollRules[%d]의 RewardTriggerTag(%s)가 Reward.Trigger 하위 태그가 아닙니다."),
					Index, *Rule.RewardTriggerTag.ToString())
				);
			}

			if (SeenTriggerTags.Contains(Rule.RewardTriggerTag))
			{
				AddError(FString::Printf(
					TEXT("AugmentRerollRules에 RewardTriggerTag(%s)가 중복됩니다."),
					*Rule.RewardTriggerTag.ToString())
				);
			}
			else
			{
				SeenTriggerTags.Add(Rule.RewardTriggerTag);
			}
		}

		if (Rule.InitialCost < 1)
		{
			AddError(FString::Printf(
				TEXT("AugmentRerollRules[%d]의 InitialCost는 1 이상이어야 합니다(TrySpendTemp(0) 실패 모순)."), Index));
		}

		if (!FMath::IsFinite(Rule.CostMultiplier) || Rule.CostMultiplier < 1.0f)
		{
			AddError(FString::Printf(
				TEXT("AugmentRerollRules[%d]의 CostMultiplier는 finite이며 1.0 이상이어야 합니다."), Index));
		}
	}

	// 비대칭 교차검증: RerollRule은 있으나 RarityRuleSet에 대응 트리거가 없으면 실제로 쓰이지 않는 규칙이라는 경고만 남긴다.
	// 역방향(RarityRuleSet에는 있으나 RerollRule 없음)은 "리롤 불가" 정책이므로 검증 대상이 아님.
	if (!AugmentRarityRuleSet.IsNull())
	{
		// IsDataValid는 에디터 저장/검증 시점에 실행되어 살아있는 GameInstance/NSDataSubsystem이 없을 수 있으므로,
		// 런타임 로드 경로에 기대지 않고 직접 동기 로드해 검증.
		if (const UNSAugmentRarityRuleSet* LoadedRarityRuleSet = AugmentRarityRuleSet.LoadSynchronous())
		{
			TSet<FGameplayTag> RarityTriggerTags;
			for (const FNSAugmentRarityRule& RarityRule : LoadedRarityRuleSet->RarityRules)
			{
				RarityTriggerTags.Add(RarityRule.RewardTriggerTag);
			}

			for (const FNSAugmentRerollRule& Rule : AugmentRerollRules)
			{
				if (Rule.RewardTriggerTag.IsValid() && !RarityTriggerTags.Contains(Rule.RewardTriggerTag))
				{
					AddWarning(FString::Printf(
						TEXT("AugmentRerollRules의 트리거(%s)가 AugmentRarityRuleSet에 없어 오퍼가 생성되지 않으므로 이 리롤 규칙은 사용되지 않습니다."),
						*Rule.RewardTriggerTag.ToString())
					);
				}
			}
		}
	}

	return Result;
}
#endif
