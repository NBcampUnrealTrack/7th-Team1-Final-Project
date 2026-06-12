// Copyright 2026 One Team. All rights reserved.


#include "GA_RangerAutoFire.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Combat/Weapon/NSWeaponBase.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"

UGA_RangerAutoFire::UGA_RangerAutoFire()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Ranger_AutoFire);
	SetAssetTags(AssetTags);
	
	ActivationPolicy = ENSAbilityActivationPolicy::WhileInputActive;
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
	
	constexpr float MinFireRate = 0.01f;
	constexpr float MinFireInterval = 0.01f;
	
	FinalFireRate = FMath::Max(FinalFireRate, MinFireRate);
	OutFireInterval = FMath::Max(1.0f / FinalFireRate, MinFireInterval);
	
	return true;
}

void UGA_RangerAutoFire::FireOnce()
{
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
	
	// Miss도 TargetData 흐름에 태우기 위해 Trace 정보를 채워둠
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

	// 원격 클라이언트만 아래 분기 실행
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
	
	if (bShouldExecuteCue)
	{
		ExecuteMuzzleFireCue();
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
	
	// 실제 소음은 서버에서만 처리
	ReportWeaponNoise(AvatarActor);
	
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

	if (!ClientHitResult || !ClientHitResult->bBlockingHit)
	{
		return;
	}

	FHitResult ServerHitResult;
	
	if (!ValidateTargetDataHitResult(*ClientHitResult, ServerHitResult))
	{
		return;
	}
	
	FHitResult MuzzleObstructionHitResult;
	
	if (IsMuzzleObstructed(ServerHitResult, MuzzleObstructionHitResult))
	{
		ExecuteImpactCue(MuzzleObstructionHitResult);
		return;
	}
	
	ApplyDamageToActor(ServerHitResult.GetActor());
	ExecuteImpactCue(ServerHitResult);
}

void UGA_RangerAutoFire::ApplyDamageToActor(AActor* TargetActor)
{
	if (!TargetActor || !DamageEffectClass)
	{
		return;
	}
	
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	
	if (!SourceASC || !TargetASC)
	{
		return;
	}
	
	float FinalDamage = 0.0f;
	
	if (!TryGetFinalDamage(FinalDamage))
	{
		return;
	}
	
	FGameplayEffectSpecHandle DamageSpecHandle = 
		MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	
	if (!DamageSpecHandle.IsValid() || !DamageSpecHandle.Data.IsValid())
	{
		return;
	}
	
	ApplyDamageSetByCaller(DamageSpecHandle, FinalDamage);
	
	// 데미지 감지 가해자 지정
	AssignDamageInstigator(DamageSpecHandle);
	
	// GE_Damage -> GEC_DamageExecution -> Damage Meta Attribute 흐름으로 데미지 전달
	SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
}

bool UGA_RangerAutoFire::TryGetFinalDamage(float& OutDamage)
{
	float FinalDamage = 0.0f;
	
	if (!TryGetFinalAbilityStat(
		NSGameplayTags::Ability_Ranger_AutoFire,
		NSGameplayTags::CombatStat_Damage,
		FinalDamage))
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Warning,
			"AutoFire Damage CombatStat 조회 실패. AbilityTag={AbilityTag}, StatTag={StatTag}",
			("AbilityTag", NSGameplayTags::Ability_Ranger_AutoFire.GetTag().ToString()),
			("StatTag", NSGameplayTags::CombatStat_Damage.GetTag().ToString())
		);
		
		return false;
	}
	
	OutDamage = FMath::Max(FinalDamage, 0.0f);
	
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

	OutTraceEnd = OutTraceStart + TraceDirection * TraceRange;
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);

	bOutHit = World->LineTraceSingleByChannel(
		OutHitResult,
		OutTraceStart,
		OutTraceEnd,
		TraceChannel,
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
	
	if (!LocalHitResult || !LocalHitResult->bBlockingHit)
	{
		return;
	}
	
	// 로컬 피드백용 ImpactCue이므로 실제 데미지 판정은 서버에서 다시 검증
	const FHitResult PredictedImpactHitResult = *LocalHitResult;
	FHitResult MuzzleObstructionHitResult;
	
	// 총구가 막힌 상황이면 조준 대상이 아니라 실제로 막힌 지점에 ImpactCue 표시
	if (IsMuzzleObstructed(PredictedImpactHitResult, MuzzleObstructionHitResult))
	{
		ExecuteImpactCue(MuzzleObstructionHitResult);
		return;
	}
	
	ExecuteImpactCue(PredictedImpactHitResult);
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
	const FHitResult& ServerHitResult, FHitResult& OutObstructionHitResult) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	
	if (!IsValid(AvatarActor) || !World)
	{
		return false;
	}
	
	AActor* TargetActor = ServerHitResult.GetActor();
	
	if (!IsValid(TargetActor))
	{
		return false;
	}
	
	FTransform MuzzleTransform;
	
	if (!TryGetAttackOriginTransform(MuzzleTransform))
	{
		return false;
	}
	
	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	const FVector AimPoint = ServerHitResult.ImpactPoint;
	
	const FVector ShotDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
	
	if (ShotDirection.IsNearlyZero())
	{
		return false;
	}
	
	const float BackTraceDistance = FMath::Max(MuzzleObstructionBackTraceDistance, 0.0f);
	const FVector ObstructionTraceStart = MuzzleLocation - ShotDirection * BackTraceDistance;
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);
	
	const bool bHit = World->LineTraceSingleByChannel(
		OutObstructionHitResult,
		ObstructionTraceStart,
		AimPoint,
		TraceChannel,
		QueryParams
	);
	
	const bool bIsObstructed =
		bHit && OutObstructionHitResult.bBlockingHit && OutObstructionHitResult.GetActor() != TargetActor;
	
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

