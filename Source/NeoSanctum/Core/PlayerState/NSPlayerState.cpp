// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerState.h"

#include "NSPlayerProgressComponent.h"
#include "NeoSanctum/Progression/Part/NSPartEquipComponent.h"
#include "Engine/AssetManager.h"
#include "NeoSanctum/Character/Component/NSCompanionProgressionComponent.h"
#include "NeoSanctum/Data/AI/NSCompanionCatalog.h"
#include "Net/UnrealNetwork.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/Progression/Augment/NSAugmentInventoryComponent.h"
#include "NeoSanctum/Progression/Currency/NSCurrencyComponent.h"
#include "NeoSanctum/System/Subsystem/NSCurrencyDropSubsystem.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/GAS/Stats/NSCombatStatComponent.h"
#include "NeoSanctum/Data/AI/NSCompanionDefinition.h"
#include "NeoSanctum/Core/GameState/NSOutGameState.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"

ANSPlayerState::ANSPlayerState()
{
	// PlayerState의 기본 Frequency는 1Hz(매우 낮음)
	SetNetUpdateFrequency(100.0f);

	AbilitySystemComponent = CreateDefaultSubobject<UNSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	PlayerAttributeSet = CreateDefaultSubobject<UNSPlayerAttributeSet>(TEXT("PlayerAttributeSet"));

	// 스킬 전투 스탯 DataTable을 캐싱하고 조회하는 컴포넌트
	CombatStatComponent = CreateDefaultSubobject<UNSCombatStatComponent>(TEXT("CombatStatComponent"));
	
	// 진행도 저장,로드 컴포넌트
	ProgressComponent = CreateDefaultSubobject<UNSPlayerProgressComponent>(TEXT("ProgressComponent"));

	// @민재 : Companion 진행도
	CompanionProgressionComponent = CreateDefaultSubobject<UNSCompanionProgressionComponent>(
		TEXT("CompanionProgressionComponent"));
	
	PartEquipComponent = CreateDefaultSubobject<UNSPartEquipComponent>(TEXT("PartEquipComponent"));

	// 인런 증강 보유 컴포넌트 (인런 종료 시 RunGameMode가 Clear)
	AugmentInventory = CreateDefaultSubobject<UNSAugmentInventoryComponent>(TEXT("AugmentInventory"));
	CurrencyComponent = CreateDefaultSubobject<UNSCurrencyComponent>(TEXT("CurrencyComponent"));
	
	bIsReady = false;
	bIsDead = false;
}

void ANSPlayerState::BeginPlay()
{
	Super::BeginPlay();
	
}

void ANSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANSPlayerState, bIsReady);
	DOREPLIFETIME(ANSPlayerState, bIsDead);
	DOREPLIFETIME(ANSPlayerState, RunChoice);
	DOREPLIFETIME(ANSPlayerState, bVoteConfirmed);
	DOREPLIFETIME(ANSPlayerState, CurrentCharacterDataId);
	DOREPLIFETIME(ANSPlayerState, CurrentCompanionDefinitionTag);
}

void ANSPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (ANSPlayerState* NewPlayerState = Cast<ANSPlayerState>(PlayerState))
	{
		NewPlayerState->CurrentCharacterDataId = CurrentCharacterDataId;
		NewPlayerState->CurrentCompanionDefinitionTag = CurrentCompanionDefinitionTag;
		
		UNSPlayerProgressComponent* OldProgress = ProgressComponent;
		UNSPlayerProgressComponent* NewProgress = NewPlayerState->GetProgressComponent();
		if (OldProgress && NewProgress)
		{
			FNSProgressPayload Payload;
			OldProgress->BuildPayload(Payload);
			NewProgress->ApplyPayload(Payload);
		}
		
		if (UNSCurrencyComponent* NewCurrency = NewPlayerState->GetCurrencyComponent())
		{
			NewCurrency->CopyRunStateFrom(CurrencyComponent);
		}

		if (UNSAugmentInventoryComponent* NewAugment = NewPlayerState->GetAugmentInventory())
		{
			NewAugment->CopyRunStateFrom(AugmentInventory);
		}
	}
}

UAbilitySystemComponent* ANSPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UNSPlayerAttributeSet* ANSPlayerState::GetPlayerAttributeSet() const
{
	return PlayerAttributeSet;
}

