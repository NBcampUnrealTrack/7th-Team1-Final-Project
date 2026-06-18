// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSRewardTypes.h"
#include "UObject/Object.h"
#include "NSRewardDropResolver.generated.h"

struct FNSRewardDropResult;

/**
 * 보상 드랍 테이블에서 그룹별 드랍 결과를 산출하는 클래스
 */
UCLASS()
class NEOSANCTUM_API UNSRewardDropResolver : public UObject
{
	GENERATED_BODY()
	
public:
	static void ResolveDropResultsFromTable(
		const UDataTable* DropTable, FRandomStream& RandomStream, TArray<FNSRewardDropResult>& OutResults);
	
private:
	static bool IsValidDropRow(const FNSRewardDropRow& Row);
	
	static const FNSRewardDropRow* SelectDropRow(
		const TArray<const FNSRewardDropRow*>& Rows, 
		FRandomStream& RandomStream
	);
	
	static void ApplyDropRowToResult(
		const FNSRewardDropRow& Row, FRandomStream& RandomStream, FNSRewardDropResult& OutResult);
};
