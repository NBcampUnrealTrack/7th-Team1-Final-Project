// Copyright 2026 One Team. All rights reserved.


#include "NSExperienceComponent.h"

#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "Net/UnrealNetwork.h"

UNSExperienceComponent::UNSExperienceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNSExperienceComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UNSExperienceComponent, CurrentExp, COND_OwnerOnly);
}

int32 UNSExperienceComponent::AddExperience(float BaseAmount)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || BaseAmount <= 0.0f)
	{
		return 0;
	}

	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem)
	{
		return 0;
	}

	const float MaxExp = DataSubsystem->GetMaxExperience();
	const float Multiplier = GetExpGainMultiplier();

	CurrentExp += BaseAmount * Multiplier;

	// 한 번의 지급으로 통을 여러 번 넘을 수 있으므로 넘친 만큼 전부 소비
	int32 LevelUpCount = 0;
	while (CurrentExp >= MaxExp)
	{
		CurrentExp -= MaxExp;
		++LevelUpCount;
	}

	NS_ACTOR_LOG(Owner, LogNS, Log,
		"경험치를 지급했습니다. BaseAmount={BaseAmount}, Multiplier={Multiplier}, CurrentExp={CurrentExp}, MaxExp={MaxExp}, LevelUpCount={LevelUpCount}",
		("BaseAmount", BaseAmount),
		("Multiplier", Multiplier),
		("CurrentExp", CurrentExp),
		("MaxExp", MaxExp),
		("LevelUpCount", LevelUpCount)
	);

	OnExpChanged.Broadcast(CurrentExp, MaxExp);

	return LevelUpCount;
}

float UNSExperienceComponent::GetExpGainMultiplier() const
{
	// TODO(원종): OutRun 공통 업그레이드 시스템 도입 시 ProgressComponent에서 계정 배율을 읽도록 교체.
	return 1.0f;
}

void UNSExperienceComponent::OnRep_CurrentExp(float OldCurrentExp)
{
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	const float MaxExp = DataSubsystem ? DataSubsystem->GetMaxExperience() : 100.0f;

	OnExpChanged.Broadcast(CurrentExp, MaxExp);
}
