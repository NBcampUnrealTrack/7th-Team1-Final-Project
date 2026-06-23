// Copyright 2026 One Team. All rights reserved.

#include "NSCheatManager.h"

#include "NeoSanctum/Character/Component/NSCompanionProgressionComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Tag/NSGameplayTags_Currency.h"
#include "NeoSanctum/Data/Progression/Currency/NSCurrencyTypes.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Progression/Part/NSDroppedPart.h"
#include "NeoSanctum/Progression/Reward/NSRewardHandler.h"
#include "NeoSanctum/Tag/NSGameplayTags_Reward.h"

class UNSRewardDataRegistry;
class UNSDataSubsystem;
// 테스트용 임시 코드 (재화 드랍 테스트 — 드롭 테이블 연동 후 삭제)
void UNSCheatManager::Debug_SpawnTemp()
{
	// 호스트/클라 누가 입력하든 서버 권한에서 드랍하도록 Server RPC 경유
	ANSPlayerController* OwningPC = Cast<ANSPlayerController>(GetOuterAPlayerController());
	if (!OwningPC)
	{
		return;
	}

	// 임시재화만 등급 구분
	OwningPC->Server_DebugSpawnCurrency(NSGameplayTags::Currency_Temp, ENSCurrencyGrade::Grade1);
}

// 테스트용 임시 코드 (재화 드랍 테스트 — 드롭 테이블 연동 후 삭제)
void UNSCheatManager::Debug_SpawnCommon()
{
	ANSPlayerController* OwningPC = Cast<ANSPlayerController>(GetOuterAPlayerController());
	if (!OwningPC)
	{
		return;
	}

	// 영구재화는 등급 None (VisualData 조회 규칙과 일치)
	OwningPC->Server_DebugSpawnCurrency(NSGameplayTags::Currency_Common, ENSCurrencyGrade::None);
}

// 테스트용 임시 코드 (재화 드랍 테스트 — 드롭 테이블 연동 후 삭제)
void UNSCheatManager::Debug_SpawnSkill()
{
	ANSPlayerController* OwningPC = Cast<ANSPlayerController>(GetOuterAPlayerController());
	if (!OwningPC)
	{
		return;
	}

	OwningPC->Server_DebugSpawnCurrency(NSGameplayTags::Currency_Skill, ENSCurrencyGrade::None);
}

// 테스트용 임시 코드 (재화 드랍 테스트 — 드롭 테이블 연동 후 삭제)
void UNSCheatManager::Debug_CommitPermanent()
{
	ANSPlayerController* OwningPC = Cast<ANSPlayerController>(GetOuterAPlayerController());
	if (!OwningPC)
	{
		return;
	}

	OwningPC->Server_DebugCommitPermanent();
}

void UNSCheatManager::Debug_RewardNormal()
{
	HandleRewardTriggerCheat(NSGameplayTags::Reward_Trigger_NormalKill);
}

void UNSCheatManager::Debug_RewardElite()
{
	HandleRewardTriggerCheat(NSGameplayTags::Reward_Trigger_EliteKill);
}

void UNSCheatManager::Debug_RewardBoss()
{
	HandleRewardTriggerCheat(NSGameplayTags::Reward_Trigger_BossKill);
}

void UNSCheatManager::Debug_RewardLevelUp()
{
	HandleRewardTriggerCheat(NSGameplayTags::Reward_Trigger_LevelUp);
}

void UNSCheatManager::Debug_CompanionUpgrade(FString InTag)
{
	ANSPlayerController* OwningPC = Cast<ANSPlayerController>(GetOuterAPlayerController());
	if (!OwningPC || InTag.IsEmpty())
	{
		return;
	}

	// InTag = 노드 태그
	const FGameplayTag NodeTag = FGameplayTag::RequestGameplayTag(FName(*InTag), false);
	if (!NodeTag.IsValid())
	{
		return;
	}

	OwningPC->CompanionCheatUpgrade(NodeTag);
}

void UNSCheatManager::Debug_CompanionSelect(FString InTag)
{
	ANSPlayerController* OwningPC = Cast<ANSPlayerController>(GetOuterAPlayerController());
	if (!OwningPC || InTag.IsEmpty())
	{
		return;
	}

	// InTag = 드론(컴패니언) 태그
	const FGameplayTag CompanionTag = FGameplayTag::RequestGameplayTag(FName(*InTag), false);
	if (!CompanionTag.IsValid())
	{
		return;
	}

	OwningPC->CompanionCheatSelect(CompanionTag);
}

void UNSCheatManager::HandleRewardTriggerCheat(const FGameplayTag& TriggerTag)
{
	APlayerController* OwningPlayerController = GetOuterAPlayerController();
	if (!OwningPlayerController)
	{
		return;
	}

	UWorld* World = OwningPlayerController->GetWorld();
	if (!World)
	{
		return;
	}

	if (World->GetNetMode() == NM_Client)
	{
		NS_LOG(LogNS, Warning,
			"Reward 치트는 PlayerController Server RPC 없이 서버/호스트에서만 실행할 수 있습니다. TriggerTag={TriggerTag}",
			("TriggerTag", TriggerTag.ToString())
		);
		return;
	}

	UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(OwningPlayerController);
	if (!DataSubsystem)
	{
		NS_LOG(LogNS, Warning,
			"Reward 치트 처리에 필요한 DataSubsystem을 찾을 수 없습니다. TriggerTag={TriggerTag}",
			("TriggerTag", TriggerTag.ToString())
		);
		return;
	}

	const UNSRewardDataRegistry* RewardDataRegistry = DataSubsystem->GetRewardDataRegistry();
	if (!RewardDataRegistry)
	{
		NS_LOG(LogNS, Warning,
			"Reward 치트 처리에 필요한 RewardDataRegistry가 유효하지 않습니다. TriggerTag={TriggerTag}",
			("TriggerTag", TriggerTag.ToString())
		);
		return;
	}

	const APawn* Pawn = OwningPlayerController->GetPawn();
	const FVector DropLocation = Pawn
		? Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 200.f
		: FVector::ZeroVector;

	FRandomStream RandomStream;
	RandomStream.Initialize(FMath::Rand());

	UNSRewardHandler::HandleRewardTrigger(
		World,
		RewardDataRegistry,
		TriggerTag,
		DropLocation,
		RandomStream,
		ANSDroppedPart::StaticClass(),
		60.0f
	);
}
