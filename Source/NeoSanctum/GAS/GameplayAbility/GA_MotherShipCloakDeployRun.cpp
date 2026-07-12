// Copyright 2026 One Team. All rights reserved.


#include "GA_MotherShipCloakDeployRun.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/AI/Components/NSFlyingLocomotionComponent.h"
#include "NeoSanctum/AI/Enemy/HelperActor/NSBossArenaBounds.h"
#include "NeoSanctum/Character/Enemy/NSBossMotherShip.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "DrawDebugHelpers.h"

UGA_MotherShipCloakDeployRun::UGA_MotherShipCloakDeployRun()
{
	FGameplayTagContainer CloakDeployRunAbilityTags = GetAssetTags();
	CloakDeployRunAbilityTags.AddTag(NSGameplayTags::Ability_Enemy_MotherShip_CloakDeployRun);
	SetAssetTags(CloakDeployRunAbilityTags);
	
	ActivationOwnedTags.AddTag(NSGameplayTags::State_Enemy_Combat);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}

void UGA_MotherShipCloakDeployRun::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	UGameplayAbility::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	CachedArenaBounds = GetArenaBounds();
	
	if (!ValidateAttackContext())
	{
		CancelAttackAbility();
		return;
	}
	
	if (ANSBossMotherShip* Boss = GetBossMotherShip())
	{
		CapturedAltitude = Boss->GetFlyingLocomotion()->GetAltitude();
	}
	
	EnterCloak();
	BeginLeg(ECloakDeployLeg::CloakAscend);
}

void UGA_MotherShipCloakDeployRun::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	for (FTimerHandle& DeployHandle : DeployWaveTimerHandles)
	{
		GetWorld()->GetTimerManager().ClearTimer(DeployHandle);
	}
	
	DeployWaveTimerHandles.Empty();
	GetWorld()->GetTimerManager().ClearTimer(LegPollTimerHandle);
	
	if (CurrentLegMontageTask.IsValid())
	{
		CurrentLegMontageTask->EndTask();
		CurrentLegMontageTask = nullptr;
	}
	
	if (ANSBossMotherShip* Boss = GetBossMotherShip())
	{
		if (UNSFlyingLocomotionComponent* Flying = Boss->GetFlyingLocomotion())
		{
			Flying->EndScriptedMove();
		}
	}
	
	ExitCloak();

	CachedArenaBounds = nullptr;
	CapturedAltitude = 0.f;
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_MotherShipCloakDeployRun::HandleAttackMontageCompleted()
{
	// 의도적 비움 
}

void UGA_MotherShipCloakDeployRun::BeginLeg(ECloakDeployLeg NewLeg)
{
	ANSBossMotherShip* Boss = GetBossMotherShip();
	if (!Boss)
	{
		CancelAttackAbility();
		return;
	}
	
	UNSFlyingLocomotionComponent* FlyingLocomotionComponent = Boss->GetFlyingLocomotion();
	if (!FlyingLocomotionComponent)
	{
		CancelAttackAbility();
		return;
	}
	
	if (!CachedArenaBounds.IsValid())
	{
		CancelAttackAbility();
		return;
	}
	
	CurrentLeg = NewLeg;

	switch (NewLeg)
	{
	case ECloakDeployLeg::CloakAscend :
		{
			const FVector Dest = CachedArenaBounds->GetDiagonalStartCorner(RunAltitude);
			PlayLegMontage(AscendDescendMontage);
			FlyingLocomotionComponent->BeginScriptedMove(
				Dest,
				RunAltitude,
				AscendSpeed);
			GetWorld()->GetTimerManager().SetTimer(
				LegPollTimerHandle,
				this,
				&ThisClass::PollLegProgress,
				LegPollInterval,
				true);
			if (bDrawDebug)
			{
				DrawDebugSphere(GetWorld(), Dest, 100.f, 16, FColor::Cyan, false, DrawTime);
			}
			break;
		}
	case ECloakDeployLeg::TraverseAndDeploy :
		{
			const FVector Dest = CachedArenaBounds->GetDiagonalEndCorner(RunAltitude);
			bAllWavesDeployed = false;
			PlayLegMontage(TraverseMontage);
			FlyingLocomotionComponent->BeginScriptedMove(
				Dest,
				RunAltitude, 
				RunSpeed);
			ScheduleDeployWaves();
		}
		break;
	case ECloakDeployLeg::DescendUncloak :
		{
			const FVector Dest = CachedArenaBounds->GetDiagonalEndCorner(CapturedAltitude);
			PlayLegMontage(AscendDescendMontage);
			FlyingLocomotionComponent->BeginScriptedMove(
				Dest,
				CapturedAltitude, 
				DescendSpeed);
		}
		break;
	}
}

