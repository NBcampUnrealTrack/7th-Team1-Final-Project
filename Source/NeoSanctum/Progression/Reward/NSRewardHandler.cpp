// Copyright 2026 One Team. All rights reserved.


#include "NSRewardHandler.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "NeoSanctum/Data/Augment/NSAugmentRarityRuleSet.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Progression/Part/NSPartUtils.h"
#include "NeoSanctum/Data/Reward/NSRewardDataRegistry.h"
#include "NeoSanctum/Data/Reward/NSRewardDropResolver.h"
#include "NeoSanctum/Data/Reward/NSRewardTriggerData.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Progression/Augment/NSAugmentSelectionComponent.h"
#include "NeoSanctum/Progression/Experience/NSExperienceComponent.h"
#include "NeoSanctum/Progression/Part/NSDroppedPart.h"
#include "NeoSanctum/System/Subsystem/NSCurrencyDropSubsystem.h"
#include "NeoSanctum/System/Subsystem/NSDroppedPartRegistrySubsystem.h"
#include "NeoSanctum/Tag/NSGameplayTags_Reward.h"
#include "NeoSanctum/System/Subsystem/NSHealDropSubsystem.h"

void UNSRewardHandler::HandleRewardTrigger(
	UWorld* World,
	const UNSRewardDataRegistry* RewardDataRegistry,
	const FGameplayTag& TriggerTag,
	const FVector& DropLocation,
	FRandomStream& RandomStream,
	TSubclassOf<ANSDroppedPart> DroppedPartClass,
	float CurrencyDropDuration)
{
	if (!World)
	{
		NS_LOG(LogNS, Warning, "Reward 처리에 필요한 World가 유효하지 않습니다.");
		return;
	}
	
	if (World->GetNetMode() == NM_Client)
	{
		return;
	}
	
	if (!RewardDataRegistry)
	{
		NS_LOG(LogNS, Warning,
			"RewardDataRegistry가 유효하지 않습니다. TriggerTag={TriggerTag}",
			("TriggerTag", TriggerTag.ToString())
		);
		return;
	}
	
	if (!TriggerTag.IsValid())
	{
		NS_LOG(LogNS, Warning, "Reward TriggerTag가 유효하지 않습니다.");
		return;
	}
	
	const UNSRewardTriggerData* RewardTriggerData = RewardDataRegistry->FindRewardTriggerDataByTag(TriggerTag);
	
	if (!RewardTriggerData)
	{
		NS_LOG(LogNS, Warning,
			"RewardTriggerData를 찾을 수 없습니다. TriggerTag={TriggerTag}",
			("TriggerTag", TriggerTag.ToString())
		);
		return;
	}
	
	HandleRewardEntries(World, *RewardTriggerData, TriggerTag);
	
	if (RewardTriggerData->DropTable.IsNull())
	{
		return;
	}
	
	const UDataTable* DropTable = RewardTriggerData->DropTable.Get();
	
	if (!DropTable)
	{
		NS_LOG(LogNS, Warning,
			"RewardTriggerData의 DropTable이 로드되어 있지 않습니다. TriggerTag={TriggerTag}, Asset={Asset}",
			("TriggerTag", TriggerTag.ToString()),
			("Asset", GetNameSafe(RewardTriggerData))
		);
		return;
	}
	
	TArray<FNSRewardDropResult> DropResults;
	UNSRewardDropResolver::ResolveDropResultsFromTable(DropTable, RandomStream, DropResults);
	
	HandleDropResults(
		World,
		DropResults,
		DropLocation,
		RandomStream,
		DroppedPartClass,
		CurrencyDropDuration
	);
}

