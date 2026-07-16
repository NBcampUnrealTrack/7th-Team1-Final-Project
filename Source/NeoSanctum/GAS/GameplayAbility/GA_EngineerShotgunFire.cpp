// Copyright 2026 One Team. All rights reserved.


#include "GA_EngineerShotgunFire.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Combat/NSDamageRules.h"
#include "NeoSanctum/Combat/Weapon/NSWeaponBase.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_EngineerShotgunFire::UGA_EngineerShotgunFire()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Engineer_ShotgunFire);
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(NSGameplayTags::State_Deactivate_HandIK);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Reloading);
	ActivationPolicy = ENSAbilityActivationPolicy::WhileInputActive;
	bRequestReloadOnEmptyAmmo = true;
}

void UGA_EngineerShotgunFire::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bFireCycleElapsed = false;
	bTargetDataProcessed = false;

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	float FinalFireInterval = 0.0f;

	if (!TryGetFinalFireInterval(FinalFireInterval))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 서버에서 실제 데미지를 처리할 때만 발사 그룹 ID 필요.
	AttackFeedbackGroupId = ActorInfo->IsNetAuthority() ? FGuid::NewGuid() : FGuid();

	PlayFireMontage();

	if (bLogPredictionKey)
	{
		NS_ACTOR_LOG(ActorInfo->AvatarActor.Get(), LogNSGAS, Log,
			"EngineerShotgunFire 활성화. 로컬조작={LocallyControlled} 예측키={PredictionKey}",
			("LocallyControlled", ActorInfo->IsLocallyControlled()),
			("PredictionKey", GetCurrentPredictionKeyStatus())
		);
	}

	OnTargetDataReadyCallbackDelegateHandle = ASC->AbilityTargetDataSetDelegate(
		Handle,
		ActivationInfo.GetActivationPredictionKey()
	).AddUObject(this, &ThisClass::OnTargetDataReadyCallback);

	const bool bShouldWaitForClientTargetData = ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled();

	if (bShouldWaitForClientTargetData)
	{
		if (bLogPredictionKey)
		{
			NS_ACTOR_LOG(ActorInfo->AvatarActor.Get(), LogNSGAS, Log,
				"EngineerShotgunFire 클라이언트 TargetData 대기. 예측키={PredictionKey}",
				("PredictionKey", GetCurrentPredictionKeyStatus())
			);
		}

		ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationInfo.GetActivationPredictionKey());
	}
	else
	{
		if (bLogPredictionKey)
		{
			NS_ACTOR_LOG(ActorInfo->AvatarActor.Get(), LogNSGAS, Log,
				"EngineerShotgunFire 로컬 TargetData 생성. 예측키={PredictionKey}",
				("PredictionKey", GetCurrentPredictionKeyStatus())
			);
		}

		FireOnce();
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	World->GetTimerManager().SetTimer(
		FireDelayTimerHandle,
		this,
		&ThisClass::FinishFireCycle,
		FinalFireInterval,
		false
	);
}

void UGA_EngineerShotgunFire::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireDelayTimerHandle);
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (OnTargetDataReadyCallbackDelegateHandle.IsValid())
		{
			ASC->AbilityTargetDataSetDelegate(
				Handle,
				ActivationInfo.GetActivationPredictionKey()
			).Remove(OnTargetDataReadyCallbackDelegateHandle);

			OnTargetDataReadyCallbackDelegateHandle.Reset();
		}

		ASC->ConsumeClientReplicatedTargetData(Handle, ActivationInfo.GetActivationPredictionKey());
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_EngineerShotgunFire::FireOnce()
{
	FVector TraceStart;
	FVector CenterDirection;
	
	if (!TryBuildShotgunTraceBasis(TraceStart, CenterDirection))
	{
		return;
	}
	
	TArray<FHitResult> PelletHitResults;
	int32 FinalPelletCount = 0;
	float FinalSpreadAngleDegrees = 0.0f;
	float FinalFireRange = 0.0f;
	
	if (!TryGetFinalPelletCount(FinalPelletCount))
	{
		return;
	}
	
	if (!TryGetFinalSpreadAngleDegrees(FinalSpreadAngleDegrees))
	{
		return;
	}
	
	if (!TryGetFinalFireRange(FinalFireRange))
	{
		return;
	}
	
	PelletHitResults.Reserve(FinalPelletCount);
	
	for (int32 PelletIndex = 0; PelletIndex < FinalPelletCount; ++PelletIndex)
	{
		FHitResult HitResult;
		FVector TraceEnd;
		bool bHit = false;
		
		const FVector PelletDirection =
			BuildPelletDirection(CenterDirection, PelletIndex, FinalSpreadAngleDegrees);
		
		if (!TryBuildPelletTrace(TraceStart, PelletDirection, FinalFireRange, HitResult, TraceEnd, bHit))
		{
			continue;
		}
		
		HitResult.TraceStart = TraceStart;
		HitResult.TraceEnd = TraceEnd;
		
		if (!bHit)
		{
			HitResult.Location = TraceEnd;
			HitResult.ImpactPoint = TraceEnd;
		}

		PelletHitResults.Add(HitResult);
	}
	
	if (PelletHitResults.Num() <= 0)
	{
		return;
	}
	
	const FGameplayAbilityTargetDataHandle TargetDataHandle = MakeTargetDataFromHitResults(PelletHitResults);
	OnTargetDataReadyCallback(TargetDataHandle, FGameplayTag());
}

