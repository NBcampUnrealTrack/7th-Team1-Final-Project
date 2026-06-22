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
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	// 음수 잔액 방지
	CommonCurrency = FMath::Max<int64>(0, CommonCurrency + Amount);
}

void UNSPlayerProgressComponent::AddJobCurrency(int64 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	// 음수 잔액 방지
	JobCurrency = FMath::Max<int64>(0, JobCurrency + Amount);
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
	OutPayload.EquippedPart = EquippedPart;
	ConvertMapToArray(CharacterSkillLevels, OutPayload.CharacterSkillLevels);
	OutPayload.SelectedCompanionTag = SelectedCompanionTag;
	OutPayload.CompanionNodeLevels.Reset();
	for (const TPair<FGameplayTag, int32>& NodeLevelPair : CompanionNodeLevels)
	{
		FNSCompanionNodeLevel NodeLevelEntry;
		NodeLevelEntry.Tag   = NodeLevelPair.Key;
		NodeLevelEntry.Level = NodeLevelPair.Value;
		OutPayload.CompanionNodeLevels.Add(NodeLevelEntry);
	}
}

void UNSPlayerProgressComponent::ApplyPayload(const FNSProgressPayload& Payload)
{
	CommonCurrency = Payload.CommonCurrency;
	UnlockedNPCIds = TSet<FName>(Payload.UnlockedNPCIds);
	ConvertArrayToMap(Payload.CommonSkillLevels, CommonSkillLevels);
	ConvertArrayToMap(Payload.PetUpgradeLevels, PetUpgradeLevels);

	ActiveCharacterId = Payload.ActiveCharacterId;
	JobCurrency = Payload.JobCurrency;
	EquippedPart = Payload.EquippedPart;
	ConvertArrayToMap(Payload.CharacterSkillLevels, CharacterSkillLevels);
	SelectedCompanionTag = Payload.SelectedCompanionTag;
	CompanionNodeLevels.Reset();
	for (const FNSCompanionNodeLevel& NodeLevelEntry : Payload.CompanionNodeLevels)
	{
		CompanionNodeLevels.Add(NodeLevelEntry.Tag, NodeLevelEntry.Level);
	}
}
