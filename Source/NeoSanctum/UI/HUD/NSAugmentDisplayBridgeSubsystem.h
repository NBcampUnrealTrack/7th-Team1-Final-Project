// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NeoSanctum/Type/NSAugmentDisplayTypes.h"
#include "NSAugmentDisplayBridgeSubsystem.generated.h"

class UDataTable;
class UNSAugmentDefinition;


/**
 * 증강 시스템의 실제 값과 증강 DataAsset의 표시 문장을 조합해
 * 카드 UI용 ViewData를 생성하는 Bridge입니다.
 */
UCLASS()
class NEOSANCTUM_API UNSAugmentDisplayBridgeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	/**
	 * 증강 식별자와 현재 상태를 카드 UI용 ViewData로 변환
	 */
	bool TryBuildCardViewData(
		const FPrimaryAssetId& DefId,
		ENSAugmentRarity Rarity,
		int32 CurrentStack,
		FNSAugmentCardViewData& OutViewData
		) const;

private:
	//DataTable에서 지정한 Definition을 사용하는 모든 Row를 수집
	bool TryCollectDefinitionRows(
		const UDataTable* DefinitionTable,
		const UNSAugmentDefinition* Definition,
		TArray<const FNSAugmentDefinitionRow*>& OutRows
	) const;

	//DA의 Description 포맷에 실제 수치를 넣는다
	FText FormatDescription(
		const FText& DescriptionFormat,
		const TArray<const FNSAugmentDefinitionRow*>& Rows
	) const;

	//CombatStat.Damage 같은 태그에서 Damage 포맷 인자 이름을 만듦
	static FName MakeFormatArgumentName(
	const FGameplayTag& StatTag
	);

	//Add/Multiply의 입력 규칙에 맞는 표시용 절댓값을 만듦
	static FText MakeDisplayValue(
	float ValuePerStack,
	ENSCombatStatModifierOperation Operation
	);
};
