// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerState.h"

#include "NSPlayerProgressComponent.h"
#include "Net/UnrealNetwork.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/Progression/Augment/NSAugmentInventoryComponent.h"
#include "NeoSanctum/Progression/Save/NSPermanentSaveGame.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/GAS/Stats/NSCombatStatComponent.h"
#include "NeoSanctum/System/NSSaveGameSubsystem.h"

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

	// 인런 증강 보유 컴포넌트 (인런 종료 시 RunGameMode가 Clear)
	AugmentInventory = CreateDefaultSubobject<UNSAugmentInventoryComponent>(TEXT("AugmentInventory"));

	bIsReady = false;
	bIsDead = false;
}

void ANSPlayerState::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 세이브 데이터 초기화. 복제 프로퍼티로 클라이언트에 전달됨
	if (!HasAuthority()) return;

	UNSSaveGameSubsystem* SaveSys = GetWorld()->GetGameInstance()->GetSubsystem<UNSSaveGameSubsystem>();
	if (!SaveSys)
	{
		return;
	}

	// 캐시 데이터가 있으면 바로 적용, 없으면 콜백으로 받음
	if (SaveSys->GetCachedPermanentData())
	{
		ProgressComponent->InitFromSaveData(SaveSys->GetCachedPermanentData());
	}
	else
	{
		SaveSys->OnPermanentDataLoaded.AddUObject(this, &ANSPlayerState::OnSaveDataLoaded);
	}
}

void ANSPlayerState::OnSaveDataLoaded(UNSPermanentSaveGame* Data)
{
	ProgressComponent->InitFromSaveData(Data);
}

void ANSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANSPlayerState, bIsReady);
	DOREPLIFETIME(ANSPlayerState, bIsDead);
	DOREPLIFETIME(ANSPlayerState, RunChoice);
	DOREPLIFETIME(ANSPlayerState, bVoteConfirmed);
	DOREPLIFETIME(ANSPlayerState, CurrentCharacterData);
}

void ANSPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (ANSPlayerState* NewPlayerState = Cast<ANSPlayerState>(PlayerState))
	{
		// SeamlessTravel 이후에도 유지되어야 할 캐릭터 데이터
		NewPlayerState->CurrentCharacterData = CurrentCharacterData;
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

void ANSPlayerState::Server_SetReady_Implementation()
{
	bIsReady = !bIsReady;
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
	if (!HasAuthority())
	{
		return;
	}

	CurrentCharacterData = InCharacterData;
	
	ForceNetUpdate();
}

UNSCharacterData* ANSPlayerState::GetCurrentCharacterData() const
{
	if (!CurrentCharacterData.IsNull())
	{
		return CurrentCharacterData.LoadSynchronous();
	}

	return DefaultCharacterData.LoadSynchronous();
}
