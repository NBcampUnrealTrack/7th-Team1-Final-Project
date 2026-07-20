// Copyright 2026 One Team. All rights reserved.

#include "NSCheatManager.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "NeoSanctum/Character/Component/NSCompanionProgressionComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSProgressionSubsystem.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/Config/NSCommonDataConfig.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NeoSanctum/Progression/Part/NSPartEquipComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Currency.h"
#include "Engine/DataTable.h"
#include "NeoSanctum/Data/Progression/Currency/NSCurrencyTypes.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/Interaction/NPC/NSRescueNPC.h"
#include "NeoSanctum/Progression/Part/NSDroppedPart.h"
#include "NeoSanctum/Progression/Reward/NSRewardHandler.h"
#include "NeoSanctum/Tag/NSGameplayTags_Reward.h"
#include "NeoSanctum/Progression/Save/NSPermanentSaveGame.h"
#include "NeoSanctum/System/NSSaveGameSubsystem.h"

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

void UNSCheatManager::Debug_ResetCompanionUpgrades()
{
	ANSPlayerController* OwningPC =
		Cast<ANSPlayerController>(
			GetOuterAPlayerController());

	if (!OwningPC || !OwningPC->IsLocalController())
	{
		return;
	}

	UGameInstance* GameInstance =
		OwningPC->GetGameInstance();

	UNSSaveGameSubsystem* SaveSubsystem =
		GameInstance
			? GameInstance->GetSubsystem<UNSSaveGameSubsystem>()
			: nullptr;

	UNSPermanentSaveGame* PermanentSave =
		SaveSubsystem
			? SaveSubsystem->GetCachedPermanentData()
			: nullptr;

	if (!SaveSubsystem || !PermanentSave)
	{
		return;
	}

	// 노드별 저장 레벨 초기화 (드론별 + 공유)
	PermanentSave->Companion.DroneNodes.Reset();
	PermanentSave->Companion.SharedNodeLevels.Reset();

	// 변경된 캐시 데이터를 영구 저장
	SaveSubsystem->SavePermanent(
		PermanentSave,
		FNSSaveComplete());

	// 리슨 서버 진행도에도 초기화된 노드 레벨 전달
	OwningPC->UploadLocalProgress(
		OwningPC->GetActiveCharacterIdForUpload());
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

// 테스트용 임시 코드 (인런 구출 NPC 구현 후 삭제)
void UNSCheatManager::Debug_UnlockNPC(FString NpcId)
{
	ANSPlayerController* OwningPC = Cast<ANSPlayerController>(GetOuterAPlayerController());
	if (!OwningPC || NpcId.IsEmpty())
	{
		return;
	}

	UGameInstance* GameInstance = OwningPC->GetGameInstance();
	UNSProgressionSubsystem* Progression =
		GameInstance ? GameInstance->GetSubsystem<UNSProgressionSubsystem>() : nullptr;
	if (!Progression)
	{
		return;
	}

	// 로컬 CachedData에 해금 기록 후, 기존 업로드 경로로 서버 동기화(+소유 클라 복제/브로드캐스트)
	Progression->UnlockNPC(FName(*NpcId));
	OwningPC->UploadLocalProgress(OwningPC->GetActiveCharacterIdForUpload());
}

// 테스트용 임시 코드 (인런 구출 NPC 구현 후 삭제)
void UNSCheatManager::Debug_LockNPC(FString NpcId)
{
	ANSPlayerController* OwningPC = Cast<ANSPlayerController>(GetOuterAPlayerController());
	if (!OwningPC || NpcId.IsEmpty())
	{
		return;
	}

	UGameInstance* GameInstance = OwningPC->GetGameInstance();
	UNSProgressionSubsystem* Progression =
		GameInstance ? GameInstance->GetSubsystem<UNSProgressionSubsystem>() : nullptr;
	if (!Progression)
	{
		return;
	}

	// 로컬 CachedData에서 해금 제거 후, 기존 업로드 경로로 서버 동기화(+소유 클라 복제/브로드캐스트)
	Progression->LockNPC(FName(*NpcId));
	OwningPC->UploadLocalProgress(OwningPC->GetActiveCharacterIdForUpload());
}

// 테스트용 치트 (슬롯 언락/파츠 구매 테스트 — 캐시된 영구 공통재화를 즉시 10000으로 설정, 디스크 저장/서버 동기화 없음)
void UNSCheatManager::Debug_SetCommonCurrency()
{
	ANSPlayerController* OwningPC = Cast<ANSPlayerController>(GetOuterAPlayerController());
	if (!OwningPC)
	{
		return;
	}

	UGameInstance* GameInstance = OwningPC->GetGameInstance();
	UNSSaveGameSubsystem* SaveSubsystem =
		GameInstance ? GameInstance->GetSubsystem<UNSSaveGameSubsystem>() : nullptr;
	UNSPermanentSaveGame* PermanentSave =
		SaveSubsystem ? SaveSubsystem->GetCachedPermanentData() : nullptr;
	if (!PermanentSave)
	{
		return;
	}

	// NSProgressionSubsystem::GetSaveData()가 읽는 캐시 객체와 동일. 저장/서버 동기화 없이 즉시 반영됨
	PermanentSave->CommonCurrency = 10000;
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

void UNSCheatManager::Debug_UpgradeCommonNode(FString NodeId)
{
	ANSPlayerController* OwningPC = Cast<ANSPlayerController>(GetOuterAPlayerController());
	if (!OwningPC || NodeId.IsEmpty())
	{
		return;
	}

	UGameInstance* GameInstance = OwningPC->GetGameInstance();
	UNSProgressionSubsystem* ProgressionSubsystem =
		GameInstance ? GameInstance->GetSubsystem<UNSProgressionSubsystem>() : nullptr;
	if (!ProgressionSubsystem)
	{
		return;
	}

	const FName Node(*NodeId);
	const int32 NewLevel = ProgressionSubsystem->GetCommonSkillLevel(Node) + 1;
	const int64 Cost = ProgressionSubsystem->GetCommonUpgradeCost(Node, NewLevel);
	const bool bSuccess = ProgressionSubsystem->UpgradeCommonSkill(Node, NewLevel, Cost);

	NS_LOG(LogNS, Warning,
		"NodeId={NodeId}, NewLevel={NewLevel}, Cost={Cost}, Success={Success}",
		("NodeId", NodeId),
		("NewLevel", NewLevel),
		("Cost", Cost),
		("Success", bSuccess)
	);

	// 로컬 세이브 변경분을 서버 ProgressComponent에 반영
	OwningPC->UploadLocalProgress(OwningPC->GetActiveCharacterIdForUpload());
}

void UNSCheatManager::Debug_HurtSelf(float DamageAmount)
{
	APlayerController* OwningPC = GetOuterAPlayerController();
	APawn* Pawn = OwningPC ? OwningPC->GetPawn() : nullptr;

	// Player ASC는 캐릭터가 아니라 PlayerState에 있으므로 인터페이스로 조회.
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Pawn);
	UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	const float CurrentHealth = ASC->GetNumericAttribute(UNSBaseAttributeSet::GetHealthAttribute());
	const float NewHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
	ASC->SetNumericAttributeBase(UNSBaseAttributeSet::GetHealthAttribute(), NewHealth);

	NS_LOG(LogNS, Warning, "[Debug] HurtSelf: {Amount} 데미지 적용, 현재 체력={NewHealth}",
		("Amount", DamageAmount),
		("NewHealth", NewHealth)
	);
}

void UNSCheatManager::Debug_ResetCommonUpgrades()
{
	ANSPlayerController* OwningPC = Cast<ANSPlayerController>(GetOuterAPlayerController());

	if (!OwningPC || !OwningPC->IsLocalController())
	{
		return;
	}

	UGameInstance* GameInstance = OwningPC->GetGameInstance();

	UNSSaveGameSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UNSSaveGameSubsystem>() : nullptr;

	UNSPermanentSaveGame* PermanentSave = SaveSubsystem ? SaveSubsystem->GetCachedPermanentData() : nullptr;

	if (!SaveSubsystem || !PermanentSave)
	{
		return;
	}

	// 노드별 저장 레벨 초기화 (재화는 건드리지 않음)
	PermanentSave->CommonSkillLevels.Reset();

	// 변경된 캐시 데이터를 영구 저장
	SaveSubsystem->SavePermanent(PermanentSave, FNSSaveComplete());

	// 리슨 서버 진행도에도 초기화된 노드 레벨 전달
	OwningPC->UploadLocalProgress(OwningPC->GetActiveCharacterIdForUpload());
}

// 테스트용 치트 (인런 파츠 상점 테스트 — 임시재화 10000 즉시 지급, 드랍/줍기 없음)
void UNSCheatManager::Debug_AddTempCurrency()
{
	ANSPlayerController* OwningPC = Cast<ANSPlayerController>(GetOuterAPlayerController());
	if (!OwningPC)
	{
		return;
	}

	// 서버 권한 RPC 경유: 원격 클라가 입력해도 서버의 CurrencyComponent에 반영되도록 함
	OwningPC->Server_DebugAddTempCurrency();
}

// 테스트용 치트 (보스전 강제 진입 — 텔레포트/보스 스포너 활성화까지 즉시 처리)
void UNSCheatManager::Debug_ForceBossFight()
{
	ANSPlayerController* OwningPC = Cast<ANSPlayerController>(GetOuterAPlayerController());
	if (!OwningPC)
	{
		return;
	}

	// 서버 권한 RPC 경유: 원격 클라가 입력해도 서버에서 GameMode/GameState를 직접 조작하도록 함
	OwningPC->Server_DebugForceBossFight();
}

// 테스트용 임시 코드 (인런 구출 NPC 재화 보상 테스트 — 실제 트리거 연결 후 삭제)
void UNSCheatManager::Debug_TriggerRescue(FString NpcId)
{
	ANSPlayerController* OwningPC = Cast<ANSPlayerController>(GetOuterAPlayerController());
	UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
	if (!World || NpcId.IsEmpty())
	{
		return;
	}

	const FName TargetNpcId(*NpcId);
	for (TActorIterator<ANSRescueNPC> It(World); It; ++It)
	{
		if (It->GetNPCId() == TargetNpcId)
		{
			INSInteractable::Execute_OnInteract(*It, OwningPC);
			return;
		}
	}
}

// 테스트용 치트 (파츠 VisualTag 테스트 — DT_PartDefinition의 Row Name을 그대로 입력, 예: Arm1)
// 실제 드랍/줍기와 같은 경로(Server_RequestEquip)를 타서 OnPartChanged가 정상 브로드캐스트되게 함
void UNSCheatManager::Debug_EquipPart(FString RowName)
{
	ANSPlayerController* OwningPC = Cast<ANSPlayerController>(GetOuterAPlayerController());
	if (!OwningPC)
	{
		return;
	}

	ANSPlayerState* PS = OwningPC->GetPlayerState<ANSPlayerState>();
	UNSPartEquipComponent* EquipComp = PS ? PS->GetPartEquipComponent() : nullptr;
	if (!EquipComp)
	{
		return;
	}

	// 빈 문자열이면 장착 파츠 전체 해제 (슬롯 하나만 지우는 RPC는 따로 없음, 호스트에서만 동작)
	if (RowName.IsEmpty())
	{
		if (OwningPC->HasAuthority())
		{
			EquipComp->ClearAll();
		}
		return;
	}

	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(OwningPC);
	const UNSCommonDataConfig* CommonConfig = DataSS ? DataSS->GetCommonDataConfig() : nullptr;
	UDataTable* PartTable = CommonConfig ? CommonConfig->PartsBaseStatTable.Get() : nullptr;
	const FNSPartDefinitionRow* Row = PartTable
		? PartTable->FindRow<FNSPartDefinitionRow>(FName(*RowName), TEXT("Debug_EquipPart"), false)
		: nullptr;
	if (!Row)
	{
		NS_LOG(LogNS, Warning, "[Debug_EquipPart] Row를 찾을 수 없습니다. RowName={RowName}", ("RowName", RowName));
		return;
	}

	FNSPartData NewPart;
	NewPart.DefinitionPtr = Row->Definition;
	NewPart.CurrentRarity = ENSPartRarity::Common;
	EquipComp->Server_RequestEquip(NewPart);
}