void UNSRewardHandler::HandleExperienceRewardEntry(UWorld* World, float BaseExpAmount)
{
	if (!World || BaseExpAmount <= 0.0f)
	{
		return;
	}

	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		ANSPlayerState* NSPlayerState = PlayerController ? PlayerController->GetPlayerState<ANSPlayerState>() : nullptr;

		UNSExperienceComponent* ExperienceComponent = NSPlayerState ? NSPlayerState->GetExperienceComponent() : nullptr;

		if (!ExperienceComponent)
		{
			continue;
		}

		const int32 LevelUpCount = ExperienceComponent->AddExperience(BaseExpAmount);

		UNSAugmentSelectionComponent* AugmentSelectionComponent =
			PlayerController->FindComponentByClass<UNSAugmentSelectionComponent>();

		if (!AugmentSelectionComponent)
		{
			if (LevelUpCount > 0)
			{
				NS_LOG(LogNS, Warning,
					"AugmentSelectionComponent를 찾을 수 없어 레벨업 보상을 지급하지 못했습니다. Player={Player}, LevelUpCount={LevelUpCount}",
					("Player", GetNameSafe(NSPlayerState)),
					("LevelUpCount", LevelUpCount)
				);
			}
			continue;
		}

		if (LevelUpCount > 0)
		{
			NS_LOG(LogNS, Log,
				"레벨업 보상을 적재합니다. Player={Player}, BaseExpAmount={BaseExpAmount}, LevelUpCount={LevelUpCount}",
				("Player", GetNameSafe(NSPlayerState)),
				("BaseExpAmount", BaseExpAmount),
				("LevelUpCount", LevelUpCount)
			);
		}

		// 배율 차이로 플레이어마다 레벨업 시점이 달라지므로 개인별로 적재
		for (int32 Index = 0; Index < LevelUpCount; ++Index)
		{
			AugmentSelectionComponent->EnqueueOffer(NSGameplayTags::Reward_Trigger_LevelUp);
		}
	}
}

void UNSRewardHandler::HandleRewardEntries(
	UWorld* World,
	const UNSRewardTriggerData& RewardTriggerData,
	const FGameplayTag& TriggerTag)
{
	for (const FNSRewardEntry& RewardEntry : RewardTriggerData.RewardEntries)
	{
		if (!RewardEntry.RewardTypeTag.IsValid())
		{
			continue;
		}
		
		if (RewardEntry.RewardTypeTag == NSGameplayTags::Reward_Type_Augment)
		{
			HandleAugmentRewardEntry(
				World,
				TriggerTag
			);
			continue;
		}
		
		NS_LOG(LogNS, Warning,
			"아직 RewardEntry 처리가 연결되지 않은 RewardType입니다. TriggerTag={TriggerTag}, RewardType={RewardType}",
			("TriggerTag", TriggerTag.ToString()),
			("RewardType", RewardEntry.RewardTypeTag.ToString())
		);
	}
}


void UNSRewardHandler::HandleAugmentRewardEntry(
	UWorld* World,
	const FGameplayTag& TriggerTag)
{
	if (!World)
	{
		return;
	}
	
	if (!TriggerTag.IsValid())
	{
		NS_LOG(LogNS, Warning, "증강 보상 트리거 태그가 유효하지 않습니다.");
		return;
	}
	
	int32 EnqueuedCount = 0;
	
	// 협동 보상 기준으로 모든 플레이어에게 동일한 증강 선택권을 지급
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (!PlayerController)
		{
			continue;
		}
		
		UNSAugmentSelectionComponent* AugmentSelectionComponent =
			PlayerController->FindComponentByClass<UNSAugmentSelectionComponent>();
		
		if (!AugmentSelectionComponent)
		{
			continue;
		}
		
		AugmentSelectionComponent->EnqueueOffer(TriggerTag);
		++EnqueuedCount;
	}
	
	NS_LOG(LogNS, Log,
		"Augment 보상 선택권을 적재했습니다. TriggerTag={TriggerTag}, PlayerCount={PlayerCount}",
		("TriggerTag", TriggerTag.ToString()),
		("PlayerCount", EnqueuedCount)
	);
}

