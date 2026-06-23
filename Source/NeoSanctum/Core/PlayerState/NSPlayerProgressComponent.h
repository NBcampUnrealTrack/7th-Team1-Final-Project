// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NeoSanctum/Core/PlayerState/NSProgressTypes.h"
#include "GameplayTagContainer.h"
#include "NSPlayerProgressComponent.generated.h"


UCLASS(ClassGroup=(NeoSanctum), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSPlayerProgressComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSPlayerProgressComponent();
	
	// 컴포넌트 -> 페이로드
	void BuildPayload(FNSProgressPayload& OutPayload) const; 
	
	// 페이로드 -> 컴포넌트
	void ApplyPayload(const FNSProgressPayload& Payload);
	
	// 조회용 함수 (폰 스폰 시 스탯 반영 / UI 사용 용도)
	UFUNCTION(BlueprintCallable)
	FName GetActiveCharacterId() const { return ActiveCharacterId; }
	UFUNCTION(BlueprintCallable)
	bool IsNPCUnlocked(const FName& NPCId) const { return UnlockedNPCIds.Contains(NPCId); }
	int64 GetCommonCurrency() const { return CommonCurrency; }
	int64 GetJobCurrency() const { return JobCurrency; }
	const FNSPartSaveData& GetEquippedPart() const { return EquippedPart; }
	const TMap<FName,int32>& GetCommonSkillLevels() const { return CommonSkillLevels; }
	const TMap<FName,int32>& GetCharacterSkillLevels() const { return CharacterSkillLevels; }
	const TMap<FName,int32>& GetPetUpgradeLevels() const { return PetUpgradeLevels; }
	FGameplayTag GetSelectedCompanion() const { return SelectedCompanionTag; }
	const TMap<FGameplayTag,int32>& GetCompanionNodeLevels() const { return CompanionNodeLevels; }
	

	UFUNCTION(BlueprintCallable)
	void UnlockNPC(const FName& NPCId);
	
	// 인런 재화 적립용 함수들
	UFUNCTION(BlueprintCallable)
	void AddCommonCurrency(int64 Amount);
	UFUNCTION(BlueprintCallable)
	void AddJobCurrency(int64 Amount);


private:
	// 계정 단위
	int64 CommonCurrency = 0;
	TSet<FName> UnlockedNPCIds;
	TMap<FName,int32> CommonSkillLevels;
	TMap<FName,int32> PetUpgradeLevels;
	FGameplayTag SelectedCompanionTag;
	TMap<FGameplayTag,int32> CompanionNodeLevels;
	
	// 직업(캐릭터) 단위
	FName ActiveCharacterId;
	int64 JobCurrency = 0;
	FNSPartSaveData EquippedPart;
	TMap<FName,int32> CharacterSkillLevels;
};