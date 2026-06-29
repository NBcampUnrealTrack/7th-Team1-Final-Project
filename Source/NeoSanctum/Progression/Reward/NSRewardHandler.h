// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NeoSanctum/Data/Progression/Drop/NSDropLaunchData.h"
#include "NeoSanctum/Data/Reward/NSRewardTypes.h"
#include "UObject/Object.h"
#include "NSRewardHandler.generated.h"

class UNSRewardTriggerData;
class ANSDroppedPart;
class UNSRewardDataRegistry;

/**
 * Reward Trigger를 실제 보상 처리 흐름으로 연결하는 실행 담당 클래스
 */
UCLASS()
class NEOSANCTUM_API UNSRewardHandler : public UObject
{
	GENERATED_BODY()
	
public:
	// 서버 권한에서 Reward Trigger를 처리하고, 보상 타입별 시스템으로 분기
	static void HandleRewardTrigger(
		UWorld* World,
		const UNSRewardDataRegistry* RewardDataRegistry,
		const FGameplayTag& TriggerTag,
		const FVector& DropLocation,
		FRandomStream& RandomStream,
		TSubclassOf<ANSDroppedPart> DroppedPartClass,
		float CurrencyDropDuration
	);
	
private:
	static void HandleRewardEntries(
		UWorld* World,
		const UNSRewardTriggerData& RewardTriggerData, 
		const FGameplayTag& TriggerTag
	);
	
	static void HandleDropResults(
		UWorld* World,
		const TArray<FNSRewardDropResult>& DropResults,
		const FVector& DropLocation,
		FRandomStream& RandomStream,
		TSubclassOf<ANSDroppedPart> DroppedPartClass,
		float CurrencyDropDuration
	);
	
	static FNSDropLaunchData MakeDropLaunchData(
		UWorld* World,
		const FVector& Origin,
		FRandomStream& RandomStream
	);
	
	static bool TryFindDropGroundLocation(
		UWorld* World,
		const FVector& CandidateTargetLocation,
		FVector& OutGroundLocation
	);
	
	static void HandleAugmentRewardEntry(
		UWorld* World,
		const FGameplayTag& TriggerTag
	);
	
	static void HandleCurrencyDropResult(
		UWorld* World,
		const FNSRewardDropResult& DropResult,
		const FNSDropLaunchData& LaunchData,
		float CurrencyDropDuration
	);
	
	static void HandlePartDropResult(
		UWorld* World,
		const FNSRewardDropResult& DropResult,
		const FVector& DropLocation,
		FRandomStream& RandomStream,
		TSubclassOf<ANSDroppedPart> DroppedPartClass
	);
	
	static void HandleAugmentDropResult(const FNSRewardDropResult& DropResult);
	
	static FNSPartData MakePartDataFromDropResult(
		UWorld* World, const FNSRewardDropResult& DropResult, FRandomStream& RandomStream);
};