void UNSRewardHandler::HandleDropResults(
	UWorld* World,
	const TArray<FNSRewardDropResult>& DropResults,
	const FVector& DropLocation,
	FRandomStream& RandomStream,
	TSubclassOf<ANSDroppedPart> DroppedPartClass,
	float CurrencyDropDuration)
{
	if (DropResults.IsEmpty())
	{
		NS_LOG(LogNS, Log, "RewardDropResult가 비어 있습니다. 이번 판정에서는 드랍이 없습니다.");
		return;
	}
	
	for (const FNSRewardDropResult& DropResult : DropResults)
	{
		if (DropResult.RewardTypeTag == NSGameplayTags::Reward_Type_Currency)
		{
			// 서버가 발사 정보를 한 번 결정해 모든 클라이언트가 같은 재화 궤적을 재생
			const FNSDropLaunchData LaunchData = MakeDropLaunchData(
				World,
				DropLocation,
				RandomStream
			);
			
			HandleCurrencyDropResult(
				World,
				DropResult,
				LaunchData,
				CurrencyDropDuration
			);
			continue;
		}

		if (DropResult.RewardTypeTag == NSGameplayTags::Reward_Type_Heal)
        {
			// 재화와 동일하게
            const FNSDropLaunchData LaunchData = MakeDropLaunchData(
                    World,
                    DropLocation,
                    RandomStream
            );
            HandleHealDropResult(
                    World,
                    DropResult,
                    LaunchData,
                    CurrencyDropDuration
            );
            continue;
        }
		
		if (DropResult.RewardTypeTag == NSGameplayTags::Reward_Type_Part)
		{
			HandlePartDropResult(
				World,
				DropResult,
				DropLocation,
				RandomStream,
				DroppedPartClass
			);
			continue;
		}
		
		if (DropResult.RewardTypeTag == NSGameplayTags::Reward_Type_Augment)
		{
			HandleAugmentDropResult(DropResult);
			continue;
		}
		
		NS_LOG(LogNS, Warning,
			"처리되지 않은 RewardDropResult입니다. RewardType={RewardType}",
			("RewardType", DropResult.RewardTypeTag.ToString())
		);
	}
}

bool UNSRewardHandler::TryFindDropGroundLocation(
	UWorld* World,
	const FVector& CandidateTargetLocation,
	FVector& OutGroundLocation)
{
	if (!World)
	{
		return false;
	}

	constexpr float GroundTraceStartOffset = 300.0f;
	constexpr float GroundTraceDepth = 1000.0f;
	constexpr float MinGroundNormalZ = 0.7f;
	constexpr float LandingProbeRadius = 45.0f;

	const FVector TraceStart = CandidateTargetLocation
		+ FVector::UpVector * GroundTraceStartOffset;

	const FVector TraceEnd = CandidateTargetLocation
		- FVector::UpVector * GroundTraceDepth;

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(RewardDropGroundTrace),
		false
	);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

	FHitResult GroundHit;

	const bool bFoundGround = World->SweepSingleByObjectType(
		GroundHit,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(LandingProbeRadius),
		QueryParams
	);

	if (!bFoundGround || GroundHit.ImpactNormal.Z < MinGroundNormalZ)
	{
		return false;
	}

	OutGroundLocation = GroundHit.Location;
	return true;
}