void UGA_EngineerShotgunFire::PlayFireMontage()
{
	if (!FireMontage)
	{
		return;
	}

	const float MontagePlayRate = FMath::Max(FireMontagePlayRate, 0.01f);

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			FireMontage,
			MontagePlayRate,
			NAME_None,
			true
		);

	if (!MontageTask)
	{
		return;
	}

	MontageTask->ReadyForActivation();
}

void UGA_EngineerShotgunFire::FinishFireCycle()
{
	bFireCycleElapsed = true;

	if (IsWaitingForRemoteClientTargetData() && !bTargetDataProcessed)
	{
		return;
	}

	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		false
	);
}

bool UGA_EngineerShotgunFire::TryGetFinalDamage(float& OutDamage)
{
	if (!TryGetFinalSkillDamage(NSGameplayTags::Ability_Engineer_ShotgunFire, OutDamage))
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Warning,
			"ShotgunFire 스킬 데미지 계산 실패. AbilityTag={AbilityTag}",
			("AbilityTag", NSGameplayTags::Ability_Engineer_ShotgunFire.GetTag().ToString())
		);

		return false;
	}

	return true;
}

bool UGA_EngineerShotgunFire::TryGetFinalFireInterval(float& OutFireInterval)
{
	float FinalFireRate = 0.0f;

	if (!TryGetFinalAbilityStat(
		NSGameplayTags::Ability_Engineer_ShotgunFire,
		NSGameplayTags::CombatStat_FireRate,
		FinalFireRate))
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Warning,
			"EngineerShotgunFire FireRate CombatStat 조회 실패. AbilityTag={AbilityTag}, StatTag={StatTag}",
			("AbilityTag", NSGameplayTags::Ability_Engineer_ShotgunFire.GetTag().ToString()),
			("StatTag", NSGameplayTags::CombatStat_FireRate.GetTag().ToString())
		);

		return false;
	}

	constexpr float MinFireRate = 0.01f;
	constexpr float MinFireInterval = 0.01f;

	FinalFireRate = FMath::Max(FinalFireRate, MinFireRate);
	OutFireInterval = FMath::Max(1.0f / FinalFireRate, MinFireInterval);

	return true;
}

bool UGA_EngineerShotgunFire::TryGetFinalFireRange(float& OutFireRange) const
{
	float FinalFireRange = 0.0f;

	if (!TryGetFinalAbilityStat(
		NSGameplayTags::Ability_Engineer_ShotgunFire,
		NSGameplayTags::CombatStat_FireRange,
		FinalFireRange))
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Warning,
			"EngineerShotgunFire FireRange CombatStat 조회 실패. AbilityTag={AbilityTag}, StatTag={StatTag}",
			("AbilityTag", NSGameplayTags::Ability_Engineer_ShotgunFire.GetTag().ToString()),
			("StatTag", NSGameplayTags::CombatStat_FireRange.GetTag().ToString())
		);

		return false;
	}

	OutFireRange = FMath::Max(FinalFireRange, 0.0f);
	return true;
}

bool UGA_EngineerShotgunFire::TryGetFinalPelletCount(int32& OutPelletCount) const
{
	float FinalPelletCount = 0.0f;

	if (!TryGetFinalAbilityStat(
		NSGameplayTags::Ability_Engineer_ShotgunFire,
		NSGameplayTags::CombatStat_PelletCount,
		FinalPelletCount))
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Warning,
			"EngineerShotgunFire PelletCount CombatStat 조회 실패. AbilityTag={AbilityTag}, StatTag={StatTag}",
			("AbilityTag", NSGameplayTags::Ability_Engineer_ShotgunFire.GetTag().ToString()),
			("StatTag", NSGameplayTags::CombatStat_PelletCount.GetTag().ToString())
		);

		return false;
	}

	OutPelletCount = FMath::Max(FMath::RoundToInt(FinalPelletCount), 1);
	return true;
}

