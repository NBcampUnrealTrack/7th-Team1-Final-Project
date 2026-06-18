// Copyright 2026 One Team. All rights reserved.


#include "NSProgressionSubsystem.h"
#include "NeoSanctum/System/NSSaveGameSubsystem.h"
#include "NeoSanctum/Progression/Save/NSPermanentSaveGame.h"



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

void UNSProgressionSubsystem::SetEquippedPart(FName CharacterId, FName PartId)
{
	UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save || CharacterId.IsNone())
	{
		return;
	}
	
	FNSCharacterSaveData& Slot = Save->Characters.FindOrAdd(CharacterId);

	// 인런 진입시 파츠는 1개. None이면 해제.
	Slot.EquippedPartIds.Reset();
	if (!PartId.IsNone())
	{
		Slot.EquippedPartIds.Add(PartId);
	}
	
	SaveNow();
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

FName UNSProgressionSubsystem::GetEquippedPart(FName CharacterId) const
{
	const UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save)
	{
		return NAME_None;
	}
	
	const FNSCharacterSaveData* Slot = Save->Characters.Find(CharacterId);
	if (!Slot || Slot->EquippedPartIds.Num() == 0)
	{
		return NAME_None;
	}
	
	return Slot->EquippedPartIds[0];
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
