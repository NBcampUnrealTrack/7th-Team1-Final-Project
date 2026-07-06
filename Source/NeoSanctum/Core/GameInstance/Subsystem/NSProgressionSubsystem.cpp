// Copyright 2026 One Team. All rights reserved.


#include "NSProgressionSubsystem.h"
#include "Engine/AssetManager.h"
#include "NeoSanctum/System/NSSaveGameSubsystem.h"
#include "NeoSanctum/Progression/Save/NSPermanentSaveGame.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/CommonUpgrade/NSCommonUpgradeTypes.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"



bool UNSProgressionSubsystem::UpgradeCommonSkill(FName NodeId, int32 NewLevel, int64 Cost)
{
	UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save || NodeId.IsNone() || Cost < 0 || Save->CommonCurrency < Cost)
	{
		return false;
	}

	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(GetGameInstance());
	const FNSCommonUpgradeNodeRow* Row = DataSubsystem ? DataSubsystem->GetCommonUpgradeNodeRow(NodeId) : nullptr;

	if (!Row || NewLevel > Row->MaxLevel)
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

int64 UNSProgressionSubsystem::GetCommonUpgradeCost(FName NodeId, int32 TargetLevel) const
{
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(GetGameInstance());
	const FNSCommonUpgradeNodeRow* Row = DataSubsystem ? DataSubsystem->GetCommonUpgradeNodeRow(NodeId) : nullptr;
	if (!Row || TargetLevel < 1)
	{
		return 0;
	}

	const float PercentMultiplier =
		FMath::Pow(1.0f + Row->CostGrowthPercent * 0.01f, static_cast<float>(TargetLevel - 1));
	const float FlatGrowth = static_cast<float>(Row->CostGrowthFlat) * static_cast<float>(TargetLevel - 1);

	return FMath::CeilToInt64(Row->BaseCost * PercentMultiplier + FlatGrowth);
}

int32 UNSProgressionSubsystem::GetCommonUpgradeMaxLevel(FName NodeId) const
{
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(GetGameInstance());
	const FNSCommonUpgradeNodeRow* Row = DataSubsystem ? DataSubsystem->GetCommonUpgradeNodeRow(NodeId) : nullptr;

	return Row ? Row->MaxLevel : 0;
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
	const FNSCharacterSaveData* CharData = Save->Characters.Find(CharacterId);
	if (!CharData || CharData->EquippedPartDefinition.IsNull())
	{
		return FNSPartSaveData();
	}

	const FNSPartSaveData* Owned = Save->OwnedParts.FindByPredicate(
		[CharData](const FNSPartSaveData& P)
		{
			return P.Definition == CharData->EquippedPartDefinition
				&& P.Rarity == CharData->EquippedPartRarity;
		});
	return Owned ? *Owned : FNSPartSaveData();
}

FName UNSProgressionSubsystem::GetLastSelectedCharacterId() const
{
	UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save)
	{
		return NAME_None;
	}

	// 최초 플레이 등으로 아직 저장된 값이 없으면 Ranger로 초기화
	if (Save->LastSelectedCharacterId.IsNone())
	{
		Save->LastSelectedCharacterId = TEXT("DA_Character_Ranger");
		const_cast<UNSProgressionSubsystem*>(this)->SaveNow();
	}

	return Save->LastSelectedCharacterId;
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

bool UNSProgressionSubsystem::UnlockSlot(FName CharacterId, FGameplayTag Slot)
{
	UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save || CharacterId.IsNone())
	{
		NS_LOG(LogNS, Warning, "[SlotUnlock] 실패: Save 없음 또는 CharacterId=None. CharacterId={CharacterId}",
			("CharacterId", CharacterId.ToString()));
		return false;
	}
	if (IsSlotUnlocked(CharacterId, Slot))
	{
		NS_LOG(LogNS, Warning, "[SlotUnlock] 실패: 이미 해금됨. Slot={Slot}", ("Slot", Slot.ToString()));
		return false;
	}

	UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(GetGameInstance());
	if (!DataSS)
	{
		NS_LOG(LogNS, Warning, "[SlotUnlock] 실패: DataSubsystem 없음.");
		return false;
	}
	const FNSPartSlotRow* Row = DataSS->GetSlotRow(Slot);
	if (!Row || !Row->bEnabled)
	{
		NS_LOG(LogNS, Warning, "[SlotUnlock] 실패: GetSlotRow 실패 또는 비활성. Slot={Slot}, RowFound={RowFound}",
			("Slot", Slot.ToString()), ("RowFound", Row != nullptr));
		return false;
	}
	if (Save->CommonCurrency < Row->UnlockCost)
	{
		NS_LOG(LogNS, Warning, "[SlotUnlock] 실패: 재화 부족. 보유={Currency}, 필요={Cost}",
			("Currency", Save->CommonCurrency), ("Cost", Row->UnlockCost));
		return false;
	}

	Save->CommonCurrency -= Row->UnlockCost;
	Save->Characters.FindOrAdd(CharacterId).UnlockedSlots.Add(Slot);
	SaveNow();
	NS_LOG(LogNS, Log, "[SlotUnlock] 성공: Slot={Slot}, 잔여재화={Currency}",
		("Slot", Slot.ToString()), ("Currency", Save->CommonCurrency));
	return true;
}

