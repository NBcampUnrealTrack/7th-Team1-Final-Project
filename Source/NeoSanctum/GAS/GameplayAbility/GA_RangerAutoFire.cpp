// Copyright 2026 One Team. All rights reserved.


#include "GA_RangerAutoFire.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Combat/NSDamageRules.h"
#include "NeoSanctum/Combat/Weapon/NSWeaponBase.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_RangerAutoFire::UGA_RangerAutoFire()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Ranger_AutoFire);
	SetAssetTags(AssetTags);
	
	// 입력을 유지하면 Ability가 한 발 단위로 다시 활성화.
	// 내부 반복 타이머가 아니라 Ability 활성화 주기로 연사를 구성.
	ActivationPolicy = ENSAbilityActivationPolicy::WhileInputActive;
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Reloading);
	bRequestReloadOnEmptyAmmo = true;
}

void UGA_RangerAutoFire::ActivateAbility(
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
	
	// 한 발 발사 주기마다 종료 조건을 새로 추적.
	// 원격 클라이언트는 Timer 종료와 TargetData 수신 순서가 달라질 수 있음.
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
		// TODO: 탄약 관련기능이 생기면 0발 때는 재장전 GA에게 이벤트 전달
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	PlayFireMontage();

	if (bLogPredictionKey && ActorInfo)
	{
		NS_ACTOR_LOG(ActorInfo->AvatarActor.Get(), LogNSGAS, Log,
			"레인저 연사 활성화. 로컬조작={LocallyControlled} 예측키={PredictionKey}",
			("LocallyControlled", ActorInfo->IsLocallyControlled()),
			("PredictionKey", GetCurrentPredictionKeyStatus())
		);
	}

	// 서버가 클라이언트 TargetData를 받을 수 있도록 델리게이트 등록
	OnTargetDataReadyCallbackDelegateHandle = ASC->AbilityTargetDataSetDelegate(
		Handle,
		ActivationInfo.GetActivationPredictionKey()
	).AddUObject(this, &ThisClass::OnTargetDataReadyCallback);

	// 서버의 원격 클라이언트 캐릭터인지 판단
	const bool bShouldWaitForClientTargetData = ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled();

	if (bShouldWaitForClientTargetData)
	{
		if (bLogPredictionKey && ActorInfo)
		{
			NS_ACTOR_LOG(ActorInfo->AvatarActor.Get(), LogNSGAS, Log,
				"클라이언트 TargetData 대기 중. 예측키={PredictionKey}",
				("PredictionKey", GetCurrentPredictionKeyStatus())
			);
		}

		// TargetData가 델리게이트 등록보다 먼저 도착한 경우 처리
		// 바로 OnTargetDataReadyCallback 실행
		ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationInfo.GetActivationPredictionKey());
	}
	else
	{
		if (bLogPredictionKey && ActorInfo)
		{
			NS_ACTOR_LOG(ActorInfo->AvatarActor.Get(), LogNSGAS, Log,
				"로컬 TargetData 생성. 예측키={PredictionKey}",
				("PredictionKey", GetCurrentPredictionKeyStatus())
			);
		}
		// 로컬 조작 클라이언트나 호스트는 직접 TargetData를 만듬
		FireOnce();
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	// 최종 FireRate로 계산한 간격 동안 Ability를 Active 상태로 유지해 연사 속도를 제한
	World->GetTimerManager().SetTimer(
		FireDelayTimerHandle,
		this,
		&ThisClass::FinishFireCycle,
		FinalFireInterval,
		false
	);
}

