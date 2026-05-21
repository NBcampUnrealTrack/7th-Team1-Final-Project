// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NeoSanctum/Progression/Save/NSPermanentSaveGame.h"
#include "NSPlayerProgressComponent.generated.h"

UCLASS(ClassGroup=(NeoSanctum), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSPlayerProgressComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSPlayerProgressComponent();

	// 세이브에서 LastSelectedCharacterId 슬롯을 자동 활성화. 슬롯이 없으면 빈 상태로 시작
	void InitFromSaveData(const UNSPermanentSaveGame* SaveData);

	// 현재 활성 캐릭터 슬롯만 OutSaveData에 기록. 다른 캐릭터 데이터는 Subsystem에서 머지됨
	void PopulateSaveData(UNSPermanentSaveGame* OutSaveData) const;

	// 캐릭터 전환. SaveData에서 해당 슬롯을 로드하며, 슬롯이 없으면 신규 캐릭터로 빈 상태에서 시작
	UFUNCTION(BlueprintCallable)
	void SetActiveCharacter(const FName& InCharacterId, const UNSPermanentSaveGame* SaveData);

	UFUNCTION(BlueprintCallable)
	FName GetActiveCharacterId() const { return ActiveCharacterId; }

	UFUNCTION(BlueprintCallable)
	bool IsNPCUnlocked(const FName& NPCId) const;

	UFUNCTION(BlueprintCallable)
	void UnlockNPC(const FName& NPCId);

	UFUNCTION(BlueprintCallable)
	bool IsSkillUnlocked(const FName& SkillId) const;

	UFUNCTION(BlueprintCallable)
	void UnlockSkill(const FName& SkillId);

	bool IsDirty() const { return bDirty; }
	void ClearDirty() { bDirty = false; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// 슬롯 데이터를 컴포넌트 필드로 적재
	void LoadSlot(const FName& InCharacterId, const UNSPermanentSaveGame* SaveData);

	UPROPERTY(Replicated)
	FName ActiveCharacterId;

	UPROPERTY(Replicated)
	int64 TotalCurrency = 0;

	UPROPERTY(Replicated)
	TArray<FName> EquippedPartIds;

	UPROPERTY(Replicated)
	TArray<FName> UnlockedSkillIds;

	UPROPERTY(Replicated)
	TArray<FName> UnlockedNPCIds;

	bool bDirty = false;
};