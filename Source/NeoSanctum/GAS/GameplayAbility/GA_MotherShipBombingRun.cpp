// Copyright 2026 One Team. All rights reserved.


#include "GA_MotherShipBombingRun.h"

#include "AbilitySystemComponent.h"
#include "NeoSanctum/AI/Components/NSFlyingLocomotionComponent.h"
#include "NeoSanctum/AI/Enemy/HelperActor/NSBossArenaBounds.h"
#include "NeoSanctum/Character/Enemy/NSBossMotherShip.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/AI/Enemy/HelperActor/NSBombMissile.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"
#include "NeoSanctum/Combat/NSDamageRules.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_MotherShipBombingRun::UGA_MotherShipBombingRun()
{
	FGameplayTagContainer BombingRunAbilityTags = GetAssetTags();
	BombingRunAbilityTags.AddTag(NSGameplayTags::Ability_Enemy_MotherShip_BombingRun);
	SetAssetTags(BombingRunAbilityTags);
	
	ActivationOwnedTags.AddTag(NSGameplayTags::State_Enemy_Combat);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}

void UGA_MotherShipBombingRun::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	UGameplayAbility::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true,true);
		return;
	}
	
	CachedAttackRow = GetCurrentAttackRow();
	CachedArenaBounds = GetArenaBounds();
	
	if (!ValidateAttackContext())
	{
		CancelAttackAbility();
		return;
	}
	
	// ★ 추가: 이 패턴 동안 지형 관통 허용
	if (ANSBossMotherShip* Boss = GetBossMotherShip())
	{
		Boss->SetTerrainCollisionIgnored(true);
	}

	
	BeginLeg(EBombingRunLeg::Ascend);
}

void UGA_MotherShipBombingRun::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	ClearAllZoneTimers();
	GetWorld()->GetTimerManager().ClearTimer(LegPollTimerHandle);
	ClearAllZoneWarningCues();
	
	for (TWeakObjectPtr<ANSBombMissile>& Missile : ActiveMissiles)
	{
		if (Missile.IsValid())
		{
			Missile->Destroy();
		}
	}
	ActiveMissiles.Empty();
	
	if (ANSBossMotherShip* Boss = GetBossMotherShip())
	{
		if (UNSFlyingLocomotionComponent* FlyingLocomotionComponent =Boss->GetFlyingLocomotion())
		{
			FlyingLocomotionComponent->EndScriptedMove();
		}
		Boss->SetTerrainCollisionIgnored(false);
	}
	CachedAttackRow = nullptr;
	CachedArenaBounds = nullptr;
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_MotherShipBombingRun::HandleAttackMontageCompleted()
{
	// 의도적 비움
}

void UGA_MotherShipBombingRun::BeginLeg(EBombingRunLeg NewLeg)
{
	ANSBossMotherShip* Boss = GetBossMotherShip();
	if (!Boss)
		{
		CancelAttackAbility();
		return;
	}
	
	UNSFlyingLocomotionComponent* FlyingLocomotionComponent =Boss->GetFlyingLocomotion();
	if (!FlyingLocomotionComponent)
		{
		CancelAttackAbility();
		return;
	}
	
	if (!CachedArenaBounds.Get())
	{
		CancelAttackAbility();
		return;
	}
	
	ANSBossArenaBounds* Arena = CachedArenaBounds.Get();
	
	CurrentLeg = NewLeg;
	switch (NewLeg) {
	case EBombingRunLeg::Ascend :
		FlyingLocomotionComponent->BeginScriptedMove(
			Arena->GetFarEndCenter(RunAltitude), RunAltitude, AscendSpeed);
		GetWorld()->GetTimerManager().SetTimer(
			LegPollTimerHandle, 
			this,
			&ThisClass::PollLegProgress,
			LegPollInterval,
			true);
		break;
	case EBombingRunLeg::Traverse :
		FlyingLocomotionComponent->BeginScriptedMove(
			Arena->GetEntranceCenter(RunAltitude), RunAltitude, RunSpeed);
		break;
	case EBombingRunLeg::HoldAtEntrance :
		GetWorld()->GetTimerManager().ClearTimer(LegPollTimerHandle);
		break;
	case EBombingRunLeg::Return :
		FlyingLocomotionComponent->BeginScriptedMove(
		Arena->GetArenaCenter(CapturedAltitude), CapturedAltitude, ReturnSpeed);
		GetWorld()->GetTimerManager().SetTimer(
			LegPollTimerHandle, 
			this,
			&ThisClass::PollLegProgress,
			LegPollInterval,
			true);
		break;
	}
}

