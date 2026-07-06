// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"

#include "Net/UnrealNetwork.h"

UNSPlayerProgressComponent::UNSPlayerProgressComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNSPlayerProgressComponent::UnlockNPC(const FName& NPCId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	bool bAlreadyUnlocked = false;
	UnlockedNPCIds.Add(NPCId, &bAlreadyUnlocked);
	// 이미 있으면 복제 갱신 불필요
	if (bAlreadyUnlocked)
	{
		return;   
	}
	
	SyncReplicatedPayloadFromCurrentState();
	BroadcastProgressChanged();
}

void UNSPlayerProgressComponent::AddCommonCurrency(int64 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	// 음수 잔액 방지
	CommonCurrency = FMath::Max<int64>(0, CommonCurrency + Amount);
	
	SyncReplicatedPayloadFromCurrentState();
	BroadcastProgressChanged();
}

void UNSPlayerProgressComponent::AddJobCurrency(int64 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	// 음수 잔액 방지
	JobCurrency = FMath::Max<int64>(0, JobCurrency + Amount);
	
	SyncReplicatedPayloadFromCurrentState();
	BroadcastProgressChanged();
}

void UNSPlayerProgressComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UNSPlayerProgressComponent, ReplicatedProgressPayload, COND_OwnerOnly);
}

void UNSPlayerProgressComponent::OnRep_ReplicatedProgressPayload()
{
	ApplyPayload(ReplicatedProgressPayload);
	BroadcastProgressChanged();
}

void UNSPlayerProgressComponent::SyncReplicatedPayloadFromCurrentState()
{
	BuildPayload(ReplicatedProgressPayload);

	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}

void UNSPlayerProgressComponent::BroadcastProgressChanged()
{
	OnProgressChanged.Broadcast();
	OnCurrencyChanged.Broadcast(CommonCurrency);
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
	
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SyncReplicatedPayloadFromCurrentState();
	}

	BroadcastProgressChanged();
}