void UGA_RangerAutoFire::EndAbility(
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

	// 이전 발사의 델리게이트 제거
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

		// ASC 내부의 저장된 복제 TargetData 소비/정리
		ASC->ConsumeClientReplicatedTargetData(Handle, ActivationInfo.GetActivationPredictionKey());
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_RangerAutoFire::FinishFireCycle()
{
	bFireCycleElapsed = true;
	
	// 원격 클라이언트의 경우 FireInterval이 먼저 끝날 수 있음.
	// TargetData를 아직 처리하지 않았다면 수신 완료 시점까지 Ability 종료를 미룬다.
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

bool UGA_RangerAutoFire::TryGetFinalFireInterval(float& OutFireInterval)
{
	float FinalFireRate = 0.0f;
	
	if (!TryGetFinalAbilityStat(
		NSGameplayTags::Ability_Ranger_AutoFire,
		NSGameplayTags::CombatStat_FireRate,
		FinalFireRate))
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Warning,
			"AutoFire FireRate CombatStat 조회 실패. AbilityTag={AbilityTag}, StatTag={StatTag}",
			("AbilityTag", NSGameplayTags::Ability_Ranger_AutoFire.GetTag().ToString()),
			("StatTag", NSGameplayTags::CombatStat_FireRate.GetTag().ToString())
		);
		
		return false;
	}
	
	// FireRate가 0 이하이거나 데이터가 비정상이어도
	// 0으로 나누거나 즉시 재활성화되는 상황을 방지.
	constexpr float MinFireRate = 0.01f;
	constexpr float MinFireInterval = 0.01f;
	
	FinalFireRate = FMath::Max(FinalFireRate, MinFireRate);
	OutFireInterval = FMath::Max(1.0f / FinalFireRate, MinFireInterval);
	
	return true;
}

bool UGA_RangerAutoFire::TryGetFinalFireRange(float& OutFireRange) const
{
	float FinalFireRange = 0.0f;

	if (!TryGetFinalAbilityStat(
		NSGameplayTags::Ability_Ranger_AutoFire,
		NSGameplayTags::CombatStat_FireRange,
		FinalFireRange))
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Warning,
			"AutoFire FireRange CombatStat 조회 실패. AbilityTag={AbilityTag}, StatTag={StatTag}",
			("AbilityTag", NSGameplayTags::Ability_Ranger_AutoFire.GetTag().ToString()),
			("StatTag", NSGameplayTags::CombatStat_FireRange.GetTag().ToString())
		);

		return false;
	}

	OutFireRange = FMath::Max(FinalFireRange, 0.0f);
	return true;
}

void UGA_RangerAutoFire::FireOnce()
{
	// 로컬 Trace는 TargetData 생성용.
	// 실제 명중 판정과 데미지는 서버에서 다시 검증.
	
	FHitResult HitResult;
	FVector TraceStart;
	FVector TraceEnd;
	bool bHit = false;
	
	if (!TryBuildHitscanTrace(HitResult, TraceStart, TraceEnd, bHit))
	{
		return;
	}
	
	// 서버 검증에서 사용할 Trace 정보를 항상 채움
	HitResult.TraceStart = TraceStart;
	HitResult.TraceEnd = TraceEnd;
	
	// 명중하지 않아도 TraceStart / TraceEnd를 전달해야
	// 서버가 허공 조준에 대한 총구 막힘 판정을 수행할 수 있다.
	if (!bHit)
	{
		HitResult.Location = TraceEnd;
		HitResult.ImpactPoint = TraceEnd;
	}
	
	const FGameplayAbilityTargetDataHandle TargetDataHandle = MakeTargetDataFromHitResult(HitResult);
	OnTargetDataReadyCallback(TargetDataHandle, FGameplayTag());
}