bool UGA_EngineerShotgunFire::TryGetFinalSpreadAngleDegrees(float& OutSpreadAngleDegrees) const
{
	float FinalSpreadAngleDegrees = 0.0f;

	if (!TryGetFinalAbilityStat(
		NSGameplayTags::Ability_Engineer_ShotgunFire,
		NSGameplayTags::CombatStat_PelletSpread,
		FinalSpreadAngleDegrees))
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Warning,
			"EngineerShotgunFire PelletSpread CombatStat 조회 실패. AbilityTag={AbilityTag}, StatTag={StatTag}",
			("AbilityTag", NSGameplayTags::Ability_Engineer_ShotgunFire.GetTag().ToString()),
			("StatTag", NSGameplayTags::CombatStat_PelletSpread.GetTag().ToString())
		);

		return false;
	}

	OutSpreadAngleDegrees = FMath::Max(FinalSpreadAngleDegrees, 0.0f);
	return true;
}

bool UGA_EngineerShotgunFire::TryGetAimTraceStartLocation(FVector& OutLocation) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(AvatarActor);

	if (!IsValid(PlayerCharacter))
	{
		return false;
	}

	return PlayerCharacter->TryGetAimTraceStartLocation(OutLocation);
}

bool UGA_EngineerShotgunFire::TryGetAttackOriginTransform(FTransform& OutTransform) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(AvatarActor);

	if (!IsValid(PlayerCharacter))
	{
		return false;
	}

	const ANSWeaponBase* CurrentWeapon = PlayerCharacter->GetCurrentWeapon();

	if (!IsValid(CurrentWeapon))
	{
		return false;
	}

	return CurrentWeapon->TryGetAttackOriginTransform(OutTransform);
}

bool UGA_EngineerShotgunFire::TryBuildShotgunTraceBasis(
	FVector& OutTraceStart,
	FVector& OutCenterDirection) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (!IsValid(AvatarActor))
	{
		return false;
	}

	if (!TryGetAimTraceStartLocation(OutTraceStart))
	{
		return false;
	}

	const APawn* Pawn = Cast<APawn>(AvatarActor);
	const APlayerController* PlayerController = Pawn ?
		Cast<APlayerController>(Pawn->GetController()) : nullptr;

	if (!IsValid(PlayerController))
	{
		return false;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);

	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return false;
	}

	const float CrosshairScreenX = ViewportSizeX * 0.5f;
	const float CrosshairScreenY = ViewportSizeY * 0.5f;

	FVector DeprojectWorldLocation;
	FVector DeprojectWorldDirection;

	if (!PlayerController->DeprojectScreenPositionToWorld(
		CrosshairScreenX,
		CrosshairScreenY,
		DeprojectWorldLocation,
		DeprojectWorldDirection))
	{
		return false;
	}

	OutCenterDirection = DeprojectWorldDirection.GetSafeNormal();

	return !OutCenterDirection.IsNearlyZero();
}

FVector UGA_EngineerShotgunFire::BuildPelletDirection(
	const FVector& CenterDirection,
	int32 PelletIndex,
	float FinalSpreadAngleDegrees) const
{
	if (PelletIndex == 0)
	{
		return CenterDirection;
	}

	const float ConeHalfAngleRad = FMath::DegreesToRadians(FinalSpreadAngleDegrees * 0.5f);
	return FMath::VRandCone(CenterDirection, ConeHalfAngleRad).GetSafeNormal();
}

bool UGA_EngineerShotgunFire::TryBuildPelletTrace(
	const FVector& TraceStart,
	const FVector& TraceDirection,
	float FinalFireRange,
	FHitResult& OutHitResult,
	FVector& OutTraceEnd,
	bool& bOutHit) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();

	if (!IsValid(AvatarActor) || !World || TraceDirection.IsNearlyZero())
	{
		return false;
	}

	OutTraceEnd = TraceStart + TraceDirection * FinalFireRange;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);

	bOutHit = World->LineTraceSingleByChannel(
		OutHitResult,
		TraceStart,
		OutTraceEnd,
		NSCollisionChannels::PlayerWeaponTrace,
		QueryParams
	);

	return true;
}

