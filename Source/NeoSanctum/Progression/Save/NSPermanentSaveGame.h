// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "NeoSanctum/Core/PlayerState/NSProgressTypes.h"
#include "GameplayTagContainer.h"
#include "NSPermanentSaveGame.generated.h"

// 캐릭터의 영구 저장 데이터
USTRUCT(BlueprintType)
struct FNSCharacterSaveData
{
	GENERATED_BODY()

	// 직업별 재화
	UPROPERTY(SaveGame, BlueprintReadOnly)
	int64 JobCurrency = 0;   
	
	// 장착 파츠 1개 — 소유 인벤토리의 (정의,등급) 참조, Definition이 null이면 미장착
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TSoftObjectPtr<UNSPartDefinition> EquippedPartDefinition;
	UPROPERTY(SaveGame, BlueprintReadOnly)
	ENSPartRarity EquippedPartRarity = ENSPartRarity::Common;
	
	// 캐릭터별 스킬 (스킬종류, 레벨)
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TMap<FName,int32> CharacterSkillLevels;

	// 캐릭터별 언락된 슬롯
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TSet<FGameplayTag> UnlockedSlots;
};

// 한 드론이 독립적으로 보유하는 노드 레벨 묶음 (중첩 TMap 회피용 래퍼)
USTRUCT(BlueprintType)
struct FNSCompanionDroneNodeSaveData
{
	GENERATED_BODY()

	// 이 드론 전용 노드태그 → 레벨 (공유 노드는 여기 넣지 않음)
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TMap<FGameplayTag, int32> NodeLevels;
};

USTRUCT(BlueprintType)
struct FNSCompanionSaveData
{
	GENERATED_BODY()
	
	// 선택된 드론 태그
	UPROPERTY(SaveGame, BlueprintReadOnly)
	FGameplayTag SelectedCompanionTag;
	
	// 드론태그 → 그 드론의 독립 노드 레벨들
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TMap<FGameplayTag, FNSCompanionDroneNodeSaveData> DroneNodes;
	
	// 전 드론 공유 노드 (현재: Upgrade.Companion.Basic = 재화 탐지 범위) 노드태그 → 레벨
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TMap<FGameplayTag, int32> SharedNodeLevels;
	
	// 이미 해금(구매)된 드론들
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TSet<FGameplayTag> UnlockedCompanions;
	
	int32 GetNodeLevel(FGameplayTag DroneTag, FGameplayTag NodeTag, bool bShared) const
	{
		if (bShared)
		{
			return SharedNodeLevels.FindRef(NodeTag);
		}
		const FNSCompanionDroneNodeSaveData* Drone = DroneNodes.Find(DroneTag);
		return Drone ? Drone->NodeLevels.FindRef(NodeTag) : 0;
	}

	void SetNodeLevel(FGameplayTag DroneTag, FGameplayTag NodeTag, bool bShared, int32 Level)
	{
		if (bShared)
		{
			SharedNodeLevels.Add(NodeTag, Level);
			return;
		}
		DroneNodes.FindOrAdd(DroneTag).NodeLevels.Add(NodeTag, Level);
	}
};

UCLASS()
class NEOSANCTUM_API UNSPermanentSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// 계정 단위
	// 공용 재화
	UPROPERTY(SaveGame)
	int64 CommonCurrency = 0;
	// NPC 진행도
	UPROPERTY(SaveGame)
	TSet<FName> UnlockedNPCIds;
	// 공용 스킬 정보(스킬종류, 레벨)
	UPROPERTY(SaveGame)
	TMap<FName,int32> CommonSkillLevels;
	// 펫 업그레이드 정보(스킬종류, 레벨)
	UPROPERTY(SaveGame)
	TMap<FName,int32> PetUpgradeLevels;
	// 계정 공유 파츠 인벤토리
	UPROPERTY(SaveGame)
	TArray<FNSPartSaveData> OwnedParts;
	// 가장 최근에 플레이한 캐릭터 ID
	UPROPERTY(SaveGame)
	FName LastSelectedCharacterId;
	// 펫 관련 데이터
	UPROPERTY(SaveGame)
	FNSCompanionSaveData Companion;

	// 아웃런 목표 안내(웨이포인트 마커 + 텍스트) 진행 상태 —> 전부 영구 1회성

	// 이동 입력을 한 번이라도 발동했는지
	UPROPERTY(SaveGame)
	bool bMoveGuideDone = false;
	// 점프 입력을 한 번이라도 발동했는지
	UPROPERTY(SaveGame)
	bool bJumpGuideDone = false;
	// 대시 입력을 한 번이라도 발동했는지
	UPROPERTY(SaveGame)
	bool bDashGuideDone = false;
	// 캐릭터 선택 콘솔과 한 번이라도 상호작용했는지
	UPROPERTY(SaveGame)
	bool bCharacterConsoleGuideDone = false;
	// 게임시작 콘솔과 한 번이라도 상호작용했는지
	UPROPERTY(SaveGame)
	bool bReadyConsoleGuideDone = false;
	// 해금 후 첫 상호작용(안내 완료)까지 마친 NPC 집합
	UPROPERTY(SaveGame)
	TSet<FName> GuidedNPCIds;

	// 직업 단위
	// Key : 캐릭터 FName, Value : 세이브 데이터
	UPROPERTY(SaveGame)
	TMap<FName, FNSCharacterSaveData> Characters;
};