void UGA_RangerAutoFire::PlayFireMontage()
{
	if (!FireMontage)
	{
		return;
	}
	
	const float MontagePlayRate = FMath::Max(FireMontagePlayRate, 0.01f);
	
	// 발사 판정은 FireInterval이 관리하므로 몽타주는 연출 피드백으로만 재생
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

FGameplayAbilityTargetDataHandle UGA_RangerAutoFire::MakeTargetDataFromHitResult(const FHitResult& HitResult) const
{
	FGameplayAbilityTargetDataHandle TargetDataHandle;
	
	FGameplayAbilityTargetData_SingleTargetHit* TargetData =
		new FGameplayAbilityTargetData_SingleTargetHit();
	
	TargetData->HitResult = HitResult;
	TargetDataHandle.Add(TargetData);
	
	return TargetDataHandle;
}

void UGA_RangerAutoFire::OnTargetDataReadyCallback(
	const FGameplayAbilityTargetDataHandle& TargetDataHandle,
	FGameplayTag ApplicationTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!ASC)
	{
		return;
	}

	// TargetData RPC와 현재 Ability 활성화를 같은 PredictionKey 흐름으로 묶는다.
	FScopedPredictionWindow ScopedPredictionWindow(ASC);

	FGameplayAbilityTargetDataHandle LocalTargetDataHandle = TargetDataHandle;

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();

	if (bLogPredictionKey && ActorInfo)
	{
		NS_ACTOR_LOG(ActorInfo->AvatarActor.Get(), LogNSGAS, Log,
			"TargetData 준비 완료. 로컬조작={LocallyControlled} TargetData개수={TargetDataNum} 예측키={PredictionKey}",
			("LocallyControlled", ActorInfo->IsLocallyControlled()),
			("TargetDataNum", TargetDataHandle.Num()),
			("PredictionKey", GetCurrentPredictionKeyStatus())
		);
	}

	const bool bShouldNotifyServer =
		ActorInfo && ActorInfo->IsLocallyControlled() && !ActorInfo->IsNetAuthority();

	// 원격 클라이언트만 TargetData를 서버 Ability로 전송.
	// 리슨 서버 호스트는 같은 인스턴스에서 직접 처리.
	if (bShouldNotifyServer)
	{
		if (bLogPredictionKey && ActorInfo)
		{
			NS_ACTOR_LOG(ActorInfo->AvatarActor.Get(), LogNSGAS, Log,
				"서버로 TargetData 전송. ScopedPredictionKey유효={ScopedPredictionKeyValid} 예측키={PredictionKey}",
				("ScopedPredictionKeyValid", ASC->ScopedPredictionKey.IsValidKey()),
				("PredictionKey", GetCurrentPredictionKeyStatus())
			);
		}

		// 클라이언트가 TargetData를 서버로 보내는 RPC
		// 같은 Ability Handle + PredictionKey를 가진 서버 Ability에게 전송
		ASC->CallServerSetReplicatedTargetData(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActivationInfo().GetActivationPredictionKey(),
			LocalTargetDataHandle,
			ApplicationTag,
			ASC->ScopedPredictionKey
		);
	}

	OnRangerTargetDataReady(LocalTargetDataHandle);
	
	// 원격 클라이언트 TargetData 처리가 끝났음을 기록.
	// FireInterval Timer가 먼저 끝난 경우 여기서 Ability 종료를 이어서 처리.
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

void UGA_RangerAutoFire::OnRangerTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	
	const bool bShouldExecuteCue = 
		ActorInfo && (ActorInfo->IsLocallyControlled() || ActorInfo->IsNetAuthority());
	
	// 발사 연출은 로컬 조작자와 서버 권한 경로에서 처리.
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
	
	// 로컬 클라이언트는 서버 결과를 기다리지 않고 ImpactCue를 예측 재생.
	if (ShouldPlayLocalFeedback() && AvatarActor && !AvatarActor->HasAuthority())
	{
		ExecutePredictedImpactCue(TargetDataHandle);
	}
	
	// 소음과 GameplayEffect 데미지는 서버 권한으로만 처리.
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
	
	// 실제 소음은 서버에서만 처리
	TryReportAbilityNoise(
		NSGameplayTags::Ability_Ranger_AutoFire,
		NSGameplayTags::CombatStat_NoiseFireLoudness,
		NoiseLocation
	);
	
	// 실제 데미지는 서버에서만 처리
	ProcessTargetDataForDamage(TargetDataHandle);
}