FGameplayAbilityTargetDataHandle UGA_EngineerShotgunFire::MakeTargetDataFromHitResults(
	const TArray<FHitResult>& HitResults) const
{
	FGameplayAbilityTargetDataHandle TargetDataHandle;

	for (const FHitResult& HitResult : HitResults)
	{
		FGameplayAbilityTargetData_SingleTargetHit* TargetData =
			new FGameplayAbilityTargetData_SingleTargetHit();

		TargetData->HitResult = HitResult;
		TargetDataHandle.Add(TargetData);
	}

	return TargetDataHandle;
}

void UGA_EngineerShotgunFire::OnTargetDataReadyCallback(
	const FGameplayAbilityTargetDataHandle& TargetDataHandle,
	FGameplayTag ApplicationTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!ASC)
	{
		return;
	}

	FScopedPredictionWindow ScopedPredictionWindow(ASC);
	FGameplayAbilityTargetDataHandle LocalTargetDataHandle = TargetDataHandle;
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();

	if (bLogPredictionKey && ActorInfo)
	{
		NS_ACTOR_LOG(ActorInfo->AvatarActor.Get(), LogNSGAS, Log,
			"EngineerShotgunFire TargetData 준비. 로컬조작={LocallyControlled} TargetData개수={TargetDataNum} 예측키={PredictionKey}",
			("LocallyControlled", ActorInfo->IsLocallyControlled()),
			("TargetDataNum", TargetDataHandle.Num()),
			("PredictionKey", GetCurrentPredictionKeyStatus())
		);
	}

	const bool bShouldNotifyServer =
		ActorInfo && ActorInfo->IsLocallyControlled() && !ActorInfo->IsNetAuthority();

	if (bShouldNotifyServer)
	{
		ASC->CallServerSetReplicatedTargetData(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActivationInfo().GetActivationPredictionKey(),
			LocalTargetDataHandle,
			ApplicationTag,
			ASC->ScopedPredictionKey
		);
	}

	OnShotgunTargetDataReady(LocalTargetDataHandle);

	if (IsWaitingForRemoteClientTargetData())
	{
		bTargetDataProcessed = true;

		if (bFireCycleElapsed)
		{
			EndAbility(
				GetCurrentAbilitySpecHandle(),
				GetCurrentActorInfo(),
				GetCurrentActivationInfo(),
				true,
				false
			);

			return;
		}
	}

	ASC->ConsumeClientReplicatedTargetData(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActivationInfo().GetActivationPredictionKey());
}