UNSCombatStatComponent* ANSPlayerState::GetCombatStatComponent() const
{
	return CombatStatComponent;
}

void ANSPlayerState::SetReady(bool bNewReady)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bIsReady == bNewReady)
	{
		return;
	}

	bIsReady = bNewReady;

	// 서버/호스트는 OnRep가 호출되지 않으므로 직접 알림
	NotifyReadyStateChanged();

	ForceNetUpdate();
}

void ANSPlayerState::Server_CollectCurrency_Implementation(int32 DropId)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UNSCurrencyDropSubsystem* DropSys = World->GetSubsystem<UNSCurrencyDropSubsystem>())
	{
		DropSys->TryCollect(DropId, this);
	}

}

void ANSPlayerState::SetIsDead(bool bNewIsDead)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsDead = bNewIsDead;
}

void ANSPlayerState::SetCurrentCharacterData(UNSCharacterData* InCharacterData)
{
	SetCurrentCharacterDataId(InCharacterData ? InCharacterData->GetPrimaryAssetId() : FPrimaryAssetId());
}

void ANSPlayerState::SetCurrentCharacterDataId(FPrimaryAssetId InCharacterDataId)
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentCharacterDataId = InCharacterDataId;
	
	ForceNetUpdate();
}

UNSCharacterData* ANSPlayerState::GetCurrentCharacterData() const
{
	if (CurrentCharacterDataId.IsValid())
	{
		if (UNSCharacterData* CurrentCharacterData = LoadCharacterData(CurrentCharacterDataId))
		{
			return CurrentCharacterData;
		}
	}

	return LoadCharacterData(DefaultCharacterDataId);
}

UNSCharacterData* ANSPlayerState::LoadCharacterData(FPrimaryAssetId CharacterDataId) const
{
	if (!CharacterDataId.IsValid())
	{
		return nullptr;
	}

	const FSoftObjectPath CharacterDataPath = UAssetManager::Get().GetPrimaryAssetPath(CharacterDataId);
	if (!CharacterDataPath.IsValid())
	{
		return nullptr;
	}

	return Cast<UNSCharacterData>(CharacterDataPath.TryLoad());
}

#pragma region Companion

void ANSPlayerState::SetCurrentCompanionDefinitionTag(FGameplayTag CompanionTag)
{
	if (!HasAuthority()) return;
	
	CurrentCompanionDefinitionTag = CompanionTag;
	
	ForceNetUpdate();
}

UNSCompanionDefinition* ANSPlayerState::GetCurrentCompanionDefinition() const
{
	if (CurrentCompanionDefinitionTag.IsValid())
	{
		if (UNSCompanionDefinition* CurrentCompanionData = LoadCompanionDefinition(CurrentCompanionDefinitionTag))
		{
			return CurrentCompanionData;
		}
	}
	// 기본값 폴백
	return LoadCompanionDefinition(DefaultCompanionDefinitionTag);
}

UNSCompanionDefinition* ANSPlayerState::LoadCompanionDefinition(FGameplayTag CompanionTag) const
{
	if (!CompanionTag.IsValid())
	{
		return nullptr;
	}
	
	if (!IsValid(CompanionProgressionComponent)) return nullptr;
	
	if (!CompanionProgressionComponent->Catalog) return nullptr;
	
	UNSCompanionDefinition* CompanionDefinition = CompanionProgressionComponent->Catalog->FindByTag(CompanionTag);
	if (!CompanionDefinition) return nullptr;
	
	return CompanionDefinition;
}

#pragma endregion

void ANSPlayerState::OnRep_bIsReady()
{
	NotifyReadyStateChanged();
}

void ANSPlayerState::NotifyReadyStateChanged() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ANSOutGameState* OutGameState =
		World->GetGameState<ANSOutGameState>();

	if (!OutGameState)
	{
		return;
	}

	OutGameState->NotifyReadyStateChanged();
}

void ANSPlayerState::OnRep_RunEndVoteState()
{
	NotifyRunEndVoteChanged();
}

void ANSPlayerState::NotifyRunEndVoteChanged() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	ANSRunGameState* RunGameState =
		World->GetGameState<ANSRunGameState>();
	
	if (!RunGameState)
	{
		return;
	}
	
	RunGameState->NotifyRunVoteChanged();
}