void UGA_RangerAutoFire::ProcessTargetDataForDamage(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if (TargetDataHandle.Num() <= 0)
	{
		return;
	}

	const FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(0);

	if (!TargetData)
	{
		return;
	}

	const FHitResult* ClientHitResult = TargetData->GetHitResult();

	if (!ClientHitResult)
	{
		return;
	}

	FHitResult ServerHitResult;
	FVector ServerTraceStart;
	FVector ServerTraceEnd;
	bool bServerAimHit = false;
	
	// 클라이언트 TargetData는 입력 검증에만 사용하고,
	// 실제 명중 판정은 서버 Canonical Aim Trace 결과를 사용.
	if (!TryBuildServerAimTrace(
		ServerHitResult,
		ServerTraceStart,
		ServerTraceEnd,
		bServerAimHit))
	{
		return;
	}
	
	if (!IsTargetDataTraceValid(
		*ClientHitResult,
		ServerHitResult,
		ServerTraceStart,
		ServerTraceEnd,
		bServerAimHit))
	{
		return;
	}
	
	// 조준 Trace가 빗나가도 서버 기준 TraceEnd까지 총구 막힘 검사는 수행.
	const FVector AimPoint = bServerAimHit ? FVector(ServerHitResult.ImpactPoint) : ServerTraceEnd;
	const AActor* AimTargetActor = bServerAimHit ? ServerHitResult.GetActor() : nullptr;
	
	FHitResult MuzzleObstructionHitResult;

	if (IsMuzzleObstructed(
		AimPoint,
		AimTargetActor,
		MuzzleObstructionHitResult))
	{
		// BackTrace로 늘어난 구간은 실제 명중 거리에서 제외.
		const float BackTraceDistance = FMath::Max(MuzzleObstructionBackTraceDistance, 0.0f);
		const float HitDistance = FMath::Max(MuzzleObstructionHitResult.Distance - BackTraceDistance, 0.0f);

		ApplyDamageToActor(MuzzleObstructionHitResult, HitDistance);
		ExecuteImpactCue(MuzzleObstructionHitResult);
		return;
	}

	if (!bServerAimHit)
	{
		return;
	}

	ApplyDamageToActor(ServerHitResult, ServerHitResult.Distance);
	ExecuteImpactCue(ServerHitResult);
}

void UGA_RangerAutoFire::ApplyDamageToActor(const FHitResult& HitResult, float HitDistance)
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
		NSGameplayTags::Ability_Ranger_AutoFire,
		HitDistance,
		DamageFalloffMultiplier))
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Warning,
			"AutoFire 거리 감쇠 계산 실패. HitDistance={HitDistance}",
			("HitDistance", HitDistance)
		);

		return;
	}

	// 감쇠된 값을 기존 Effect.Damage.Base 흐름으로 전달.
	FinalDamage *= DamageFalloffMultiplier;
	FinalDamage *= NSDamageRules::ResolveDirectHitDamageMultiplier(HitResult);
	
	// Attribute를 직접 변경하지 않고 GameplayEffect Spec으로 데미지를 전달.
	// Damage 값은 SetByCaller로 설정하고, 대상 ASC에 서버 권한으로 적용.
	FGameplayEffectSpecHandle DamageSpecHandle = 
		MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	
	if (!DamageSpecHandle.IsValid() || !DamageSpecHandle.Data.IsValid())
	{
		return;
	}
	
	ApplyDamageSetByCaller(DamageSpecHandle, FinalDamage);
	DamageSpecHandle.Data->GetContext().AddHitResult(HitResult, true);
	
	// 데미지 감지 가해자 지정
	AssignDamageInstigator(DamageSpecHandle);
	
	// GE_Damage -> GEC_DamageExecution -> Damage Meta Attribute 흐름으로 데미지 전달
	SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
}