void UGA_EngineerShotgunFire::OnShotgunTargetDataReady(
	const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();

	const bool bShouldExecuteCue =
		ActorInfo && (ActorInfo->IsLocallyControlled() || ActorInfo->IsNetAuthority());

	if (bShouldExecuteCue)
	{
		ExecuteMuzzleFireCue();
		ExecuteBulletTrailCue(TargetDataHandle);
	}

	if (ShouldPlayLocalFeedback() && bDrawDebugHitscan)
	{
		DrawDebugTargetData(TargetDataHandle);
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (ShouldPlayLocalFeedback() && AvatarActor && !AvatarActor->HasAuthority())
	{
		ExecutePredictedImpactCue(TargetDataHandle);
	}

	if (!AvatarActor || !AvatarActor->HasAuthority())
	{
		return;
	}

	FTransform AttackOriginTransform;
	FVector NoiseLocation = AvatarActor->GetActorLocation();
	
	if (TryGetAttackOriginTransform(AttackOriginTransform))
	{
		NoiseLocation = AttackOriginTransform.GetLocation();
	}
	
	TryReportAbilityNoise(
		NSGameplayTags::Ability_Engineer_ShotgunFire,
		NSGameplayTags::CombatStat_NoiseFireLoudness,
		NoiseLocation
	);
	
	ProcessTargetDataForDamage(TargetDataHandle);
}

void UGA_EngineerShotgunFire::ProcessTargetDataForDamage(
	const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if (TargetDataHandle.Num() <= 0)
	{
		return;
	}

	for (int32 Idx = 0; Idx < TargetDataHandle.Num(); ++Idx)
	{
		const FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(Idx);

		if (!TargetData)
		{
			continue;
		}

		const FHitResult* ClientHitResult = TargetData->GetHitResult();

		if (!ClientHitResult)
		{
			continue;
		}

		FHitResult ServerHitResult;
		FVector ServerTraceStart;
		FVector ServerTraceEnd;
		bool bServerAimHit = false;

		// RangerAutoFire와 동일하게 서버 기준 Trace를 다시 구성
		if (!TryBuildServerPelletTrace(
			*ClientHitResult,
			ServerHitResult,
			ServerTraceStart,
			ServerTraceEnd,
			bServerAimHit))
		{
			continue;
		}

		// 클라이언트 TargetData는 입력 검증에만 사용하고 실제 판정은 서버 Trace를 기준으로 처리
		if (!IsTargetDataTraceValid(
			*ClientHitResult,
			ServerHitResult,
			ServerTraceStart,
			ServerTraceEnd,
			bServerAimHit))
		{
			continue;
		}

		// 카메라 Trace가 허공을 향해도 총구 Trace가 막힌 대상은 데미지 대상으로 처리
		const FVector AimPoint = bServerAimHit ? FVector(ServerHitResult.ImpactPoint) : ServerTraceEnd;
		const AActor* AimTargetActor = bServerAimHit ? ServerHitResult.GetActor() : nullptr;

		FHitResult MuzzleObstructionHitResult;

		if (IsMuzzleObstructed(AimPoint, AimTargetActor, MuzzleObstructionHitResult))
		{
			// BackTrace로 늘어난 구간은 실제 명중 거리에서 제외.
			const float BackTraceDistance = FMath::Max(MuzzleObstructionBackTraceDistance, 0.0f);
			const float HitDistance = FMath::Max(MuzzleObstructionHitResult.Distance - BackTraceDistance, 0.0f);

			ApplyDamageToActor(MuzzleObstructionHitResult, HitDistance);
			ExecuteImpactCue(MuzzleObstructionHitResult);
			continue;
		}

		if (!bServerAimHit)
		{
			continue;
		}

		ApplyDamageToActor(ServerHitResult, ServerHitResult.Distance);
		ExecuteImpactCue(ServerHitResult);
	}

	// 모든 펠릿 GE 적용이 끝난 다음에 완료 신호를 보냄.
	CompleteAttackFeedbackGroup();
}

void UGA_EngineerShotgunFire::CompleteAttackFeedbackGroup() const
{
	if (!AttackFeedbackGroupId.IsValid())
	{
		return;
	}

	const APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());

	ANSPlayerController* PlayerController = AvatarPawn
		? Cast<ANSPlayerController>(AvatarPawn->GetController()) : nullptr;

	if (!PlayerController)
	{
		return;
	}

	// 대상별 피드백 RPC를 모두 보낸 뒤 대표 피드백 재생을 요청.
	PlayerController->Client_CompleteAttackHitFeedbackGroup(AttackFeedbackGroupId);
}

void UGA_EngineerShotgunFire::ApplyDamageToActor(const FHitResult& HitResult, float HitDistance)
{
	AActor* TargetActor = HitResult.GetActor();
	if (!TargetActor || !DamageEffectClass)
	{
		return;
	}
	
	if (!NSDamageRules::IsValidDirectDamageHit(HitResult))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	if (!SourceASC || !TargetASC)
	{
		return;
	}

	if (!NSDamageRules::CanApplyDamage(GetAvatarActorFromActorInfo(), TargetActor))
	{
		return;
	}

	float FinalDamage = 0.0f;

	if (!TryGetFinalDamage(FinalDamage))
	{
		return;
	}

	float DamageFalloffMultiplier = 1.0f;

	if (!TryCalculateDamageFalloffMultiplier(
		NSGameplayTags::Ability_Engineer_ShotgunFire,
		HitDistance,
		DamageFalloffMultiplier))
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Warning,
		"Shotgun 거리 감쇠 계산에 실패했습니다. HitDistance={HitDistance}",
		("HitDistance", HitDistance)
	);

		return;
	}

	// 펠릿의 실제 충돌 거리에 맞춰 최종 데미지를 감쇠.
	FinalDamage *= DamageFalloffMultiplier;
	
	// Physics Asset Body / BoneName 기반 부위 데미지 배율을 최종 데미지에 반영.
	FinalDamage *= NSDamageRules::ResolveDirectHitDamageMultiplier(HitResult);

	FGameplayEffectSpecHandle DamageSpecHandle =
		MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());

	if (!DamageSpecHandle.IsValid() || !DamageSpecHandle.Data.IsValid())
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = DamageSpecHandle.Data->GetContext();

	// AttributeSet이 이번 발사의 그룹 ID를 Ability에서 찾을 수 있게 함.
	EffectContext.AddSourceObject(this);
	EffectContext.AddHitResult(HitResult, true);

	ApplyDamageSetByCaller(DamageSpecHandle, FinalDamage);
	AssignDamageInstigator(DamageSpecHandle);

	SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
}