FNSDropLaunchData UNSRewardHandler::MakeDropLaunchData(UWorld* World,
	const FVector& Origin,
	FRandomStream& RandomStream)
{
	FNSDropLaunchData LaunchData;
	
	if (!World)
	{
		return LaunchData;
	}
	
	constexpr float StartHeightOffset = 30.0f;
	constexpr float MinLaunchDistance = 140.0f;
	constexpr float MaxLaunchDistance = 220.0f;
	constexpr float ArcHeight = 140.0f;
	constexpr float MinFlightDuration = 0.35f;
	constexpr float MaxFlightDuration = 0.45f;
	
	const float LaunchAngle = RandomStream.FRandRange(0.0f, UE_TWO_PI);
	const float LaunchDistance = RandomStream.FRandRange(MinLaunchDistance, MaxLaunchDistance);
	
	const FVector HorizontalOffset(
		FMath::Cos(LaunchAngle) * LaunchDistance,
		FMath::Sin(LaunchAngle) * LaunchDistance,
		0.0f
	);
	
	FVector CandidateTargetLocation = Origin + HorizontalOffset;

	/**
	 * 원점과 후보 착지점 사이에 벽이 있으면 착지점이 벽 안/벽 뒤로 잡히므로
	 * 수평 트레이스로 막힘을 검사하고 막혔으면 벽 앞으로 당긴다
	 */
	constexpr float WallProbeHeight = 50.0f;
	constexpr float WallBackoffDistance = 50.f;
	
	FCollisionQueryParams WallQueryParams(SCENE_QUERY_STAT(RewardDropWallTrace), false);
	FCollisionObjectQueryParams WallObjectParams;
	WallObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	
	const FVector WallTraceStart = Origin + FVector::UpVector * WallProbeHeight;
	const FVector WallTraceEnd = CandidateTargetLocation + FVector::UpVector * WallProbeHeight;
	
	FHitResult WallHit;
	if (World->LineTraceSingleByObjectType(WallHit, WallTraceStart, WallTraceEnd, WallObjectParams, WallQueryParams))
	{
		// 히트 지점에서 원점 방향으로 여유 거리만큼 당긴 위치를 새 후보로 사용
		const FVector BackDirection = (WallTraceStart - WallTraceEnd).GetSafeNormal();
		const FVector PulledLocation = WallHit.Location + BackDirection * WallBackoffDistance;
		CandidateTargetLocation.X = PulledLocation.X;
		CandidateTargetLocation.Y = PulledLocation.Y;
	}

	LaunchData.StartLocation = Origin + FVector(0.0f, 0.0f, StartHeightOffset);
	
	FVector GroundTargetLocation;
	
	if (TryFindDropGroundLocation(World, CandidateTargetLocation, GroundTargetLocation))
	{
		LaunchData.TargetLocation = GroundTargetLocation;
	}
	else if (TryFindDropGroundLocation(World, Origin, GroundTargetLocation))
	{
		LaunchData.TargetLocation = GroundTargetLocation;
	}
	else
	{
		// 후보 위치에서 유효한 지면을 찾지 못하면 기존 드랍 위치로 fallback
		LaunchData.TargetLocation = Origin;
	}
	
	const AGameStateBase* GameState = World->GetGameState();
	
	LaunchData.StartServerTime = GameState ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
	LaunchData.FlightDuration = RandomStream.FRandRange(MinFlightDuration, MaxFlightDuration);
	LaunchData.ArcHeight = ArcHeight;
	
	return LaunchData;
}

void UNSRewardHandler::HandleCurrencyDropResult(
	UWorld* World,
	const FNSRewardDropResult& DropResult,
	const FNSDropLaunchData& LaunchData,
	float CurrencyDropDuration)
{
	if (!World)
	{
		return;
	}
	
	if (!DropResult.CurrencyTag.IsValid())
	{
		NS_LOG(LogNS, Warning,
			"Currency RewardResult의 CurrencyTag가 유효하지 않습니다. RewardType={RewardType}",
			("RewardType", DropResult.RewardTypeTag.ToString())
		);
		return;
	}
	
	if (DropResult.Quantity <= 0)
	{
		NS_LOG(LogNS, Warning,
			"Currency RewardResult의 Quantity가 0 이하입니다. Currency={Currency}, Quantity={Quantity}",
			("Currency", DropResult.CurrencyTag.ToString()),
			("Quantity", DropResult.Quantity)
		);
		return;
	}
	
	UNSCurrencyDropSubsystem* CurrencyDropSubsystem = World->GetSubsystem<UNSCurrencyDropSubsystem>();
	
	if (!CurrencyDropSubsystem)
	{
		NS_LOG(LogNS, Warning,
			"CurrencyDropSubsystem을 찾을 수 없습니다. Currency={Currency}",
			("Currency", DropResult.CurrencyTag.ToString())
		);
		return;
	}
	
	// TODO: @원종 임시로 CurrencyGrade를 None으로 테스트
	const int32 DropId = CurrencyDropSubsystem->RegisterDrop(
		DropResult.CurrencyTag,
		ENSCurrencyGrade::None,
		static_cast<int64>(DropResult.Quantity),
		LaunchData.TargetLocation,
		CurrencyDropDuration,
		LaunchData
	);
	
	if (DropId == INDEX_NONE)
	{
		NS_LOG(LogNS, Warning,
			"Currency 드랍 등록에 실패했습니다. Currency={Currency}, Quantity={Quantity}",
			("Currency", DropResult.CurrencyTag.ToString()),
			("Quantity", DropResult.Quantity)
		);
		return;
	}
}

