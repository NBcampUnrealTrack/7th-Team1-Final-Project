// Copyright 2026 One Team. All rights reserved.


#include "NSProgressionSubsystem.h"
#include "NeoSanctum/System/NSSaveGameSubsystem.h"
#include "NeoSanctum/Progression/Save/NSPermanentSaveGame.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"



bool UNSProgressionSubsystem::UpgradeCommonSkill(FName NodeId, int32 NewLevel, int64 Cost)
{
	UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save || NodeId.IsNone() || Cost < 0 || Save->CommonCurrency < Cost)
	{
		return false;
	}
	
	Save->CommonCurrency -= Cost;
	Save->CommonSkillLevels.Add(NodeId, NewLevel);
	SaveNow();
	
	return true;
}

void UNSProgressionSubsystem::UnlockNPC(FName NPCId)
{
	UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save || NPCId.IsNone())
	{
		return;
	}

	bool bAlreadyUnlocked = false;
	Save->UnlockedNPCIds.Add(NPCId, &bAlreadyUnlocked);
	if (bAlreadyUnlocked)
	{
		return;
	}

	SaveNow();
}

void UNSProgressionSubsystem::LockNPC(FName NPCId)
{
	UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save || NPCId.IsNone())
	{
		return;
	}

	if (Save->UnlockedNPCIds.Remove(NPCId) == 0)
	{
		return;
	}

	SaveNow();
}

bool UNSProgressionSubsystem::UpgradeCharacterSkill(FName CharacterId, FName NodeId, int32 NewLevel, int64 Cost)
{
	UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save || CharacterId.IsNone() || NodeId.IsNone() || Cost < 0)
	{
		return false;
	}
	
	FNSCharacterSaveData& Slot = Save->Characters.FindOrAdd(CharacterId);
	if (Slot.JobCurrency < Cost)
	{
		return false;
	}
	
	Slot.JobCurrency -= Cost;
	Slot.CharacterSkillLevels.Add(NodeId, NewLevel);
	SaveNow();
	
	return true;
}

bool UNSProgressionSubsystem::UpgradePet(FName PetNodeId, int32 NewLevel, int64 Cost)
{
	UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save || PetNodeId.IsNone() || Cost < 0 || Save->CommonCurrency < Cost)
	{
		return false;
	}
	
	// 펫은 계정 공유
	Save->CommonCurrency -= Cost;
	Save->PetUpgradeLevels.Add(PetNodeId, NewLevel);
	SaveNow();
	
	return true;
}

int64 UNSProgressionSubsystem::GetCommonCurrency() const
{
	const UNSPermanentSaveGame* Save = GetSaveData();
	
	return Save ? Save->CommonCurrency : 0;
}

int64 UNSProgressionSubsystem::GetJobCurrency(FName CharacterId) const
{
	const UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save)
	{
		return 0;
	}
	
	const FNSCharacterSaveData* Slot = Save->Characters.Find(CharacterId);
	
	return Slot ? Slot->JobCurrency : 0;
}

int32 UNSProgressionSubsystem::GetCommonSkillLevel(FName NodeId) const
{
	const UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save)
	{
		return 0;
	}
	
	const int32* Level = Save->CommonSkillLevels.Find(NodeId);
	
	return Level ? *Level : 0;
}

int32 UNSProgressionSubsystem::GetCharacterSkillLevel(FName CharacterId, FName NodeId) const
{
	const UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save)
	{
		return 0;
	}
	
	const FNSCharacterSaveData* Slot = Save->Characters.Find(CharacterId);
	if (!Slot)
	{
		return 0;
	}
	
	const int32* Level = Slot->CharacterSkillLevels.Find(NodeId);
	
	return Level ? *Level : 0;
}

int32 UNSProgressionSubsystem::GetPetLevel(FName PetNodeId) const
{
	const UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save)
	{
		return 0;
	}
	
	const int32* Level = Save->PetUpgradeLevels.Find(PetNodeId);
	
	return Level ? *Level : 0;
}

FNSPartSaveData UNSProgressionSubsystem::GetEquippedPart(FName CharacterId) const
{
	const UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save)
	{
		return FNSPartSaveData();
	}
	const FNSCharacterSaveData* Slot = Save->Characters.Find(CharacterId);
	if (!Slot || Slot->EquippedPartDefinition.IsNull())
	{
		
		return FNSPartSaveData();
	}

	const FNSPartSaveData* Owned = Save->OwnedParts.FindByPredicate(
		[Slot](const FNSPartSaveData& P)
		{
			return P.Definition == 
				Slot->EquippedPartDefinition && P.Rarity == Slot->EquippedPartRarity;
		});
	
	return Owned ? *Owned : FNSPartSaveData();
}

FGameplayTag UNSProgressionSubsystem::GetSelectedCompanion() const
{
	const UNSPermanentSaveGame* Save = GetSaveData();
	
	return Save ? Save->Companion.SelectedCompanionTag : FGameplayTag();
}

int32 UNSProgressionSubsystem::GetCompanionNodeLevel(FGameplayTag NodeTag) const
{
	const UNSPermanentSaveGame* Save = GetSaveData();
	
	return Save ? Save->Companion.NodeLevels.FindRef(NodeTag) : 0;
}

