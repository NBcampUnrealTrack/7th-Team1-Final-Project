// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NSProgressionSubsystem.generated.h"

class UNSSaveGameSubsystem;
class UNSPermanentSaveGame;

/**
 *	아웃게임 진행도 쓰는 용도의 서브시스템
 *	각 로컬은 CachedData 수정, 즉시 저장
 *	서버 동기화는 런 시작 업로드가 담당함
 */
UCLASS()
class NEOSANCTUM_API UNSProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	// 공용 스킬 강화
	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrade")
	bool UpgradeCommonSkill(FName NodeId, int32 NewLevel, int64 Cost);
	
	// 캐릭터 스킬 강화
	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrade")
	bool UpgradeCharacterSkill(FName CharacterId, FName NodeId, int32 NewLevel, int64 Cost);
	
	// 펫 업그레이드
	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrade")
	bool UpgradePet(FName PetNodeId, int32 NewLevel, int64 Cost);

	// 치트 확인용 임시 함수
	// 특정 NPC 해금 (로컬 CachedData에 추가 후 즉시 저장).
	UFUNCTION(BlueprintCallable, Category = "Progression|Unlock")
	void UnlockNPC(FName NPCId);

	// 치트 확인용 임시 함수
	// 특정 NPC 잠금 해제 취소 (로컬 CachedData에서 제거 후 즉시 저장).
	UFUNCTION(BlueprintCallable, Category = "Progression|Unlock")
	void LockNPC(FName NPCId);
	
	// 슬롯 언락
	UFUNCTION(BlueprintCallable, Category = "Progression|Part")
	bool UnlockSlot(FName CharacterId, FGameplayTag Slot);

	UFUNCTION(BlueprintPure, Category = "Progression|Part")
	bool IsSlotUnlocked(FName CharacterId, FGameplayTag Slot) const;

	UFUNCTION(BlueprintPure, Category = "Progression|Part")
	int64 GetSlotUnlockCost(FGameplayTag Slot) const;

	// 장착: 소유 검증 후 캐릭터 장착 참조 설정
	UFUNCTION(BlueprintCallable, Category = "Progression|Part")
	void SetEquippedPart(FName CharacterId, TSoftObjectPtr<UNSPartDefinition> Definition, ENSPartRarity Rarity);
	
	UFUNCTION(BlueprintCallable, Category="Progression|Companion")
	bool UpgradeCompanionNode(FGameplayTag CompanionTag, FGameplayTag NodeTag, int32 MaxLevel, int64 Cost);
	
	UFUNCTION(BlueprintCallable, Category="Progression|Companion")
	bool SelectCompanion(FGameplayTag CompanionTag, FGameplayTag RequiredCompanionTag, int32 RequiredCount);
	
	UFUNCTION(BlueprintPure, Category="Progression|Companion")
	bool CanSelectCompanion(FGameplayTag RequiredCompanionTag, int32 RequiredCount) const;
	
	// ---------- 조회 (UI 표시용) ----------
	
	UFUNCTION(BlueprintPure, Category = "Progression|Query")
	int64 GetCommonCurrency() const;

	// 공통 업그레이드 노드를 TargetLevel까지 올리는데 필요한 비용.
	UFUNCTION(BlueprintPure, Category = "Progression|Query")
	int64 GetCommonUpgradeCost(FName NodeId, int32 TargetLevel) const;

	// 공통 업그레이드 노드의 최대 레벨.
	UFUNCTION(BlueprintPure, Category = "Progression|Query")
	int32 GetCommonUpgradeMaxLevel(FName NodeId) const;

	UFUNCTION(BlueprintPure, Category = "Progression|Query")
	int64 GetJobCurrency(FName CharacterId) const;

	UFUNCTION(BlueprintPure, Category = "Progression|Query")
	int32 GetCommonSkillLevel(FName NodeId) const;

	UFUNCTION(BlueprintPure, Category = "Progression|Query")
	int32 GetCharacterSkillLevel(FName CharacterId, FName NodeId) const;

	UFUNCTION(BlueprintPure, Category = "Progression|Query")
	int32 GetPetLevel(FName PetNodeId) const;

	// 장착 파츠가 없으면 NAME_None 반환
	UFUNCTION(BlueprintPure, Category = "Progression|Part")
	FNSPartSaveData GetEquippedPart(FName CharacterId) const;

	UFUNCTION(BlueprintPure, Category = "Progression|Query")
	FName GetLastSelectedCharacterId() const;
	
	UFUNCTION(BlueprintPure, Category="Progression|Companion")
	FGameplayTag GetSelectedCompanion() const;
	
	UFUNCTION(BlueprintPure, Category="Progression|Companion")
	int32 GetCompanionNodeLevel(FGameplayTag NodeTag) const;
	
	UFUNCTION(BlueprintCallable, Category = "Progression|Part")
	bool PurchasePart(FName CharacterId, TSoftObjectPtr<UNSPartDefinition> Definition, ENSPartRarity Rarity);
	UFUNCTION(BlueprintPure, Category = "Progression|Part")
	bool IsPartOwned(TSoftObjectPtr<UNSPartDefinition> Definition, ENSPartRarity Rarity) const;
	const TArray<FNSPartSaveData>& GetOwnedParts() const;


private:
	UNSSaveGameSubsystem* GetSaveSubsystem() const;
	UNSPermanentSaveGame* GetSaveData() const;
	void SaveNow();
};