bool UGA_RangerAutoFire::ValidateTargetDataHitResult(
	const FHitResult& ClientHitResult,
	FHitResult& OutServerHitResult) const
{
	if (!ClientHitResult.bBlockingHit || !IsValid(ClientHitResult.GetActor()))
	{
		return false;
	}
	
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	
	if (!AvatarActor || !World || !AvatarActor->HasAuthority())
	{
		return false;
	}
	
	const FVector TraceStart = ClientHitResult.TraceStart;
	const FVector TraceEnd = ClientHitResult.TraceEnd;
	
	if (TraceStart.Equals(TraceEnd))
	{
		return false;
	}
	
	const float MaxTraceDistance = TraceRange + ServerHitLocationTolerance;
	if (FVector::DistSquared(TraceStart, TraceEnd) > FMath::Square(MaxTraceDistance))
	{
		return false;
	}
	
	FVector ServerAimTraceStartLocation;
	
	if (!TryGetAimTraceStartLocation(ServerAimTraceStartLocation))
	{
		return false;
	}
	
	if (FVector::DistSquared(ServerAimTraceStartLocation, TraceStart) > FMath::Square(ServerTraceStartTolerance))
	{
		return false;
	}
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);
	
	const bool bServerHit = World->LineTraceSingleByChannel(
		OutServerHitResult,
		TraceStart,
		TraceEnd,
		TraceChannel,
		QueryParams
	);
	
	if (!bServerHit || !OutServerHitResult.bBlockingHit)
	{
		return false;
	}
	
	if (OutServerHitResult.GetActor() != ClientHitResult.GetActor())
	{
		return false;
	}
	
	if (FVector::DistSquared(OutServerHitResult.ImpactPoint, ClientHitResult.ImpactPoint)
		> FMath::Square(ServerHitLocationTolerance))
	{
		return false;
	}
	
	return true;
}

void UGA_RangerAutoFire::ReportWeaponNoise(const AActor* InAvatarActor)
{
	if (APawn* NoiseInstigator = Cast<APawn>(const_cast<AActor*>(InAvatarActor)))
    {
        NoiseInstigator->MakeNoise(1.0f, NoiseInstigator, InAvatarActor->GetActorLocation());
    }
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