bool UNSProgressionSubsystem::ResetCompanionUpgrades()
{
	UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save)
	{
		return false;
	}

	// 선택된 펫과 재화는 유지하고 강화 정보만 초기화
	Save->Companion.NodeLevels.Reset();
	Save->Companion.UpgradeCounts.Reset();

	SaveNow();
	return true;
}

bool UNSProgressionSubsystem::PurchasePart(TSoftObjectPtr<UNSPartDefinition> Definition, ENSPartRarity Rarity, int64 Cost)
{
	UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save || Definition.IsNull() || Cost < 0 || Save->CommonCurrency < Cost)
	{
		return false;
	}
	if (IsPartOwned(Definition, Rarity))
	{
		return false;
	}

	UNSPartDefinition* Def = Definition.LoadSynchronous();
	if (!Def)
	{
		return false;
	}
	const FNSPartValueRange* Range = Def->ValueRange.Find(Rarity);

	FNSPartSaveData New;
	New.Definition = Definition;
	New.Rarity = Rarity;
	New.EnhanceLevel = 0;
	// 값 1회 롤 후 고정
	New.Value = Range ? FMath::RandRange(Range->Min, Range->Max) : 0.f;

	Save->CommonCurrency -= Cost;
	Save->OwnedParts.Add(New);
	SaveNow();
	
	return true;
}

void UNSProgressionSubsystem::SetEquippedPart(FName CharacterId, TSoftObjectPtr<UNSPartDefinition> Definition, ENSPartRarity Rarity)
{
	UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save || CharacterId.IsNone())
	{
		return;
	}
	
	// 해제(null)이거나, 소유한 파츠만 장착 가능
	if (!Definition.IsNull() && !IsPartOwned(Definition, Rarity)) { return; }

	FNSCharacterSaveData& Slot = Save->Characters.FindOrAdd(CharacterId);
	// null이면 해제
	Slot.EquippedPartDefinition = Definition;
	Slot.EquippedPartRarity = Rarity;
	SaveNow();
}

bool UNSProgressionSubsystem::UpgradeCompanionNode(FGameplayTag CompanionTag, FGameplayTag NodeTag, int32 MaxLevel, int64 Cost)
{
	UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save || !NodeTag.IsValid() || Cost < 0)
	{
		return false;
	}
	
	// UI가 넘긴 Max로 게이트
	const int32 NewLevel = Save->Companion.NodeLevels.FindRef(NodeTag) + 1;
	if (NewLevel > MaxLevel)
	{
		return false;
	}   
	
	// 공통재화
	if (Save->CommonCurrency < Cost)
	{
		return false;
	}   

	Save->CommonCurrency -= Cost;
	Save->Companion.NodeLevels.Add(NodeTag, NewLevel);
	Save->Companion.UpgradeCounts.FindOrAdd(CompanionTag)++;
	SaveNow();
	
	return true;
}

bool UNSProgressionSubsystem::SelectCompanion(
	FGameplayTag CompanionTag,
	FGameplayTag RequiredCompanionTag, 
	int32 RequiredCount)
{
	UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save || !CompanionTag.IsValid() || !CanSelectCompanion(RequiredCompanionTag, RequiredCount))
	{
		return false;
	}
	
	Save->Companion.SelectedCompanionTag = CompanionTag;
	SaveNow();
	
	return true;
}

bool UNSProgressionSubsystem::CanSelectCompanion(FGameplayTag RequiredCompanionTag, int32 RequiredCount) const
{
	// 전제 없음 = 항상 가능
	if (!RequiredCompanionTag.IsValid())
	{
		return true;
	} 
	
	const UNSPermanentSaveGame* Save = GetSaveData();
	
	return Save && Save->Companion.UpgradeCounts.FindRef(RequiredCompanionTag) >= RequiredCount;
}

bool UNSProgressionSubsystem::IsPartOwned(TSoftObjectPtr<UNSPartDefinition> Definition, ENSPartRarity Rarity) const
{
	const UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save || Definition.IsNull())
	{
		return false;
	}
	
	return Save->OwnedParts.ContainsByPredicate(
		[&Definition, Rarity](const FNSPartSaveData& P)
		{
			return P.Definition == Definition && P.Rarity == Rarity;
		});
}

const TArray<FNSPartSaveData>& UNSProgressionSubsystem::GetOwnedParts() const
{
	static const TArray<FNSPartSaveData> Empty;
	const UNSPermanentSaveGame* Save = GetSaveData();
	
	return Save ? Save->OwnedParts : Empty;
}

UNSSaveGameSubsystem* UNSProgressionSubsystem::GetSaveSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	
	return GameInstance ? GameInstance->GetSubsystem<UNSSaveGameSubsystem>() : nullptr;
}

UNSPermanentSaveGame* UNSProgressionSubsystem::GetSaveData() const
{
	UNSSaveGameSubsystem* SaveSubsystem = GetSaveSubsystem();
	
	return SaveSubsystem ? SaveSubsystem->GetCachedPermanentData() : nullptr;
}

void UNSProgressionSubsystem::SaveNow()
{
	UNSSaveGameSubsystem* SaveSubsystem = GetSaveSubsystem();
	UNSPermanentSaveGame* Save = SaveSubsystem ? SaveSubsystem->GetCachedPermanentData() : nullptr;
	if (SaveSubsystem && Save)
	{
		// 같은 CachedData 객체를 넘기므로 머지 분기를 건너뛰고 그대로 저장됨
		SaveSubsystem->SavePermanent(Save, FNSSaveComplete());
	}
}
