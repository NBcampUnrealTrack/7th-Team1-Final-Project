// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerState.h"
#include "NeoSanctum/Core/GameFlow/NSRunFlowType.h"
#include "UObject/PrimaryAssetId.h"
#include "NSPlayerState.generated.h"

class UNSCompanionDefinition;
class UNSCompanionProgressionComponent;
class UNSCombatStatComponent;
class UNSAbilitySystemComponent;
class UNSPlayerAttributeSet;
class UNSPlayerProgressComponent;
class UNSAugmentInventoryComponent;
class UNSCharacterData;
class UNSPartEquipComponent;
class UNSCurrencyComponent;

UCLASS()
class NEOSANCTUM_API ANSPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ANSPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UNSPlayerAttributeSet* GetPlayerAttributeSet() const;
	
	UNSCombatStatComponent* GetCombatStatComponent() const;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	
	bool IsReady() const { return bIsReady; }
	bool IsDead() const { return bIsDead; }
	// 사망 상태를 변경하기 위한 Setter
	void SetIsDead(bool bNewIsDead);
	
	void SetReady(bool bNewReady);
	
	// 클라에서 재화 액터에 오버랩 시 호출
	UFUNCTION(Server, Reliable)
	void Server_CollectCurrency(int32 DropId);
	
	UNSPlayerProgressComponent* GetProgressComponent() const { return ProgressComponent; }
	UNSCurrencyComponent* GetCurrencyComponent() const { return CurrencyComponent; }
	UNSPartEquipComponent* GetPartEquipComponent() const { return PartEquipComponent; }

	UNSAugmentInventoryComponent* GetAugmentInventory() const { return AugmentInventory; }

public:
	// 캐릭터 데이터 Setter
	UFUNCTION(BlueprintCallable, Category = "Character|Data")
	void SetCurrentCharacterData(UNSCharacterData* InCharacterData);

	UFUNCTION(BlueprintCallable, Category = "Character|Data")
	void SetCurrentCharacterDataId(FPrimaryAssetId InCharacterDataId);

	// @민재 : TagSetter
	void SetCurrentCompanionDefinitionTag(FGameplayTag CompanionTag);
	
	UFUNCTION(BlueprintPure, Category = "Character|Data")
	FPrimaryAssetId GetCurrentCharacterDataId() const { return CurrentCharacterDataId; }
	
	// 기존 캐릭터 id 받아오는 용
	FPrimaryAssetId GetDefaultCharacterDataId() const { return DefaultCharacterDataId; }

	// 캐릭터 데이터 Getter
	UFUNCTION(BlueprintPure, Category = "Character|Data")
	UNSCharacterData* GetCurrentCharacterData() const;
	
	// @민재 : Companion 데이터 Getter
	UFUNCTION(BlueprintPure, Category = "Character|Data")
	UNSCompanionDefinition* GetCurrentCompanionDefinition() const;
	
	UFUNCTION()
	UNSCompanionProgressionComponent* GetCompanionProgressionComponent() const {return CompanionProgressionComponent;}
	
public:
	// 플레이어의 진행 투표 확인용 (기본값: 거점 복귀)
	UPROPERTY(Replicated, BlueprintReadOnly, Category="RunEnd")
	ENSRunChoice RunChoice = ENSRunChoice::ReturnToHub; 
	// 투표 후 확인 버튼을 눌렀는지 확인용
	UPROPERTY(Replicated, BlueprintReadOnly, Category="RunEnd")
	bool bVoteConfirmed = false; 

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSPlayerAttributeSet> PlayerAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NS|CombatStat")
	TObjectPtr<UNSCombatStatComponent> CombatStatComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	TObjectPtr<UNSPlayerProgressComponent> ProgressComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parts")
	TObjectPtr<UNSPartEquipComponent> PartEquipComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Augment")
	TObjectPtr<UNSAugmentInventoryComponent> AugmentInventory;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Currency")
	TObjectPtr<UNSCurrencyComponent> CurrencyComponent;
	
	// @민재 : Companion업그레이드 관련 Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	TObjectPtr<UNSCompanionProgressionComponent> CompanionProgressionComponent;
	
	// 기본 캐릭터 데이터 ID : 기본적으로 Ranger Data를 쓰고 있음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Data")
	FPrimaryAssetId DefaultCharacterDataId;
	
	// @민재 기본 Companion 데이터 ID : 기본 스캔 드론
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Data")
	FGameplayTag DefaultCompanionDefinitionTag;
	
private:
	// AssetManager를 통해 ID로 DataAsset을 로드
	UNSCharacterData* LoadCharacterData(FPrimaryAssetId CharacterDataId) const;

	// @민재 : AssetManager 방식 동일
	UNSCompanionDefinition* LoadCompanionDefinition(FGameplayTag CompanionTag) const;
	
	UPROPERTY(Replicated)
	bool bIsReady;

	// 사망 상태 변수
	UPROPERTY(Replicated)
	bool bIsDead;
	
	// 현재 캐릭터 데이터 ID
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Character|Data", meta = (AllowPrivateAccess = "true"))
	FPrimaryAssetId CurrentCharacterDataId;
	
	// @민재 : companion 데이터 ID
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Character|Data", meta = (AllowPrivateAccess = "true"))
	FGameplayTag CurrentCompanionDefinitionTag;
};