bool UGA_RangerAutoFire::TryGetFinalDamage(float& OutDamage)
{
	if (!TryGetFinalSkillDamage(NSGameplayTags::Ability_Ranger_AutoFire, OutDamage))
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Warning,
			"AutoFire 스킬 데미지 계산 실패. AbilityTag={AbilityTag}",
			("AbilityTag", NSGameplayTags::Ability_Ranger_AutoFire.GetTag().ToString())
		);

		return false;
	}

	return true;
}

void UGA_RangerAutoFire::ApplyDamageSetByCaller(FGameplayEffectSpecHandle& InSpecHandle, float InDamage) const
{
	if (!InSpecHandle.IsValid() || !InSpecHandle.Data.IsValid())
	{
		return;
	}
	
	const float ClampedDamage = FMath::Max(InDamage, 0.0f);
	InSpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_Damage_Base, ClampedDamage);
}

void UGA_RangerAutoFire::DrawDebugTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle) const
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
		
		// 초록: 허공, 빨강: 명중
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

void UGA_RangerAutoFire::DrawDebugMuzzleObstructionTrace(
	const FVector& TraceStart,
	const FVector& TraceEnd,
	const FHitResult& ObstructionHitResult,
	bool bIsObstructed) const
{
	UWorld* World = GetWorld();
	
	if (!World)
	{
		return;
	}
	
	if (TraceStart.Equals(TraceEnd))
	{
		return;
	}
	
	const bool bHit = ObstructionHitResult.bBlockingHit;
	const FVector DebugEnd = bHit ? ObstructionHitResult.ImpactPoint : TraceEnd;
	const FColor DebugColor = bIsObstructed ? FColor::Orange : FColor::Cyan;
	
	// Cyan:	총구 경로가 열려 있음
	// Orange:	총구 경로가 벽/장애물에 막힘
	
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

bool UGA_RangerAutoFire::ShouldPlayLocalFeedback() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const APawn* Pawn = Cast<APawn>(AvatarActor);
	
	if (!IsValid(Pawn))
	{
		return false;
	}
	
	return Pawn->IsLocallyControlled();
}

bool UGA_RangerAutoFire::TryBuildHitscanTrace(
	FHitResult& OutHitResult,
	FVector& OutTraceStart,
	FVector& OutTraceEnd,
	bool& bOutHit) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (!IsValid(AvatarActor))
	{
		return false;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}
	
	// Trace 시작점은 카메라 위치를 사용하고,
	// 방향은 화면 중앙 Deproject 결과를 사용해 크로스헤어 조준과 일치시킨다.
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

	const FVector TraceDirection = DeprojectWorldDirection.GetSafeNormal();
	
	if (TraceDirection.IsNearlyZero())
	{
		return false;
	}

	float FinalFireRange = 0.0f;

	if (!TryGetFinalFireRange(FinalFireRange) || FinalFireRange <= 0.0f)
	{
		return false;
	}

	// 클라이언트도 CombatStat에서 가져온 최대 사정거리를 사용.
	OutTraceEnd = OutTraceStart + TraceDirection * FinalFireRange;
	
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

bool UGA_RangerAutoFire::TryBuildServerAimTrace(
	FHitResult& OutHitResult,
	FVector& OutTraceStart,
	FVector& OutTraceEnd,
	bool& bOutHit) const
{
	bOutHit = false;
	
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const APawn* Pawn = Cast<APawn>(AvatarActor);
	UWorld* World = GetWorld();
	
	if (!IsValid(AvatarActor) || !IsValid(Pawn) || !World || !AvatarActor->HasAuthority())
	{
		return false;
	}
	
	if (!TryGetAimTraceStartLocation(OutTraceStart))
	{
		return false;
	}
	
	// 서버는 Viewport Deproject를 사용할 수 없으므로,
	// 서버가 보유한 BaseAimRotation으로 판정용 조준 방향을 구성.
	const FVector ServerAimDirection = Pawn->GetBaseAimRotation().Vector().GetSafeNormal();
	
	if (ServerAimDirection.IsNearlyZero())
	{
		return false;
	}

	float FinalFireRange = 0.0f;

	if (!TryGetFinalFireRange(FinalFireRange) || FinalFireRange <= 0.0f)
	{
		return false;
	}

	// 서버 판정도 클라이언트와 같은 CombatStat 사정거리를 사용.
	OutTraceEnd = OutTraceStart + ServerAimDirection * FinalFireRange;
	
	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(RangerAutoFireServerAimTrace), false);
	
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