void UNSRewardHandler::HandleHealDropResult(UWorld* World, const FNSRewardDropResult& DropResult,
	const FNSDropLaunchData& LaunchData, float Duration)
{
	if (!World)
	{
		return;
	}

	/**
	 * Heal은 CurrencyTag/PartDefinition을 쓰지 않고, 전용 HealPotionTag로 어떤 포션인지만 식별한다.
	 * 실제 회복%는 이 태그로 DT_HealPotion을 조회해서 얻으므로(서버 TryCollect 시점), 여기선 태그 유효성만 확인.
	 */
	if (!DropResult.HealPotionTag.IsValid())
	{
		NS_LOG(LogNS, Warning,
			"Heal RewardResult의 HealPotionTag가 유효하지 않습니다. RewardType={RewardType}",
			("RewardType", DropResult.RewardTypeTag.ToString()));
		return;
	}

	UNSHealDropSubsystem* HealDropSubsystem = World->GetSubsystem<UNSHealDropSubsystem>();
	if (!HealDropSubsystem)
	{
		NS_LOG(LogNS, Warning, "HealDropSubsystem을 찾을 수 없습니다.");
		return;
	}

	const int32 DropId = HealDropSubsystem->RegisterDrop(
		  DropResult.HealPotionTag,
		  LaunchData.TargetLocation,
		  Duration,
		  LaunchData
	);
	if (DropId == INDEX_NONE)
	{
		NS_LOG(LogNS, Warning,
				"Heal 드랍 등록에 실패했습니다. PotionTag={PotionTag}",
				("PotionTag", DropResult.HealPotionTag.ToString())
		);
		return;
	}
}

void UNSRewardHandler::HandlePartDropResult(
	UWorld* World,
	const FNSRewardDropResult& DropResult,
	const FVector& DropLocation,
	FRandomStream& RandomStream,
	TSubclassOf<ANSDroppedPart> DroppedPartClass)
{
	if (!World)
	{
		return;
	}
	
	if (DropResult.Quantity <= 0)
	{
		NS_LOG(LogNS, Warning,
			"Part RewardResult의 Quantity가 0 이하입니다. Quantity={Quantity}",
			("Quantity", DropResult.Quantity)
		);
		return;
	}
	
	for (int32 SpawnIndex = 0; SpawnIndex < DropResult.Quantity; ++SpawnIndex)
	{
		FNSPartData PartData = MakePartDataFromDropResult(World, DropResult, RandomStream);
		
		if (!PartData.IsValid())
		{
			NS_LOG(LogNS, Warning,
				"Part RewardResult에서 유효한 FNSPartData를 만들 수 없습니다.");
			continue;
		}
		
		// 동일한 보상 결과에서 생성되는 파츠가 겹치지 않도록 개별 발사 정보를 발생
		FNSDropLaunchData LaunchData = MakeDropLaunchData(World, DropLocation, RandomStream);

		if (UNSDroppedPartRegistrySubsystem* Registry = World->GetSubsystem<UNSDroppedPartRegistrySubsystem>())
		{
			constexpr int32 MaxAttempts = 6;
			for (int32 Attempt = 0;
				Attempt < MaxAttempts && Registry->IsLocationOccupied(LaunchData.TargetLocation);
				++Attempt)
			{
				LaunchData = MakeDropLaunchData(World, DropLocation, RandomStream);
			}
		}

		ANSDroppedPart* DroppedPart = ANSDroppedPart::SpawnInWorld(
			World,
			DroppedPartClass,
			PartData,
			LaunchData
		);
		
		if (!DroppedPart)
		{
			NS_LOG(LogNS, Warning, "Part 드랍 액터 생성에 실패했습니다.");
			continue;
		}
	}
}

