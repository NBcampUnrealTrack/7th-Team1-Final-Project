// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/CheatManager.h"
#include "NSCheatManager.generated.h"

/** 개발/테스트 전용 치트 매니저. Shipping 빌드에서는 생성되지 않음. */
UCLASS()
class NEOSANCTUM_API UNSCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	// 테스트용 임시 코드 (재화 드랍 테스트 — 드롭 테이블 연동 후 삭제)
	// 각 플레이어 앞에 임시재화(인런 전용) 드랍
	UFUNCTION(Exec)
	void Debug_SpawnTemp();

	// 테스트용 임시 코드 (재화 드랍 테스트 — 드롭 테이블 연동 후 삭제)
	// 각 플레이어 앞에 공통재화(영구) 드랍
	UFUNCTION(Exec)
	void Debug_SpawnCommon();

	// 테스트용 임시 코드 (재화 드랍 테스트 — 드롭 테이블 연동 후 삭제)
	// 각 플레이어 앞에 스킬재화(영구) 드랍
	UFUNCTION(Exec)
	void Debug_SpawnSkill();

	// 테스트용 임시 코드 (재화 드랍 테스트 — 드롭 테이블 연동 후 삭제)
	// 대기 중인 영구재화(공통/스킬)를 ProgressComponent에 커밋 (런 종료 시뮬레이션)
	UFUNCTION(Exec)
	void Debug_CommitPermanent();
	
	// 테스트용 임시 코드 (Reward Trigger 테스트 — 실제 보상 트리거 연결 후 삭제)
	UFUNCTION(Exec)
	void Debug_RewardNormal();

	// 테스트용 임시 코드 (Reward Trigger 테스트 — 실제 보상 트리거 연결 후 삭제)
	UFUNCTION(Exec)
	void Debug_RewardElite();

	// 테스트용 임시 코드 (Reward Trigger 테스트 — 실제 보상 트리거 연결 후 삭제)
	UFUNCTION(Exec)
	void Debug_RewardBoss();

	// 테스트용 임시 코드 (Reward Trigger 테스트 — 실제 보상 트리거 연결 후 삭제)
	UFUNCTION(Exec)
	void Debug_RewardLevelUp();

	// @민재 테스트용 임시 코드 (Companion Upgrade 테스트 — 실제 트리거 연결 후 삭제)
	UFUNCTION(Exec)
	void Debug_CompanionUpgrade(FString InTag);
	
	// 저장된 펫 강화 레벨과 누적 강화 횟수 초기화
	UFUNCTION(Exec)
	void Debug_ResetCompanionUpgrades();
	
	// @민재 테스트용 임시 코드 (Companion Upgrade 테스트 — 실제 트리거 연결 후 삭제)
	UFUNCTION(Exec)
	void Debug_CompanionSelect(FString InTag);

	// 테스트용 임시 코드 (인런 구출 NPC 구현 후 삭제)
	// 특정 NPC를 해금하고 서버에 동기화 (입장 게이트 테스트용). 인자 = NPCId
	UFUNCTION(Exec)
	void Debug_UnlockNPC(FString NpcId);

	// 테스트용 임시 코드 (인런 구출 NPC 구현 후 삭제)
	// 특정 NPC 해금을 취소(잠금)하고 서버에 동기화. 인자 = NPCId
	UFUNCTION(Exec)
	void Debug_LockNPC(FString NpcId);

	// 테스트용 치트 (슬롯 언락/파츠 구매 테스트 — 영구 공통재화를 10000으로 설정)
	UFUNCTION(Exec)
	void Debug_SetCommonCurrency();

	// TODO(원종): 테스트용 임시 코드 (공용 업그레이드 테스트 - UI 연결 후 삭제)
	UFUNCTION(Exec)
	void Debug_UpgradeCommonNode(FString NodeId);

	// TODO(원종): 테스트용 임시 코드 (공용 업그레이드 Current/Max 반영 테스트 - 검증 후 삭제)
	UFUNCTION(Exec)
	void Debug_HurtSelf(float DamageAmount);

	// 저장된 공용 업그레이드 레벨 초기화(재화는 그대로)
	UFUNCTION(Exec)
	void Debug_ResetCommonUpgrades();

	// 테스트용 치트 (인런 파츠 상점 테스트 — 임시재화 10000 즉시 지급, 드랍/줍기 없음)
	UFUNCTION(Exec)
	void Debug_AddTempCurrency();

	// 테스트용 치트 (보스전 강제 진입 — 텔레포트/보스 스포너 활성화까지 즉시 처리)
	UFUNCTION(Exec)
	void Debug_ForceBossFight();

	// 테스트용 임시 코드 (인런 구출 NPC 재화 보상 테스트 — 실제 트리거 연결 후 삭제)
	// 지정한 NPCId를 가진 ANSRescueNPC를 찾아 즉시 OnInteract 실행. 인자 = NPCId
	UFUNCTION(Exec)
	void Debug_TriggerRescue(FString NpcId);

	// 테스트용 치트 (파츠 VisualTag 테스트 — DT_PartDefinition의 Row Name을 그대로 입력, 예: Arm1)
	// 빈 문자열을 넣으면 장착 파츠 전체 해제(호스트에서만 동작)
	UFUNCTION(Exec)
	void Debug_EquipPart(FString RowName);

private:
	void HandleRewardTriggerCheat(const FGameplayTag& TriggerTag);
};
