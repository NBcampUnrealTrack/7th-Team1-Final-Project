// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
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
	
	// 인런 진입시 파츠 교체용
	UFUNCTION(BlueprintCallable, Category = "Progression|Loadout")
	void SetEquippedPart(FName CharacterId, FName PartId);
	
	// ---------- 조회 (UI 표시용) ----------
	
	UFUNCTION(BlueprintPure, Category = "Progression|Query")
	int64 GetCommonCurrency() const;

	UFUNCTION(BlueprintPure, Category = "Progression|Query")
	int64 GetJobCurrency(FName CharacterId) const;

	UFUNCTION(BlueprintPure, Category = "Progression|Query")
	int32 GetCommonSkillLevel(FName NodeId) const;

	UFUNCTION(BlueprintPure, Category = "Progression|Query")
	int32 GetCharacterSkillLevel(FName CharacterId, FName NodeId) const;

	UFUNCTION(BlueprintPure, Category = "Progression|Query")
	int32 GetPetLevel(FName PetNodeId) const;

	// 장착 파츠가 없으면 NAME_None 반환
	UFUNCTION(BlueprintPure, Category = "Progression|Query")
	FName GetEquippedPart(FName CharacterId) const;


private:
	UNSSaveGameSubsystem* GetSaveSubsystem() const;
	UNSPermanentSaveGame* GetSaveData() const;
	void SaveNow();
};