void UGA_EngineerShotgunFire::ApplyDamageSetByCaller(
	FGameplayEffectSpecHandle& InSpecHandle,
	float InDamage) const
{
	if (!InSpecHandle.IsValid() || !InSpecHandle.Data.IsValid())
	{
		return;
	}

	const float ClampedDamage = FMath::Max(InDamage, 0.0f);
	InSpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_Damage_Base, ClampedDamage);
}

void UGA_EngineerShotgunFire::ExecuteMuzzleFireCue()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (!ASC || !AvatarActor)
	{
		return;
	}

	FTransform MuzzleTransform;

	if (!TryGetAttackOriginTransform(MuzzleTransform))
	{
		MuzzleTransform = FTransform(
			AvatarActor->GetActorRotation(),
			AvatarActor->GetActorLocation() + AvatarActor->GetActorForwardVector() * 100.0f
		);
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Instigator = AvatarActor;
	CueParameters.EffectCauser = AvatarActor;
	CueParameters.Location = MuzzleTransform.GetLocation();
	CueParameters.Normal = MuzzleTransform.GetRotation().GetForwardVector();

	ASC->ExecuteGameplayCue(NSGameplayTags::GameplayCue_Engineer_ShotgunFire_MuzzleFire, CueParameters);
}

void UGA_EngineerShotgunFire::ExecuteBulletTrailCue(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	
	if (!ASC || !AvatarActor || TargetDataHandle.Num() <= 0)
	{
		return;
	}
	
	FTransform MuzzleTransform;
	
	if (!TryGetAttackOriginTransform(MuzzleTransform))
	{
		MuzzleTransform = FTransform(
			AvatarActor->GetActorRotation(),
			AvatarActor->GetActorLocation() + AvatarActor->GetActorForwardVector() * 100.0f
		);
	}
	
	// 시작점은 Muzzle 소켓
	const FVector TrailStart = MuzzleTransform.GetLocation();
	
	// 타겟 데이터 안에 샷건 발사 시 만들어진 펠릿별 히트Result가 있기 때문에 이를 순회함
	for (int32 Idx = 0; Idx < TargetDataHandle.Num(); ++Idx)
	{
		const FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(Idx);
		
		if (!TargetData)
		{
			continue;
		}
		
		const FHitResult* HitResult = TargetData->GetHitResult();
		
		if (!HitResult)
		{
			continue;
		}
		
		// 끝점은 히트된 경우 ImpactPoint까지, 히트되지 않고 허공에서 끝나는 경우 TraceEnd까지
		const FVector TrailEnd = HitResult->bBlockingHit
			? FVector(HitResult->ImpactPoint)
			: FVector(HitResult->TraceEnd);
		const FVector TrailDirection = (TrailEnd - TrailStart).GetSafeNormal();
		const float TrailDistance = FVector::Dist(TrailStart, TrailEnd);
		
		if (TrailDirection.IsNearlyZero() || TrailDistance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}
		
		FGameplayCueParameters CueParameters;
		CueParameters.Instigator = AvatarActor;
		CueParameters.EffectCauser = AvatarActor;
		CueParameters.Location = TrailStart;
		CueParameters.Normal = TrailDirection;
		CueParameters.RawMagnitude = TrailDistance;
		
		ASC->ExecuteGameplayCue(NSGameplayTags::GameplayCue_Engineer_ShotgunFire_BulletTrail, CueParameters);
	}
}

void UGA_EngineerShotgunFire::ExecuteImpactCue(const FHitResult& HitResult)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (!ASC || !AvatarActor || !HitResult.bBlockingHit)
	{
		return;
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Instigator = AvatarActor;
	CueParameters.EffectCauser = AvatarActor;
	CueParameters.Location = HitResult.ImpactPoint;
	CueParameters.Normal = HitResult.ImpactNormal;

	ASC->ExecuteGameplayCue(NSGameplayTags::GameplayCue_Engineer_ShotgunFire_Impact, CueParameters);
}

