// Copyright 2026 One Team. All rights reserved.

#include "NSCheatManager.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Tag/NSGameplayTags_Currency.h"
#include "NeoSanctum/Data/Progression/Currency/NSCurrencyTypes.h"

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