void UNSRewardHandler::HandleAugmentDropResult(const FNSRewardDropResult& DropResult)
{
	NS_LOG(LogNS, Log,
		"Augment DropResult가 감지되었습니다. AugmentPoolTag={AugmentPoolTag}",
		("AugmentPoolTag", DropResult.AugmentPoolTag.ToString())
	);
}

FNSPartData UNSRewardHandler::MakePartDataFromDropResult(
	UWorld* World, const FNSRewardDropResult& DropResult, FRandomStream& RandomStream)
{
	FNSPartData PartData;

	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(World);
	if (!DataSS)
	{
		return PartData;
	}
	
	ENSPartRarity Rarity;
	if (!NSPartUtils::ResolveRarityFromTag(DropResult.RarityTag, Rarity))
	{
		NS_LOG(LogNS, Warning,
			"Part 드랍 결과의 RarityTag가 유효하지 않습니다. RarityTag={RarityTag}",
			("RarityTag", DropResult.RarityTag.ToString()));
		return PartData;
	}

	/**
	 * 이 등급에서 유효한 스탯(ValueRangesByRarity에 등급 키 존재)이 하나도 없는 파츠는 후보에서 제외.
	 * 파츠 선택 후 스탯만 필터링하면 스탯 없는 깡통 파츠가 드랍되므로, 파츠 선택 단계에서 미리 거른다
	 */
	TArray<const FNSPartDefinitionRow*> Candidates;
	for (const TPair<FPrimaryAssetId, FNSPartDefinitionRow>& PartPair : DataSS->GetAllPartRows())
	{
		if (NSPartUtils::FilterStatTagsByRarity(World, PartPair.Value.StatTags, Rarity).Num() > 0)
		{
			Candidates.Add(&PartPair.Value);
		}
	}

	if (Candidates.Num() == 0)
	{
		NS_LOG(LogNS, Warning, "Part드랍 대상 풀이 비어있습니다. Rarity={Rarity}",
			("Rarity", static_cast<int32>(Rarity)));
		return PartData;
	}

	const FNSPartDefinitionRow* Pick = Candidates[RandomStream.RandRange(0, Candidates.Num() - 1)];

	PartData.DefinitionPtr = Pick->Definition;
	PartData.Slot = Pick->PartSlot;
	PartData.CurrentRarity = Rarity;
	PartData.RollCount = 0;

	// 이 드롭 인스턴스의 스탯을 후보에서 확정 (파츠 후보 필터를 통과했으므로 반드시 1개 이상)
	const TArray<FGameplayTag> EligibleStatTags = NSPartUtils::FilterStatTagsByRarity(World, Pick->StatTags, PartData.CurrentRarity);
	PartData.StatTag = EligibleStatTags[RandomStream.RandRange(0, EligibleStatTags.Num() - 1)];

	// 스탯 × 등급별 수치 범위에서 직접 롤. 드롭 재현성 유지를 위해 RandomStream 사용
	FNSPartValueRange Range;
	if (NSPartUtils::GetStatValueRange(World, PartData.StatTag, PartData.CurrentRarity, Range))
	{
		PartData.CurrentValue = RandomStream.FRandRange(Range.Min, Range.Max);
	}
	
	return PartData;
}
