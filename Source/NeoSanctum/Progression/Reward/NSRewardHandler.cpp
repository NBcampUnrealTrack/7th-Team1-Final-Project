// Copyright 2026 One Team. All rights reserved.


#include "NSRewardHandler.h"
#include "Engine/AssetManager.h"

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
#include "NeoSanctum/Tag/NSGameplayTags_Reward.h"

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
	
	const FVector CandidateTargetLocation = Origin + HorizontalOffset;
	
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
	
	NS_LOG(LogNS, Log,
		"Currency 드랍을 등록했습니다. DropId={DropId}, Currency={Currency}, Quantity={Quantity}",
		("DropId", DropId),
		("Currency", DropResult.CurrencyTag.ToString()),
		("Quantity", DropResult.Quantity)
	);
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
	
	if (DropResult.PartDefinition.IsNull())
	{
		NS_LOG(LogNS, Warning, "Part RewardResult의 PartDefinition이 비어 있습니다.");
		return;
	}
	
	if (DropResult.Quantity <= 0)
	{
		NS_LOG(LogNS, Warning,
			"Part RewardResult의 Quantity가 0 이하입니다. PartDefinition={PartDefinition}, Quantity={Quantity}",
			("PartDefinition", DropResult.PartDefinition.ToSoftObjectPath().ToString()),
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
				"Part RewardResult에서 유효한 FNSPartData를 만들 수 없습니다. PartDefinition={PartDefinition}",
				("PartDefinition", DropResult.PartDefinition.ToSoftObjectPath().ToString())
			);
			continue;
		}
		
		// 동일한 보상 결과에서 생성되는 파츠가 겹치지 않도록 개별 발사 정보를 발생
		const FNSDropLaunchData LaunchData = MakeDropLaunchData(World, DropLocation, RandomStream);
		
		ANSDroppedPart* DroppedPart = ANSDroppedPart::SpawnInWorld(
			World,
			DroppedPartClass,
			PartData,
			LaunchData
		);
		
		if (!DroppedPart)
		{
			NS_LOG(LogNS, Warning,
				"Part 드랍 액터 생성에 실패했습니다. PartDefinition={PartDefinition}",
				("PartDefinition", DropResult.PartDefinition.ToSoftObjectPath().ToString())
			);
			continue;
		}
		
		NS_LOG(LogNS, Log,
			"Part 드랍 액터를 생성했습니다. PartDefinition={PartDefinition}, Rarity={Rarity}, Value={Value}",
			("PartDefinition", DropResult.PartDefinition.ToSoftObjectPath().ToString()),
			("Rarity", static_cast<int32>(PartData.CurrentRarity)),
			("Value", PartData.CurrentValue)
		);
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

	if (DropResult.PartDefinition.IsNull())
	{
		return PartData;
	}

	const FPrimaryAssetId DefId =
		UAssetManager::Get().GetPrimaryAssetIdForPath(DropResult.PartDefinition.ToSoftObjectPath());
	const FNSPartDefinitionRow* Row = NSPartUtils::ResolvePartRow(World, DefId);
	if (!Row)
	{
		return PartData;
	}

	PartData.DefinitionPtr = DropResult.PartDefinition;
	PartData.Slot = Row->PartSlot;
	PartData.CurrentRarity = ENSPartRarity::Common;
	PartData.RollCount = 0;

	const FNSPartUpgradeRow* UpgradeRow = NSPartUtils::ResolvePartUpgradeRow(World, PartData.CurrentRarity);
	if (UpgradeRow)
	{
		const float MinValue = FMath::Min(UpgradeRow->ValueRange.Min, UpgradeRow->ValueRange.Max);
		const float MaxValue = FMath::Max(UpgradeRow->ValueRange.Min, UpgradeRow->ValueRange.Max);
		PartData.CurrentValue = RandomStream.FRandRange(MinValue, MaxValue);
	}

	return PartData;
}