void UGA_MotherShipBombingRun::PollLegProgress()
{
	ANSBossMotherShip* Boss = GetBossMotherShip();
	if (!Boss) { CancelAttackAbility(); return; }

	UNSFlyingLocomotionComponent* FlyingLocomotionComponent = Boss->GetFlyingLocomotion();
	if (!FlyingLocomotionComponent) { CancelAttackAbility(); return; }

	const bool bReached = FlyingLocomotionComponent->HasReachedScriptedDest();
	if (!bReached) return;

	switch (CurrentLeg) {
	case EBombingRunLeg::Ascend : BeginLeg(EBombingRunLeg::Traverse); break;
	case EBombingRunLeg::Traverse :
		BeginLeg(EBombingRunLeg::HoldAtEntrance);
		StartWarningPhase();
		break;
	case EBombingRunLeg::Return : FinishAttackAbility(); break;
	}

}

void UGA_MotherShipBombingRun::StartWarningPhase()
{
	const int32 N = CachedAttackRow->BombardData.ShotCount;
	const float Interval = CachedAttackRow->BombardData.ShotInterval;
	const float Delay = CachedAttackRow->BombardData.ImpactDelay;

	if (N <= 0) return;

	ZoneWarningTimerHandles.SetNum(N);

	ActiveMissiles.SetNum(N * SlotsPerZone);
	
	for (int32 i = 0; i < N; ++i)
	{
		const float FireDelay = FMath::Max(i * Interval, KINDA_SMALL_NUMBER);
		GetWorld()->GetTimerManager().SetTimer(ZoneWarningTimerHandles[i],
			FTimerDelegate::CreateUObject(this, &ThisClass::ShowZoneWarning, i),
			FireDelay,   // ← i=0 도 다음 틱에 확실히 실행
			false);
	}

	GetWorld()->GetTimerManager().SetTimer(DetonationPhaseTimerHandle,
		this,
		&ThisClass::StartDetonationPhase,
		(N - 1) * Interval + Delay,
		false);
}

void UGA_MotherShipBombingRun::StartDetonationPhase()
{
	const int32 N = CachedAttackRow->BombardData.ShotCount;
	const float Interval = CachedAttackRow->BombardData.ShotInterval;

	if (N <= 0) return;

	ZoneImpactTimerHandles.SetNum(N);

	for (int32 i = 0; i < N; ++i)
	{
		const float FireDelay = FMath::Max(i * Interval, KINDA_SMALL_NUMBER);
		GetWorld()->GetTimerManager().SetTimer(ZoneImpactTimerHandles[i],
			FTimerDelegate::CreateUObject(this, &ThisClass::DetonateZone, i),
			FireDelay,
			false);
	}
}

