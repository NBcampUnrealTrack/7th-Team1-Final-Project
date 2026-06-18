// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerState.h"

#include "NSPlayerProgressComponent.h"
#include "NeoSanctum/Progression/Part/NSPartEquipComponent.h"
#include "Engine/AssetManager.h"
#include "Net/UnrealNetwork.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/Progression/Augment/NSAugmentInventoryComponent.h"
#include "NeoSanctum/Progression/Currency/NSCurrencyComponent.h"
#include "NeoSanctum/System/Subsystem/NSCurrencyDropSubsystem.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/GAS/Stats/NSCombatStatComponent.h"

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
}

void ANSPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (ANSPlayerState* NewPlayerState = Cast<ANSPlayerState>(PlayerState))
	{
		NewPlayerState->CurrentCharacterDataId = CurrentCharacterDataId;
		
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
	bIsReady = bNewReady;
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