void UGA_MotherShipCloakDeployRun::PollLegProgress()
{
	ANSBossMotherShip* Boss = GetBossMotherShip();
	if (!Boss)
	{
		CancelAttackAbility();
		return;
	}
	
	UNSFlyingLocomotionComponent* FlyingLocomotionComponent = Boss->GetFlyingLocomotion();
	if (!FlyingLocomotionComponent)
	{
		CancelAttackAbility();
		return;
	}
	
	if (!FlyingLocomotionComponent->HasReachedScriptedDest())
	{
		return;
	}
	
	switch (CurrentLeg)
	{
	case ECloakDeployLeg::CloakAscend :
		BeginLeg(ECloakDeployLeg::TraverseAndDeploy);
		break;
	case ECloakDeployLeg::TraverseAndDeploy :
		if (bAllWavesDeployed)
		{
			BeginLeg(ECloakDeployLeg::DescendUncloak);
		}
		break;
	case ECloakDeployLeg::DescendUncloak :
		ExitCloak();
		FinishAttackAbility();
		break;
	}
}

void UGA_MotherShipCloakDeployRun::ScheduleDeployWaves()
{
	const int32 N = GetWaveCount();
	if (N <= 0) return;

	DeployWaveTimerHandles.SetNum(N);

	for (int32 i = 0; i < N; ++i)
	{
		const float Delay = i * SpawnInterval;

		if (Delay <= 0.f)
		{
			DeployWave(i);
			continue;
		}

		GetWorld()->GetTimerManager().SetTimer(
			DeployWaveTimerHandles[i],
			FTimerDelegate::CreateUObject(this, &ThisClass::DeployWave, i),
			Delay,
			false);
	}
}

void UGA_MotherShipCloakDeployRun::DeployWave(int32 WaveIndex)
{
	ANSBossMotherShip* Boss = GetBossMotherShip();
	if (!Boss)
	{
		return;
	}
	
	if (!Boss->HasAuthority() || !CachedArenaBounds.IsValid())
	{
		return;
	}
	
	AGameModeBase* GM = GetWorld()->GetAuthGameMode();
	if (!GM)
	{
		return;
	}
	
	if (!GM->Implements<UNSRunGameModeInterface>())
	{
		return;
	}
	
	TArray<FVector> PlacedLocationsThisWave;
	const FVector BaseLocation = Boss->GetActorLocation();
	
	for (const FNSAirDropSpawnEntry& Entry : SpawnEntries)
	{
		if (!Entry.CharacterClass || !Entry.EnemyData)
		{
			continue;
		}

		for (int32 c = 0; c < Entry.CountPerWave; ++c)
		{
			const FVector SpawnLocation =
				ComputeScatteredSpawnLocation(BaseLocation, PlacedLocationsThisWave);
			PlacedLocationsThisWave.Add(SpawnLocation);

			if (bDrawDebug)
			{
				DrawDebugSphere(GetWorld(), SpawnLocation, 50.f, 12, FColor::Red, false, DrawTime);
			}
			
			INSRunGameModeInterface::Execute_RequestSpawnMonster(
				GM,
				Entry.CharacterClass,
				Entry.EnemyData,
				SpawnLocation,
				Boss->GetActorRotation());
		}
	}
	
	if (WaveIndex == GetWaveCount() - 1)
	{
		bAllWavesDeployed = true;
	}
}