bool UGA_RangerAutoFire::TryGetAimTraceStartLocation(FVector& OutLocation) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(AvatarActor);
	
	if (!IsValid(PlayerCharacter))
	{
		return false;
	}
	
	return PlayerCharacter->TryGetAimTraceStartLocation(OutLocation);
}

void UGA_RangerAutoFire::ExecuteMuzzleFireCue()
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
		// 소켓을 못 찾으면 캐릭터 전방 위치로 임시 처리
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

	ASC->ExecuteGameplayCue(NSGameplayTags::GameplayCue_Ranger_AutoFire_MuzzleFire, CueParameters);
}

void UGA_RangerAutoFire::ExecuteBulletTrailCue(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
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
	
	const FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(0);
	
	if (!TargetData)
	{
		return;
	}
	
	const FHitResult* HitResult = TargetData->GetHitResult();
	
	if (!HitResult)
	{
		return;
	}
	
	// 끝점은 히트된 경우 ImpactPoint까지, 히트되지 않고 허공에서 끝나는 경우 TraceEnd까지
	const FVector TrailEnd = HitResult->bBlockingHit
		? FVector(HitResult->ImpactPoint)
		: FVector(HitResult->TraceEnd);
	const FVector TrailDirection = (TrailEnd - TrailStart).GetSafeNormal();
	const float TrailDistance = FVector::Dist(TrailStart, TrailEnd);
	
	if (TrailDirection.IsNearlyZero() || TrailDistance <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	
	FGameplayCueParameters CueParameters;
	CueParameters.Instigator = AvatarActor;
	CueParameters.EffectCauser = AvatarActor;
	CueParameters.Location = TrailStart;
	CueParameters.Normal = TrailDirection;
	CueParameters.RawMagnitude = TrailDistance;
	
	ASC->ExecuteGameplayCue(NSGameplayTags::GameplayCue_Ranger_AutoFire_BulletTrail, CueParameters);
}

void UGA_RangerAutoFire::ExecuteImpactCue(const FHitResult& HitResult)
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
	
	ASC->ExecuteGameplayCue(NSGameplayTags::GameplayCue_Ranger_AutoFire_Impact, CueParameters);
}

void UGA_RangerAutoFire::ExecutePredictedImpactCue(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if (TargetDataHandle.Num() <= 0)
	{
		return;
	}
	
	const FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(0);
	
	if (!TargetData)
	{
		return;
	}
	
	const FHitResult* LocalHitResult = TargetData->GetHitResult();
	
	if (!LocalHitResult)
	{
		return;
	}
	
	const FVector AimPoint = LocalHitResult->bBlockingHit
		? FVector(LocalHitResult->ImpactPoint)
		: FVector(LocalHitResult->TraceEnd);
	
	const AActor* AimTargetActor = LocalHitResult->bBlockingHit ? LocalHitResult->GetActor() : nullptr;
	
	FHitResult MuzzleObstructionHitResult;
	
	// 총구가 막힌 상황이면 조준 대상이 아니라 실제로 막힌 지점에 ImpactCue 표시
	if (IsMuzzleObstructed(AimPoint, AimTargetActor, MuzzleObstructionHitResult))
	{
		ExecuteImpactCue(MuzzleObstructionHitResult);
		return;
	}
	
	if (LocalHitResult->bBlockingHit)
	{
		ExecuteImpactCue(*LocalHitResult);
	}
}