void UGA_EngineerShotgunFire::ExecutePredictedImpactCue(
	const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	for (int32 Idx = 0; Idx < TargetDataHandle.Num(); ++Idx)
	{
		const FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(Idx);

		if (!TargetData)
		{
			continue;
		}

		const FHitResult* LocalHitResult = TargetData->GetHitResult();

		if (!LocalHitResult)
		{
			continue;
		}

		const FHitResult PredictedImpactHitResult = *LocalHitResult;
		const FVector AimPoint = PredictedImpactHitResult.bBlockingHit ?
			FVector(PredictedImpactHitResult.ImpactPoint) :
			FVector(PredictedImpactHitResult.TraceEnd);
		const AActor* AimTargetActor = PredictedImpactHitResult.bBlockingHit ?
			PredictedImpactHitResult.GetActor() :
			nullptr;
		FHitResult MuzzleObstructionHitResult;

		if (IsMuzzleObstructed(AimPoint, AimTargetActor, MuzzleObstructionHitResult))
		{
			ExecuteImpactCue(MuzzleObstructionHitResult);
			continue;
		}

		if (!PredictedImpactHitResult.bBlockingHit)
		{
			continue;
		}

		ExecuteImpactCue(PredictedImpactHitResult);
	}
}

bool UGA_EngineerShotgunFire::ShouldPlayLocalFeedback() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const APawn* Pawn = Cast<APawn>(AvatarActor);

	if (!IsValid(Pawn))
	{
		return false;
	}

	return Pawn->IsLocallyControlled();
}

bool UGA_EngineerShotgunFire::IsMuzzleObstructed(
	const FVector& AimPoint,
	const AActor* AimTargetActor,
	FHitResult& OutObstructionHitResult) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();

	if (!IsValid(AvatarActor) || !World)
	{
		return false;
	}

	FTransform MuzzleTransform;

	if (!TryGetAttackOriginTransform(MuzzleTransform))
	{
		return false;
	}

	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	const FVector ShotDirection = (AimPoint - MuzzleLocation).GetSafeNormal();

	if (ShotDirection.IsNearlyZero())
	{
		return false;
	}

	const float BackTraceDistance = FMath::Max(MuzzleObstructionBackTraceDistance, 0.0f);
	const FVector ObstructionTraceStart = MuzzleLocation - ShotDirection * BackTraceDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);

	bool bHit = World->LineTraceSingleByChannel(
		OutObstructionHitResult,
		ObstructionTraceStart,
		AimPoint,
		NSCollisionChannels::PlayerWeaponTrace,
		QueryParams
	);

	const bool bHitAimTarget =
		IsValid(AimTargetActor) && OutObstructionHitResult.GetActor() == AimTargetActor;
	const bool bIsObstructed = bHit && OutObstructionHitResult.bBlockingHit && !bHitAimTarget;

	if (bDrawDebugMuzzleObstruction)
	{
		DrawDebugMuzzleObstructionTrace(
			ObstructionTraceStart,
			AimPoint,
			OutObstructionHitResult,
			bIsObstructed
		);
	}

	return bIsObstructed;
}

bool UGA_EngineerShotgunFire::TryBuildServerPelletTrace(
	const FHitResult& ClientHitResult,
	FHitResult& OutHitResult,
	FVector& OutTraceStart,
	FVector& OutTraceEnd,
	bool& bOutHit) const
{
	// RangerAutoFire의 서버 Trace 생성 단계와 같은 역할
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();

	if (!AvatarActor || !World || !AvatarActor->HasAuthority())
	{
		return false;
	}

	OutTraceStart = ClientHitResult.TraceStart;
	OutTraceEnd = ClientHitResult.TraceEnd;
	bOutHit = false;

	if (!TryGetAimTraceStartLocation(OutTraceStart))
	{
		return false;
	}

	const FVector ClientTraceDirection =
		(FVector(ClientHitResult.TraceEnd) - FVector(ClientHitResult.TraceStart)).GetSafeNormal();

	if (ClientTraceDirection.IsNearlyZero())
	{
		return false;
	}

	float FinalFireRange = 0.0f;

	if (!TryGetFinalFireRange(FinalFireRange))
	{
		return false;
	}

	OutTraceEnd = OutTraceStart + ClientTraceDirection * FinalFireRange;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);

	bOutHit = World->LineTraceSingleByChannel(
		OutHitResult,
		OutTraceStart,
		OutTraceEnd,
		NSCollisionChannels::PlayerWeaponTrace,
		QueryParams
	);

	return true;
}