void UGA_MotherShipBombingRun::ShowZoneWarning(int32 ZoneIndex)
{
	const int32 N = CachedAttackRow->BombardData.ShotCount;
	if (N <= 0) return;

	const FVector ZoneCenter = CachedArenaBounds->GetZoneCenter(ZoneIndex, N);

	FGameplayCueParameters Parameters;
	Parameters.RawMagnitude = static_cast<float>(ZoneIndex);
	Parameters.Location = ZoneCenter;
	Parameters.Normal = CachedArenaBounds->GetZoneBoxRotation().GetForwardVector().GetSafeNormal2D();

	Parameters.SourceObject = CachedArenaBounds.Get();
	Parameters.NormalizedMagnitude = static_cast<float>(N);

	UAbilitySystemComponent* BossASC = GetAbilitySystemComponentFromActorInfo();
	if (!BossASC) return;
	BossASC->AddGameplayCue(WarningCueTags[ZoneIndex], Parameters);
	ActiveWarningZoneIndices.Add(ZoneIndex);
	DrawDebugZone(ZoneIndex);

	FGameplayCueParameters DropParameters;
	DropParameters.Location = ZoneCenter;
	BossASC->ExecuteGameplayCue(MissileDropCueTag, DropParameters);

	ANSBossArenaBounds* Arena = CachedArenaBounds.Get();
	UWorld* World = GetWorld();
	if (!IsValid(Arena) || !World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetAvatarActorFromActorInfo();

	AActor* BossActor = BossASC->GetAvatarActor();
	if (!BossActor) return;
	
	ANSBossMotherShip* BossMotherShip = Cast<ANSBossMotherShip>(BossActor);
	if (!BossMotherShip) return;
	
	for (int32 s = 0; s < SlotsPerZone; ++s)
	{
		const FVector SlotPos = Arena->GetSlotCenter(ZoneIndex, s, N, SlotsPerZone);
		const FVector SpawnLocation(SlotPos.X, SlotPos.Y, BossMotherShip->GetActorLocation().Z);

		ANSBombMissile* Missile = World->SpawnActor<ANSBombMissile>(
			BombMissileClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

		if (Missile)
		{
			Missile->InitDrop(SlotPos);
		}

		ActiveMissiles[ZoneIndex * SlotsPerZone + s] = Missile;
	}
}

void UGA_MotherShipBombingRun::DetonateZone(int32 ZoneIndex)
{
	const int32 N = CachedAttackRow->BombardData.ShotCount;

	UAbilitySystemComponent* BossASC = GetAbilitySystemComponentFromActorInfo();
	if (!BossASC) return;

	ANSBossArenaBounds* Arena = CachedArenaBounds.Get();
	if (!IsValid(Arena)) return;

	FGameplayCueParameters ExplosionSoundParameters;
	ExplosionSoundParameters.Location = Arena->GetZoneCenter(ZoneIndex, N);
	BossASC->ExecuteGameplayCue(ExplosionSoundCueTag, ExplosionSoundParameters);

	const float Damage = CalculateBombardDamage(*CachedAttackRow);

	for (int32 s = 0; s < SlotsPerZone; ++s)
	{
		const int32 MissileIndex = ZoneIndex * SlotsPerZone + s;
		if (ActiveMissiles.IsValidIndex(MissileIndex) && ActiveMissiles[MissileIndex].IsValid())
		{
			ActiveMissiles[MissileIndex]->Detonate();
		}

		TArray<AActor*> Targets;
		Arena->OverlapPlayersInSlot(ZoneIndex, s, N, SlotsPerZone, ZoneHeight, Targets);

		for (AActor* Target : Targets)
		{
			FHitResult HitResult;
			HitResult.Location = Target->GetActorLocation();
			HitResult.ImpactPoint = Target->GetActorLocation();
			ApplyBombardDamageToTarget(Target, HitResult, Damage);
		}
	}

	BossASC->RemoveGameplayCue(WarningCueTags[ZoneIndex]);
	ActiveWarningZoneIndices.Remove(ZoneIndex);
	if (ZoneIndex == N - 1)
	{
		BeginLeg(EBombingRunLeg::Return);
	}
}

void UGA_MotherShipBombingRun::ClearAllZoneWarningCues()
{
	UAbilitySystemComponent* BossASC = GetAbilitySystemComponentFromActorInfo();
	if (!BossASC) return;

	for (int32 ZoneIndex : ActiveWarningZoneIndices)
	{
		BossASC->RemoveGameplayCue(WarningCueTags[ZoneIndex]);
	}
	ActiveWarningZoneIndices.Empty();
}

void UGA_MotherShipBombingRun::ClearAllZoneTimers()
{
	GetWorld()->GetTimerManager().ClearTimer(DetonationPhaseTimerHandle);

	for (FTimerHandle& WarningTimerHandle : ZoneWarningTimerHandles)
	{
		GetWorld()->GetTimerManager().ClearTimer(WarningTimerHandle);
	}

	for (FTimerHandle& ZoneTimerHandle : ZoneImpactTimerHandles)
	{
		GetWorld()->GetTimerManager().ClearTimer(ZoneTimerHandle);
	}

	ZoneWarningTimerHandles.Empty();
	ZoneImpactTimerHandles.Empty();
}

float UGA_MotherShipBombingRun::CalculateBombardDamage(const FNSEnemyAttackRow& AttackRow) const
{
	const UAbilitySystemComponent* BossASC =
		GetAbilitySystemComponentFromActorInfo();

	if (!BossASC)
	{
		return -1.0f;
	}

	const float SourceBaseDamage =
		BossASC->GetNumericAttribute(
			UNSBaseAttributeSet::GetBaseDamageAttribute());

	return FMath::Max(
		SourceBaseDamage * AttackRow.DamageScale,
		0.0f);
}

bool UGA_MotherShipBombingRun::ApplyBombardDamageToTarget(AActor* TargetActor, const FHitResult& HitResult,
	float Damage)
{
	AActor* SourceActor = GetAvatarActorFromActorInfo();

	const IAbilitySystemInterface* TargetASI =
		Cast<IAbilitySystemInterface>(TargetActor);

	UAbilitySystemComponent* SourceASC =
		GetAbilitySystemComponentFromActorInfo();

	UAbilitySystemComponent* TargetASC =
		TargetASI ? TargetASI->GetAbilitySystemComponent() : nullptr;

	if (!IsValid(SourceActor) ||
		!IsValid(SourceASC) ||
		!IsValid(TargetASC) ||
		!DamageEffectClass)
	{
		return false;
	}

	// 피아구분: 같은 Enemy 팀(일반몹/드론/보스 파츠)·사망·무적 대상은 데미지 제외.
	// HomingMissile과 동일하게 단일 규칙 NSDamageRules로 판정 (오버랩은 넓게 긁고 여기서 게이트).
	if (!NSDamageRules::CanApplyDamage(SourceActor, TargetActor))
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext =
		SourceASC->MakeEffectContext();

	EffectContext.AddSourceObject(SourceActor);
	EffectContext.AddHitResult(HitResult);
	
	FGameplayEffectSpecHandle SpecHandle =
		SourceASC->MakeOutgoingSpec(
			DamageEffectClass,
			1.0f,
			EffectContext);

	if (!SpecHandle.IsValid())
	{
		return false;
	}
	
	if (Damage >= 0.0f)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(
			NSGameplayTags::Effect_Damage_Base,
			Damage);
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(
		*SpecHandle.Data.Get(),
		TargetASC);

	return true;
}

ANSBossMotherShip* UGA_MotherShipBombingRun::GetBossMotherShip() const
{
	APawn* BossPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!BossPawn) return nullptr;
	
	return Cast<ANSBossMotherShip>(BossPawn);
}

ANSBossArenaBounds* UGA_MotherShipBombingRun::GetArenaBounds() const
{
	ANSBossMotherShip* Boss = GetBossMotherShip();
	return Boss ? Boss->GetArenaBounds() : nullptr;
}

const FNSEnemyAttackRow* UGA_MotherShipBombingRun::GetCurrentAttackRow() const
{
	ANSBossMotherShip* Boss = GetBossMotherShip();
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(Boss);

	return EnemyAgent
			   ? EnemyAgent->GetCurrentAttackRow()
			   : nullptr;
}

bool UGA_MotherShipBombingRun::ValidateAttackContext() const
{
	if (!CachedAttackRow || !CachedArenaBounds.Get()) return false;
	ANSBossMotherShip* Boss = GetBossMotherShip();
	if (!Boss) return false;
	
	if (!Boss->GetFlyingLocomotion()) return false;
	
	const int32 N = CachedAttackRow->BombardData.ShotCount;
	if (N <= 0 || WarningCueTags.Num() < N) return false;
	
	return true;
}

void UGA_MotherShipBombingRun::DrawDebugZone(int32 ZoneIndex) const
{
	if (!CachedAttackRow || !CachedAttackRow->DebugData.bDrawDebug) return;

	ANSBossArenaBounds* Arena = CachedArenaBounds.Get();
	if (!Arena) return;

	const int32 N = CachedAttackRow->BombardData.ShotCount;
	const FVector Center = Arena->GetZoneCenter(ZoneIndex, N);
	const FVector Extent = Arena->GetZoneBoxExtent(N, ZoneHeight);
	const FQuat Rotation = Arena->GetZoneBoxRotation();

	const FColor Color = ZoneColors.IsValidIndex(ZoneIndex)
		? ZoneColors[ZoneIndex].ToFColor(true)
		: FColor::White;

	DrawDebugBox(
		GetWorld(),
		Center,
		Extent,
		Rotation,
		Color,
		false,
		CachedAttackRow->DebugData.DrawTime,
		0,
		4.f);
}
