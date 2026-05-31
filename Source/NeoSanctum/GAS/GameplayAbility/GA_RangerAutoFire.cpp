// Copyright 2026 One Team. All rights reserved.


#include "GA_RangerAutoFire.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Combat/Weapon/NSWeaponBase.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"

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
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		// TODO: 탄약 관련기능이 생기면 0발 때는 재장전 GA에게 이벤트 전달
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (bLogPredictionKey)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[레인저 연사][활성화] 로컬조작:%d 서버권한:%d 예측키:%s"),
			ActorInfo->IsLocallyControlled(),
			ActorInfo->IsNetAuthority(),
			*GetCurrentPredictionKeyStatus()
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
		if (bLogPredictionKey)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[레인저 연사][타겟데이터 대기] 서버가 클라이언트 TargetData를 기다리는 중 / 예측키:%s"),
				*GetCurrentPredictionKeyStatus()
			);
		}

		// TargetData가 델리게이트 등록보다 먼저 도착한 경우 처리
		// 바로 OnTargetDataReadyCallback 실행
		ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationInfo.GetActivationPredictionKey());
	}
	else
	{
		if (bLogPredictionKey)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[레인저 연사][타겟데이터 생성] 로컬에서 TargetData 생성 중 / 예측키:%s"),
				*GetCurrentPredictionKeyStatus()
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

	const float DelayTime = FMath::Max(FireInterval, 0.01f);

	// FireInterval 동안 Ability를 Active 상태로 유지해 연사 속도를 제한
	World->GetTimerManager().SetTimer(
		FireDelayTimerHandle,
		this,
		&ThisClass::FinishFireCycle,
		DelayTime,
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

void UGA_RangerAutoFire::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	// 입력을 떼면 발사 사이클 즉시 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_RangerAutoFire::FinishFireCycle()
{
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		false
	);
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

	if (bLogPredictionKey)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[레인저 연사][타겟데이터 준비] 로컬조작:%d 서버권한:%d 데이터개수:%d 예측키:%s"),
			ActorInfo ? ActorInfo->IsLocallyControlled() : false,
			ActorInfo ? ActorInfo->IsNetAuthority() : false,
			TargetDataHandle.Num(),
			*GetCurrentPredictionKeyStatus()
		);
	}

	const bool bShouldNotifyServer =
		ActorInfo && ActorInfo->IsLocallyControlled() && !ActorInfo->IsNetAuthority();

	// 원격 클라이언트만 아래 분기 실행
	if (bShouldNotifyServer)
	{
		if (bLogPredictionKey)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[레인저 연사][타겟데이터 전송] 클라이언트가 서버로 TargetData 전송 / ScopedPredictionKey유효:%d 예측키:%s"),
				ASC->ScopedPredictionKey.IsValidKey(),
				*GetCurrentPredictionKeyStatus()
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
	
	if (!AvatarActor || !AvatarActor->HasAuthority())
	{
		return;
	}
	
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
	
	ApplyDamageToActor(ServerHitResult.GetActor());
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
	
	FGameplayEffectSpecHandle DamageSpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	
	if (!DamageSpecHandle.IsValid())
	{
		return;
	}
	
	// GE_Damage -> GEC_DamageExecution -> Damage Meta Attribute 흐름으로 데미지 전달
	SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
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
		
		// 초록: 허공, 빨강: 명중
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
				HitResult->ImpactPoint,
				12.0f,
				FColor::Red,
				false,
				DebugLineDuration
			);
		}
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

	if (!AvatarActor)
	{
		return false;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	const APawn* Pawn = Cast<APawn>(AvatarActor);
	const AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	OutTraceStart = Pawn ?
		Pawn->GetPawnViewLocation() : AvatarActor->GetActorLocation();

	const FRotator AimRotation = Controller	?
		Controller->GetControlRotation() : AvatarActor->GetActorRotation();

	const FVector TraceDirection = AimRotation.Vector();
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

	ASC->ExecuteGameplayCue(NSGameplayTags::GameplayCue_Ranger_MuzzleFire, CueParameters);
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
	
	const APawn* Pawn = Cast<APawn>(AvatarActor);
	const FVector ServerViewLocation = Pawn ? Pawn->GetPawnViewLocation() : AvatarActor->GetActorLocation();
	
	if (FVector::DistSquared(ServerViewLocation, TraceStart) > FMath::Square(ServerTraceStartTolerance))
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