FVector UGA_MotherShipCloakDeployRun::ComputeScatteredSpawnLocation(const FVector& BaseLocation,
	const TArray<FVector>& PlacedLocationsThisWave) const
{
	const int32 MaxAttempts = 5;
	const float MinSeparation = SpawnScatterRadius * 0.5f;

	FVector Candidate = BaseLocation;

	for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
	{
		const float Angle  = FMath::FRandRange(0.f, 2.f * PI);
		const float Radius = FMath::Sqrt(FMath::FRand()) * SpawnScatterRadius;
		Candidate = BaseLocation + FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.f);

		bool bTooClose = false;
		for (const FVector& Placed : PlacedLocationsThisWave)
		{
			if (FVector::DistSquared2D(Candidate, Placed) < FMath::Square(MinSeparation))
			{
				bTooClose = true;
				break;
			}
		}

		if (!bTooClose)
		{
			return Candidate;   // 안 겹치는 자리 찾음
		}
	}

	return Candidate; 
}

void UGA_MotherShipCloakDeployRun::PlayLegMontage(UAnimMontage* Montage)
{
	if (CurrentLegMontageTask.IsValid())
	{
		CurrentLegMontageTask->EndTask();
		CurrentLegMontageTask = nullptr;
	}
	
	if (!Montage)
	{
		return;
	}
	
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, 
		TEXT("CloakDeployLegMontage"),
		Montage, 
		1.f,
		NAME_None,
		false,
		1.f, 
		0.f);
	
	if (!MontageTask)
	{
		return;
	}
	
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnLegMontageEnded);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnLegMontageEnded);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnLegMontageEnded);
	
	MontageTask->ReadyForActivation();
	CurrentLegMontageTask = MontageTask;
}

void UGA_MotherShipCloakDeployRun::OnLegMontageEnded()
{
	CurrentLegMontageTask = nullptr;
}

void UGA_MotherShipCloakDeployRun::EnterCloak()
{
	if (bCloaked)
	{
		return;
	}
	
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}
	
	SourceASC->AddLooseGameplayTag(NSGameplayTags::State_Invincible);
	SourceASC->AddLooseGameplayTag(NSGameplayTags::State_Enemy_MotherShip_Stealth);
	
	if (CloakCueTag.IsValid())
	{
		SourceASC->AddGameplayCue(CloakCueTag);
	}
	
	bCloaked = true;
}

void UGA_MotherShipCloakDeployRun::ExitCloak()
{
	if (!bCloaked)
	{
		return;
	}
	
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}
	
	SourceASC->RemoveLooseGameplayTag(NSGameplayTags::State_Invincible);
	SourceASC->RemoveLooseGameplayTag(NSGameplayTags::State_Enemy_MotherShip_Stealth);
	
	if (CloakCueTag.IsValid())
	{
		SourceASC->RemoveGameplayCue(CloakCueTag);
	}
	
	bCloaked = false;
}

ANSBossMotherShip* UGA_MotherShipCloakDeployRun::GetBossMotherShip() const
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn)
	{
		return nullptr;
	}
	
	return Cast<ANSBossMotherShip>(Pawn);
}

ANSBossArenaBounds* UGA_MotherShipCloakDeployRun::GetArenaBounds() const
{
	ANSBossMotherShip* BossMotherShip = GetBossMotherShip();
	if (!BossMotherShip) return nullptr;
	
	return BossMotherShip->GetArenaBounds();
}

bool UGA_MotherShipCloakDeployRun::ValidateAttackContext() const
{
	ANSBossMotherShip* MotherShip = GetBossMotherShip();
	if (!CachedArenaBounds.Get() || !MotherShip || !MotherShip->GetFlyingLocomotion())
	{
		return false;
	}
	
	if (SpawnEntries.IsEmpty())
	{
		return false;
	}
	
	return true;
}

int32 UGA_MotherShipCloakDeployRun::GetWaveCount() const
{
	return FMath::Max(FMath::FloorToInt(TraverseDuration / FMath::Max(SpawnInterval, KINDA_SMALL_NUMBER)), 0);
}