bool UNSProgressionSubsystem::IsSlotUnlocked(FName CharacterId, FGameplayTag Slot) const
{
	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(GetGameInstance());
	if (DataSS)
	{
		const FNSPartSlotRow* Row = DataSS->GetSlotRow(Slot);
		if (Row && Row->bUnlockedByDefault)
		{
			return true;
		}
	}

	const UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save || CharacterId.IsNone())
	{
		return false;
	}
	const FNSCharacterSaveData* CharData = Save->Characters.Find(CharacterId);
	return CharData && CharData->UnlockedSlots.Contains(Slot);
}

int64 UNSProgressionSubsystem::GetSlotUnlockCost(FGameplayTag Slot) const
{
	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(GetGameInstance());
	if (!DataSS)
	{
		return 0;
	}
	const FNSPartSlotRow* Row = DataSS->GetSlotRow(Slot);
	return Row ? Row->UnlockCost : 0;
}

bool UNSProgressionSubsystem::PurchasePart(FName CharacterId, TSoftObjectPtr<UNSPartDefinition> Definition, ENSPartRarity Rarity)
{
	UNSPermanentSaveGame* Save = GetSaveData();
	if (!Save || CharacterId.IsNone() || Definition.IsNull())
	{
		NS_LOG(LogNS, Warning, "[Purchase] 실패: Save={HasSave}, CharacterId={CharId}, Definition={Definition}",
			("HasSave", Save != nullptr), ("CharId", CharacterId.ToString()), ("Definition", Definition.ToString()));
		return false;
	}
	if (IsPartOwned(Definition, Rarity))
	{
		NS_LOG(LogNS, Warning, "[Purchase] 실패: 이미 소유중인 파츠입니다: {Definition}", ("Definition", Definition.ToString()));
		return false;
	}

	const FPrimaryAssetId DefId =
		UAssetManager::Get().GetPrimaryAssetIdForPath(Definition.ToSoftObjectPath());
	UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(GetGameInstance());
	if (!DataSS)
	{
		NS_LOG(LogNS, Warning, "[Purchase] 실패: DataSubsystem이 없습니다.");
		return false;
	}
	const FNSPartDefinitionRow* Row = DataSS->GetPartRow(DefId);
	if (!Row)
	{
		NS_LOG(LogNS, Warning, "[Purchase] 실패: GetPartRow 실패. DefId={DefId}", ("DefId", DefId.ToString()));
		return false;
	}
	if (!IsSlotUnlocked(CharacterId, Row->PartSlot))
	{
		NS_LOG(LogNS, Warning, "[Purchase] 실패: 슬롯이 언락되지 않음. Slot={Slot}", ("Slot", Row->PartSlot.ToString()));
		return false;
	}
	if (Save->CommonCurrency < Row->UnlockCost)
	{
		NS_LOG(LogNS, Warning, "[Purchase] 실패: 재화 부족. Currency={Currency}, Cost={Cost}",
			("Currency", Save->CommonCurrency), ("Cost", Row->UnlockCost));
		return false;
	}

	const FNSPartUpgradeRow* UpgradeRow = DataSS->GetPartUpgradeRow(Rarity);

	FNSPartSaveData New;
	New.Definition = Definition;
	New.Rarity = Rarity;
	New.EnhanceLevel = 0;
	New.Value = UpgradeRow ? FMath::RandRange(UpgradeRow->ValueRange.Min, UpgradeRow->ValueRange.Max) : 0.f;

	Save->CommonCurrency -= Row->UnlockCost;
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
