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

	// 한 번 활성화될 때 한 발만 발사
	FireOnce();

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
	
	// Miss도 TargetData 흐름에 태우기 위해 Trace 정보를 채워둠
	if (!bHit)
	{
		HitResult.TraceStart = TraceStart;
		HitResult.TraceEnd = TraceEnd;
		HitResult.Location = TraceEnd;
		HitResult.ImpactPoint = TraceEnd;
	}
	
	const FGameplayAbilityTargetDataHandle TargetDataHandle = MakeTargetDataFromHitResult(HitResult);
	OnRangerTargetDataReady(TargetDataHandle);
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

void UGA_RangerAutoFire::OnRangerTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	// 조작 중인 클라이언트는 즉시 발사 피드백
	if (ShouldPlayLocalFeedback())
	{
		PlayFireFeedback();
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

	const FHitResult* HitResult = TargetData->GetHitResult();

	if (!HitResult || !HitResult->bBlockingHit)
	{
		return;
	}

	ApplyDamageToActor(HitResult->GetActor());
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

void UGA_RangerAutoFire::PlayFireFeedback()
{
	ExecuteMuzzleFireCue();

	if (bDrawDebugHitscan)
	{
		DrawDebugHitscan();
	}
}

void UGA_RangerAutoFire::DrawDebugHitscan()
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	FHitResult HitResult;
	FVector TraceStart;
	FVector TraceEnd;
	bool bHit = false;

	if (!TryBuildHitscanTrace(HitResult, TraceStart, TraceEnd, bHit))
	{
		return;
	}

	const FVector DebugEnd = bHit ? HitResult.ImpactPoint : TraceEnd;
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
			HitResult.ImpactPoint,
			12.0f,
			FColor::Red,
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
		// 소캣을 못 찾으면 캐릭터 전방 위치로 임시 처리
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
