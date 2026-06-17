// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"

UNSPlayerProgressComponent::UNSPlayerProgressComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSPlayerProgressComponent::UnlockNPC(const FName& NPCId)
{
	// 계정단위 저장, 서버에서 호출
	UnlockedNPCIds.Add(NPCId);
}

void UNSPlayerProgressComponent::AddCommonCurrency(int64 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	CommonCurrency += Amount;
	// 테스트용 임시 로그 (드롭 테이블 연동 후 삭제)
	UE_LOG(LogTemp, Log, TEXT("[Currency] 공통재화 +%lld 적립 → 영구 누적 %lld"), Amount, CommonCurrency);
}

void UNSPlayerProgressComponent::AddJobCurrency(int64 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	JobCurrency += Amount;
	// 테스트용 임시 로그 (드롭 테이블 연동 후 삭제)
	UE_LOG(LogTemp, Log, TEXT("[Currency] 스킬재화 +%lld 적립 → 영구 누적 %lld"), Amount, JobCurrency);
}

namespace
{
	void ConvertMapToArray(const TMap<FName, int32>& SourceMap, TArray<FNSNodeLevel>& OutArray)
	{
		OutArray.Reset();
		OutArray.Reserve(SourceMap.Num());
		for (const TPair<FName, int32>& NodePair : SourceMap)
		{
			FNSNodeLevel NodeLevel;
			NodeLevel.NodeId = NodePair.Key;
			NodeLevel.Level = NodePair.Value;
			OutArray.Add(NodeLevel);
		}
	}

	void ConvertArrayToMap(const TArray<FNSNodeLevel>& SourceArray, TMap<FName, int32>& OutMap)
	{
		OutMap.Reset();
		OutMap.Reserve(SourceArray.Num());
		for (const FNSNodeLevel& NodeLevel : SourceArray)
		{
			OutMap.Add(NodeLevel.NodeId, NodeLevel.Level);
		}
	}
}

void UNSPlayerProgressComponent::BuildPayload(FNSProgressPayload& OutPayload) const
{
	OutPayload.CommonCurrency = CommonCurrency;
	OutPayload.UnlockedNPCIds = UnlockedNPCIds.Array();
	ConvertMapToArray(CommonSkillLevels, OutPayload.CommonSkillLevels);
	ConvertMapToArray(PetUpgradeLevels, OutPayload.PetUpgradeLevels);

	OutPayload.ActiveCharacterId = ActiveCharacterId;
	OutPayload.JobCurrency = JobCurrency;
	OutPayload.EquippedPartIds = EquippedPartIds;
	ConvertMapToArray(CharacterSkillLevels, OutPayload.CharacterSkillLevels);
}

void UNSPlayerProgressComponent::ApplyPayload(const FNSProgressPayload& Payload)
{
	CommonCurrency = Payload.CommonCurrency;
	UnlockedNPCIds = TSet<FName>(Payload.UnlockedNPCIds);
	ConvertArrayToMap(Payload.CommonSkillLevels, CommonSkillLevels);
	ConvertArrayToMap(Payload.PetUpgradeLevels, PetUpgradeLevels);

	ActiveCharacterId = Payload.ActiveCharacterId;
	JobCurrency = Payload.JobCurrency;
	EquippedPartIds = Payload.EquippedPartIds;
	ConvertArrayToMap(Payload.CharacterSkillLevels, CharacterSkillLevels);
}
