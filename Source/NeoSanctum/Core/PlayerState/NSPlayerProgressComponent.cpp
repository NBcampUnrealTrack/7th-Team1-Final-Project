// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "Net/UnrealNetwork.h"

UNSPlayerProgressComponent::UNSPlayerProgressComponent()
{
	// 복제 활성화
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSPlayerProgressComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UNSPlayerProgressComponent, ActiveCharacterId, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNSPlayerProgressComponent, TotalCurrency, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNSPlayerProgressComponent, EquippedPartIds, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNSPlayerProgressComponent, UnlockedSkillIds, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNSPlayerProgressComponent, UnlockedNPCIds, COND_OwnerOnly);
}

void UNSPlayerProgressComponent::InitFromSaveData(const UNSPermanentSaveGame* SaveData)
{
	if (!SaveData) return;

	LoadSlot(SaveData->LastSelectedCharacterId, SaveData);
	bDirty = false;
}

void UNSPlayerProgressComponent::PopulateSaveData(UNSPermanentSaveGame* OutSaveData) const
{
	if (!OutSaveData || ActiveCharacterId.IsNone()) return;

	FNSCharacterSaveData& Slot = OutSaveData->Characters.FindOrAdd(ActiveCharacterId);
	Slot.TotalCurrency = TotalCurrency;
	Slot.EquippedPartIds = EquippedPartIds;
	Slot.UnlockedSkillIds = TSet<FName>(UnlockedSkillIds);
	Slot.UnlockedNPCIds = TSet<FName>(UnlockedNPCIds);

	OutSaveData->LastSelectedCharacterId = ActiveCharacterId;
}

// 캐릭터 전환 시
void UNSPlayerProgressComponent::SetActiveCharacter(const FName& InCharacterId, const UNSPermanentSaveGame* SaveData)
{
	LoadSlot(InCharacterId, SaveData);
	// LastSelectedCharacterId가 갱신되도록 저장 트리거
	bDirty = true;
}

void UNSPlayerProgressComponent::LoadSlot(const FName& InCharacterId, const UNSPermanentSaveGame* SaveData)
{
	ActiveCharacterId = InCharacterId;

	const FNSCharacterSaveData* Slot = (SaveData && !InCharacterId.IsNone())
		? SaveData->Characters.Find(InCharacterId)
		: nullptr;

	if (Slot)
	{
		TotalCurrency = Slot->TotalCurrency;
		EquippedPartIds = Slot->EquippedPartIds;
		UnlockedSkillIds = Slot->UnlockedSkillIds.Array();
		UnlockedNPCIds = Slot->UnlockedNPCIds.Array();
	}
	else
	{
		TotalCurrency = 0;
		EquippedPartIds.Reset();
		UnlockedSkillIds.Reset();
		UnlockedNPCIds.Reset();
	}
}

bool UNSPlayerProgressComponent::IsNPCUnlocked(const FName& NPCId) const
{
	return UnlockedNPCIds.Contains(NPCId);
}

void UNSPlayerProgressComponent::UnlockNPC(const FName& NPCId)
{
	if (UnlockedNPCIds.Contains(NPCId)) return;
	UnlockedNPCIds.Add(NPCId);
	bDirty = true;
}

bool UNSPlayerProgressComponent::IsSkillUnlocked(const FName& SkillId) const
{
	return UnlockedSkillIds.Contains(SkillId);
}

void UNSPlayerProgressComponent::UnlockSkill(const FName& SkillId)
{
	if (UnlockedSkillIds.Contains(SkillId)) return;
	UnlockedSkillIds.Add(SkillId);
	bDirty = true;
}