bool UGA_EngineerShotgunFire::IsTargetDataTraceValid(
	const FHitResult& ClientHitResult,
	const FHitResult& ServerHitResult,
	const FVector& ServerTraceStart,
	const FVector& ServerTraceEnd,
	bool bServerAimHit) const
{
	// RangerAutoFire와 동일하게 클라이언트 TargetData와 서버 Trace 결과를 비교 검증
	const FVector ClientTraceStart = ClientHitResult.TraceStart;
	const FVector ClientTraceEnd = ClientHitResult.TraceEnd;

	if (ClientTraceStart.Equals(ClientTraceEnd))
	{
		return false;
	}

	float FinalFireRange = 0.0f;

	if (!TryGetFinalFireRange(FinalFireRange))
	{
		return false;
	}

	const float MaxTraceDistance = FinalFireRange + ServerHitLocationTolerance;
	if (FVector::DistSquared(ClientTraceStart, ClientTraceEnd) > FMath::Square(MaxTraceDistance))
	{
		return false;
	}

	if (FVector::DistSquared(ServerTraceStart, ClientTraceStart) > FMath::Square(ServerTraceStartTolerance))
	{
		return false;
	}

	if (!bServerAimHit || !ServerHitResult.bBlockingHit)
	{
		return !ClientHitResult.bBlockingHit;
	}

	if (!ClientHitResult.bBlockingHit)
	{
		return true;
	}

	if (!IsValid(ClientHitResult.GetActor()))
	{
		return false;
	}

	if (ServerHitResult.GetActor() != ClientHitResult.GetActor())
	{
		return false;
	}

	if (FVector::DistSquared(ServerHitResult.ImpactPoint, ClientHitResult.ImpactPoint)
		> FMath::Square(ServerHitLocationTolerance))
	{
		return false;
	}

	return true;
}

void UGA_EngineerShotgunFire::AssignDamageInstigator(FGameplayEffectSpecHandle& InSpecHandle)
{
	if (!InSpecHandle.IsValid())
	{
		return;
	}

	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		InSpecHandle.Data->GetContext().AddInstigator(AvatarActor, AvatarActor);
	}
}

bool UGA_EngineerShotgunFire::IsWaitingForRemoteClientTargetData() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();

	return ActorInfo && ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled();
}

void UGA_EngineerShotgunFire::DrawDebugTargetData(
	const FGameplayAbilityTargetDataHandle& TargetDataHandle) const
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	for (int32 Idx = 0; Idx < TargetDataHandle.Num(); ++Idx)
	{
		const FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(Idx);

		if (!TargetData)
		{
			continue;
		}

		const FHitResult* HitResult = TargetData->GetHitResult();

		if (!HitResult)
		{
			continue;
		}

		const FVector TraceStart = HitResult->TraceStart;
		const FVector TraceEnd = HitResult->TraceEnd;

		if (TraceStart.Equals(TraceEnd))
		{
			continue;
		}

		const bool bHit = HitResult->bBlockingHit;
		const FVector DebugEnd = bHit ? HitResult->ImpactPoint : TraceEnd;
		const FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		const FVector DebugDirection = (DebugEnd - TraceStart).GetSafeNormal();

		if (DebugDirection.IsNearlyZero())
		{
			continue;
		}

		const float DebugDistance = FVector::Dist(TraceStart, DebugEnd);
		const float AppliedOffset =
			FMath::Clamp(DebugLineStartOffset, 0.0f, FMath::Max(DebugDistance - 10.0f, 0.0f));
		const FVector DebugStart = TraceStart + DebugDirection * AppliedOffset;

		DrawDebugLine(
			World,
			DebugStart,
			DebugEnd,
			DebugColor,
			false,
			DebugLineDuration,
			0,
			DebugLineThickness
		);

		if (bHit)
		{
			DrawDebugPoint(
				World,
				HitResult->ImpactPoint,
				12.0f,
				FColor::Red,
				false,
				DebugLineDuration
			);
		}
	}
}

void UGA_EngineerShotgunFire::DrawDebugMuzzleObstructionTrace(
	const FVector& TraceStart,
	const FVector& TraceEnd,
	const FHitResult& ObstructionHitResult,
	bool bIsObstructed) const
{
	UWorld* World = GetWorld();

	if (!World || TraceStart.Equals(TraceEnd))
	{
		return;
	}

	const bool bHit = ObstructionHitResult.bBlockingHit;
	const FVector DebugEnd = bHit ? ObstructionHitResult.ImpactPoint : TraceEnd;
	const FColor DebugColor = bIsObstructed ? FColor::Orange : FColor::Cyan;

	DrawDebugLine(
		World,
		TraceStart,
		DebugEnd,
		DebugColor,
		false,
		DebugLineDuration,
		0,
		DebugLineThickness
	);

	if (bHit)
	{
		DrawDebugPoint(
			World,
			ObstructionHitResult.ImpactPoint,
			14.0f,
			DebugColor,
			false,
			DebugLineDuration
		);
	}
}