bool UGA_RangerAutoFire::TryGetAttackOriginTransform(FTransform& OutTransform) const
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

bool UGA_RangerAutoFire::IsMuzzleObstructed(
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
	
	// 서버 Canonical AimPoint를 기준으로 총구 실제 발사 경로를 검사.
	// 조준 Trace가 허공인 경우에도 총구 앞 엄폐물을 처리할 수 있어야 함.
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
	
	const bool bHitAimTarget = IsValid(AimTargetActor) && OutObstructionHitResult.GetActor() == AimTargetActor;
	
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

bool UGA_RangerAutoFire::IsTargetDataTraceValid(
	const FHitResult& ClientHitResult,
	const FHitResult& ServerHitResult,
	const FVector& ServerTraceStart,
	const FVector& ServerTraceEnd,
	bool bServerAimHit) const
{
	const FVector ClientTraceStart = ClientHitResult.TraceStart;
	const FVector ClientTraceEnd = ClientHitResult.TraceEnd;
	
	if (ClientTraceStart.Equals(ClientTraceEnd))
	{
		return false;
	}

	float FinalFireRange = 0.0f;

	if (!TryGetFinalFireRange(FinalFireRange) || FinalFireRange <= 0.0f)
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
	
	// 클라이언트가 서버 조준 방향과 무관한 TraceEnd를 전달하는 것을 방지.
	const FVector ClientTraceDirection = (ClientTraceEnd - ClientTraceStart).GetSafeNormal();
	
	const FVector ServerTraceDirection = (ServerTraceEnd - ServerTraceStart).GetSafeNormal();
	
	if (ClientTraceDirection.IsNearlyZero() || ServerTraceDirection.IsNearlyZero())
	{
		return false;
	}
	
	const float ClampedToleranceDegrees = 
		FMath::Clamp(ServerAimDirectionToleranceDegrees, 0.0f, 45.0f);
	
	const float MinAcceptedAimDot = FMath::Cos(FMath::DegreesToRadians(ClampedToleranceDegrees));
	const float AimDirectionDot = FVector::DotProduct(ClientTraceDirection, ServerTraceDirection);
	
	if (AimDirectionDot < MinAcceptedAimDot)
	{
		if (bDrawDebugHitscan)
		{
			NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Warning,
				"서버 조준 방향 검증 실패. 입력Dot={AimDot}, 최소Dot={MinDot}, 허용각도={ToleranceDegrees}",
				("AimDot", AimDirectionDot),
				("MinDot", MinAcceptedAimDot),
				("ToleranceDegrees", ClampedToleranceDegrees)
			);
		}
		
		return false;
	}
	
	// 허공 TargetData도 방향 검증을 통과했다면 허용.
	// 이후 총구 막힘과 실제 데미지는 서버 Canonical Trace 결과만 사용.
	if (!ClientHitResult.bBlockingHit)
	{
		return true;
	}
	
	if (!bServerAimHit || !ServerHitResult.bBlockingHit)
	{
		return false;
	}
	
	if (!IsValid(ClientHitResult.GetActor()))
	{
		return false;
	}
	
	if (ServerHitResult.GetActor() != ClientHitResult.GetActor())
	{
		return false;
	}
	
	if (FVector::DistSquared(
		ServerHitResult.ImpactPoint,
		ClientHitResult.ImpactPoint)
		> FMath::Square(ServerHitLocationTolerance))
	{
		return false;
	}
	
	return true;
}

void UGA_RangerAutoFire::AssignDamageInstigator(FGameplayEffectSpecHandle& InSpecHandle)
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

bool UGA_RangerAutoFire::IsWaitingForRemoteClientTargetData() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	
	return ActorInfo && ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled();
}
