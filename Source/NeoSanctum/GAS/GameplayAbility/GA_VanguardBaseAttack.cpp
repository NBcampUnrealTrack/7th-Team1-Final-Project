// Copyright 2026 One Team. All rights reserved.

#include "GA_VanguardBaseAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/RootMotionSource.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NeoSanctum/Combat/NSDamageRules.h"
#include "NeoSanctum/Combat/Weapon/NSMeleeWeapon.h"
#include "NeoSanctum/Combat/Weapon/NSWeaponBase.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_VanguardBaseAttack::UGA_VanguardBaseAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Vanguard_BaseAttack);
	SetAssetTags(AssetTags);

	ActivationPolicy = ENSAbilityActivationPolicy::OnInputTriggered;
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
	// 대쉬 중 기본공격 선입력 방지
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dashing);
	// 한 사이클이 끝나기 전에 재실행 방지
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Vanguard_Attacking);
	// 대쉬공격 사이클이 끝나기 전에 재실행 방지
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Vanguard_ChargingDashAttack);
}

void UGA_VanguardBaseAttack::ActivateAbility(
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

	// 입력 시점 상태 기준 기본공격 파생 공격 결정
	ActiveAttackMode = SelectAttackMode(ActorInfo);

	if (ActiveAttackMode == ENSVanguardBaseAttackMode::None ||
		!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AddVanguardStateTags();
	StartMeleeHitEventTask();

	switch (ActiveAttackMode)
	{
	case ENSVanguardBaseAttackMode::GroundCombo:
		StartGroundCombo();
		break;
	case ENSVanguardBaseAttackMode::AirSlam:
		StartAirSlam();
		break;
	case ENSVanguardBaseAttackMode::DashCharge:
		StartDashCharge();
		break;
	default:
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		break;
	}
}

void UGA_VanguardBaseAttack::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (ActiveAttackMode == ENSVanguardBaseAttackMode::GroundCombo)
	{
		// 첫 공격 입력 해제 이후 콤보 입력 허용
		bGroundComboInitialInputReleased = true;
		return;
	}

	if (ActiveAttackMode == ENSVanguardBaseAttackMode::DashCharge)
	{
		FinishDashCharge();
	}
}

void UGA_VanguardBaseAttack::InputPressed(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// 활성화 이후 추가 입력만 지상 콤보 입력으로 처리
	if (ActiveAttackMode == ENSVanguardBaseAttackMode::GroundCombo)
	{
		HandleGroundComboInput();
	}
}

void UGA_VanguardBaseAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (DashAttackMoveTask)
	{
		DashAttackMoveTask->EndTask();
		DashAttackMoveTask = nullptr;
	}

	if (AirSlamHoverTask)
	{
		AirSlamHoverTask->EndTask();
		AirSlamHoverTask = nullptr;
	}
	
	if (AirSlamDiveTask)
	{
		AirSlamDiveTask->EndTask();
		AirSlamDiveTask = nullptr;
	}
	
	if (ActiveAttackMode == ENSVanguardBaseAttackMode::AirSlam)
	{
		RestoreAirSlamMovementMode();
	}
	
	RemoveAttackFlashGameplayCue();
	RemoveVanguardStateTags();
	ActiveAttackMode = ENSVanguardBaseAttackMode::None;
	DashChargeStartTime = 0.0;
	CurrentDashAttackChargeRatio = 0.0f;
	PreviousMeleeTraceSocketLocations.Reset();
	bHasPreviousMeleeTraceSocketLocations = false;
	CurrentMeleeTraceWindowId = 0;
	ComboWindowEventTask = nullptr;
	MeleeHitEventTask = nullptr;
	DashAttackRecoverEventTask = nullptr;
	CurrentGroundComboIndex = INDEX_NONE;
	bComboInputBuffered = false;
	bComboAdvancedInCurrentWindow = false;
	bGroundComboInitialInputReleased = false;
	bDashAttackMoveStarted = false;
	bDashAttackMoveFinished = false;
	bDashAttackMontageStarted = false;
	bDashAttackMontageFinished = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_VanguardBaseAttack::OnAttackMontageCompleted()
{
	if (ActiveAttackMode == ENSVanguardBaseAttackMode::DashAttack)
	{
		bDashAttackMontageFinished = true;
		TryEndDashAttack();
		return;
	}

	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		false);
}

void UGA_VanguardBaseAttack::OnAttackMontageInterrupted()
{
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		true);
}

void UGA_VanguardBaseAttack::OnDashAttackMoveFinished()
{
	DashAttackMoveTask = nullptr;
	bDashAttackMoveFinished = true;
	TryEndDashAttack();
}

void UGA_VanguardBaseAttack::OnAirSlamHoverFinished()
{
	if (AirSlamHoverTask)
	{
		AirSlamHoverTask->EndTask();
	}
	AirSlamHoverTask = nullptr;
	
	if (ActiveAttackMode != ENSVanguardBaseAttackMode::AirSlam)
	{
		return;
	}
	
	StartAirSlamDive();
}

void UGA_VanguardBaseAttack::OnAirSlamDiveFinished()
{
	if (AirSlamDiveTask)
	{
		AirSlamDiveTask->EndTask();
	}
	AirSlamDiveTask = nullptr;
	
	if (ActiveAttackMode != ENSVanguardBaseAttackMode::AirSlam)
	{
		return;
	}
	
	RestoreAirSlamMovementMode();
	StartAirSlamImpact();
}

void UGA_VanguardBaseAttack::OnMeleeHitEventReceived(FGameplayEventData Payload)
{
	HandleMeleeHitEvent(Payload);
}

void UGA_VanguardBaseAttack::OnComboWindowOpened(FGameplayEventData Payload)
{
	if (ActiveAttackMode != ENSVanguardBaseAttackMode::GroundCombo)
	{
		return;
	}

	// 새 Combo Window의 섹션 이동 가능 상태 초기화
	bComboAdvancedInCurrentWindow = false;

	if (bComboInputBuffered)
	{
		// Window 이전 선입력 소비
		TryAdvanceGroundCombo();
	}
}

void UGA_VanguardBaseAttack::OnDashAttackRecoverStarted(FGameplayEventData Payload)
{
	if (ActiveAttackMode != ENSVanguardBaseAttackMode::DashAttack)
	{
		return;
	}

	// Recover 진입 시 대쉬공격 연출 Cue 종료
	RemoveAttackFlashGameplayCue();
}

ENSVanguardBaseAttackMode UGA_VanguardBaseAttack::SelectAttackMode(
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return ENSVanguardBaseAttackMode::None;
	}

	const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	const UCharacterMovementComponent* MovementComponent =
		Character ? Character->GetCharacterMovement() : nullptr;

	// 공중에서는 지상 콤보보다 내려찍기 공격 우선
	if (MovementComponent && MovementComponent->IsFalling())
	{
		return ENSVanguardBaseAttackMode::AirSlam;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	// 대쉬 직후 입력 창 안에서는 대쉬 차지 공격으로 전환
	if (ASC && ASC->HasMatchingGameplayTag(NSGameplayTags::State_DashAttackWindow))
	{
		return ENSVanguardBaseAttackMode::DashCharge;
	}

	return ENSVanguardBaseAttackMode::GroundCombo;
}

void UGA_VanguardBaseAttack::StartGroundCombo()
{
	// 첫 콤보 단계와 입력 버퍼 초기화
	CurrentGroundComboIndex = 0;
	bComboInputBuffered = false;
	bComboAdvancedInCurrentWindow = false;
	bGroundComboInitialInputReleased = false;
	StartComboWindowEventTask();

	if (bLogVanguardAttackMode)
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Log,
			"뱅가드 콤보어택 모드 선택");
	}

	if (!PlayAttackMontageAndWait(GroundComboMontage, AttackMontagePlayRate))
	{
		FinishInstantMode();
	}
}

void UGA_VanguardBaseAttack::HandleGroundComboInput()
{
	if (!bGroundComboInitialInputReleased)
	{
		// Ability 활성화 입력 재사용 방지
		return;
	}

	if (CurrentGroundComboIndex == INDEX_NONE ||
		CurrentGroundComboIndex >= GroundComboSectionNames.Num() - 1)
	{
		return;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC &&
		ASC->HasMatchingGameplayTag(NSGameplayTags::State_Vanguard_ComboInputWindow) &&
		!bComboAdvancedInCurrentWindow)
	{
		// Window 안의 추가 입력 즉시 소비
		TryAdvanceGroundCombo();
		return;
	}

	// Window 이전 추가 입력 버퍼링
	bComboInputBuffered = true;
}

void UGA_VanguardBaseAttack::StartComboWindowEventTask()
{
	// 몽타주 Notify의 Combo Window Open 이벤트 대기
	ComboWindowEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		NSGameplayTags::Event_Vanguard_ComboWindowOpened,
		nullptr,
		false,
		true);

	if (!ComboWindowEventTask)
	{
		return;
	}

	ComboWindowEventTask->EventReceived.AddDynamic(this, &ThisClass::OnComboWindowOpened);
	ComboWindowEventTask->ReadyForActivation();
}

void UGA_VanguardBaseAttack::StartMeleeHitEventTask()
{
	// AnimNotifyState에서 보낼 Vanguard Hit 이벤트 대기
	MeleeHitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		NSGameplayTags::Event_Vanguard_Hit,
		nullptr,
		false,
		true);

	if (!MeleeHitEventTask)
	{
		return;
	}

	MeleeHitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnMeleeHitEventReceived);
	MeleeHitEventTask->ReadyForActivation();
}

void UGA_VanguardBaseAttack::HandleMeleeHitEvent(const FGameplayEventData& Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor || !AvatarActor->HasAuthority())
	{
		return;
	}

	// NotifyState 인스턴스가 바뀌면 새로운 공격 판정 구간으로 보고 이전 소켓 위치를 초기화
	const UObject* TraceWindow = Payload.OptionalObject.Get();
	const uint32 TraceWindowId = IsValid(TraceWindow) ? TraceWindow->GetUniqueID() : 0;
	if (TraceWindowId != 0 && TraceWindowId != CurrentMeleeTraceWindowId)
	{
		PreviousMeleeTraceSocketLocations.Reset();
		bHasPreviousMeleeTraceSocketLocations = false;
		CurrentMeleeTraceWindowId = TraceWindowId;
	}

	ANSMeleeWeapon* MeleeWeapon = GetCurrentMeleeWeapon();
	if (!MeleeWeapon)
	{
		return;
	}

	PerformMeleeSocketSweeps(*MeleeWeapon);
}

ANSMeleeWeapon* UGA_VanguardBaseAttack::GetCurrentMeleeWeapon() const
{
	const ANSPlayerCharacterBase* PlayerCharacter =
		Cast<ANSPlayerCharacterBase>(GetAvatarActorFromActorInfo());
	if (!PlayerCharacter)
	{
		return nullptr;
	}
	
	// Current Weapon 반환형이 WeaponBase라 캐스팅
	return Cast<ANSMeleeWeapon>(PlayerCharacter->GetCurrentWeapon());
}

void UGA_VanguardBaseAttack::PerformMeleeSocketSweeps(ANSMeleeWeapon& MeleeWeapon)
{
	TArray<FTransform> SocketTransforms;
	if (!MeleeWeapon.TryGetMeleeTraceSocketTransforms(SocketTransforms))
	{
		return;
	}

	if (SocketTransforms.IsEmpty())
	{
		return;
	}

	const bool bCanUsePreviousLocations =
		bHasPreviousMeleeTraceSocketLocations &&
		PreviousMeleeTraceSocketLocations.Num() == SocketTransforms.Num();

	// 직전 Tick 위치부터 현재 Tick 위치까지 Sweep해서 공격 궤적이 이어지도록 함
	for (int32 SocketIndex = 0; SocketIndex < SocketTransforms.Num(); ++SocketIndex)
	{
		const FVector CurrentLocation = SocketTransforms[SocketIndex].GetLocation();
		const FVector TraceStart = bCanUsePreviousLocations
			? PreviousMeleeTraceSocketLocations[SocketIndex]
			: CurrentLocation;

		SweepMeleeTrace(TraceStart, CurrentLocation);
	}

	PreviousMeleeTraceSocketLocations.Reset(SocketTransforms.Num());
	for (const FTransform& SocketTransform : SocketTransforms)
	{
		PreviousMeleeTraceSocketLocations.Add(SocketTransform.GetLocation());
	}

	bHasPreviousMeleeTraceSocketLocations = true;
}

void UGA_VanguardBaseAttack::SweepMeleeTrace(
	const FVector& TraceStart,
	const FVector& TraceEnd)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!AvatarActor || !World || MeleeTraceRadius <= 0.0f)
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NSVanguardMeleeTrace), false, AvatarActor);
	QueryParams.AddIgnoredActor(AvatarActor);

	if (const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(AvatarActor))
	{
		if (ANSWeaponBase* CurrentWeapon = PlayerCharacter->GetCurrentWeapon())
		{
			QueryParams.AddIgnoredActor(CurrentWeapon);
		}
	}

	const FVector SweepEnd = TraceStart.Equals(TraceEnd)
		? TraceEnd + FVector::UpVector * 0.1f
		: TraceEnd;

	// 무기 소켓 궤적을 구체 Sweep으로 검사.
	TArray<FHitResult> HitResults;
	const bool bHit = World->SweepMultiByChannel(
		HitResults,
		TraceStart,
		SweepEnd,
		FQuat::Identity,
		NSCollisionChannels::PlayerWeaponTrace,
		FCollisionShape::MakeSphere(MeleeTraceRadius),
		QueryParams);

	if (bDrawMeleeTraceDebug)
	{
		const FHitResult DebugHitResult = bHit && HitResults.Num() > 0 ? HitResults[0] : FHitResult();
		DrawMeleeTraceDebug(TraceStart, SweepEnd, bHit, DebugHitResult);
	}

	if (!bHit)
	{
		return;
	}

	for (const FHitResult& HitResult : HitResults)
	{
		ApplyDamageToActor(HitResult);
	}
}

void UGA_VanguardBaseAttack::ApplyDamageToActor(const FHitResult& HitResult)
{
	AActor* TargetActor = HitResult.GetActor();
	if (!TargetActor || !DamageEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

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

	// Attribute를 직접 변경하지 않고 GameplayEffect Spec으로 데미지를 전달
	// Damage 값은 SetByCaller로 설정하고, 대상 ASC에 서버 권한으로 적용
	FGameplayEffectSpecHandle DamageSpecHandle =
		MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());

	if (!DamageSpecHandle.IsValid() || !DamageSpecHandle.Data.IsValid())
	{
		return;
	}

	ApplyDamageSetByCaller(DamageSpecHandle, FinalDamage);
	DamageSpecHandle.Data->GetContext().AddHitResult(HitResult, true);

	// 데미지 스펙에서 데미지를 주는 Actor를 명시적으로 지정
	AssignDamageInstigator(DamageSpecHandle);

	// GE_InstantDamage -> GEC_DamageExecution -> Damage Meta Attribute 흐름으로 데미지 전달
	SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
}

bool UGA_VanguardBaseAttack::TryGetFinalDamage(float& OutDamage) const
{
	if (!TryGetFinalSkillDamage(NSGameplayTags::Ability_Vanguard_BaseAttack, OutDamage))
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Warning,
			"VanguardBaseAttack 데미지 계산 실패. AbilityTag={AbilityTag}",
			("AbilityTag", NSGameplayTags::Ability_Vanguard_BaseAttack.GetTag().ToString())
		);

		return false;
	}

	OutDamage = FMath::Max(OutDamage * GetCurrentAttackDamageMultiplier(), 0.0f);
	return true;
}

float UGA_VanguardBaseAttack::GetCurrentAttackDamageMultiplier() const
{
	switch (ActiveAttackMode)
	{
	case ENSVanguardBaseAttackMode::GroundCombo:
		return GroundComboDamageMultipliers.IsValidIndex(CurrentGroundComboIndex)
			? GroundComboDamageMultipliers[CurrentGroundComboIndex]
			: 1.0f;
	case ENSVanguardBaseAttackMode::AirSlam:
		return AirSlamDamageMultiplier;
	case ENSVanguardBaseAttackMode::DashAttack:
		return FMath::Lerp(
			DashAttackMinDamageMultiplier,
			DashAttackMaxDamageMultiplier,
			FMath::Clamp(CurrentDashAttackChargeRatio, 0.0f, 1.0f));
	default:
		return 1.0f;
	}
}

void UGA_VanguardBaseAttack::ApplyDamageSetByCaller(
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

void UGA_VanguardBaseAttack::AssignDamageInstigator(FGameplayEffectSpecHandle& InSpecHandle)
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

void UGA_VanguardBaseAttack::DrawMeleeTraceDebug(
	const FVector& TraceStart,
	const FVector& TraceEnd,
	bool bHit,
	const FHitResult& HitResult) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	// 디버그 : 히트되면 빨간색
	const FColor TraceColor = bHit ? FColor::Red : FColor::Green;
	
	const FVector TraceDelta = TraceEnd - TraceStart;
	const float TraceLength = TraceDelta.Size();
	
	if (TraceLength <= KINDA_SMALL_NUMBER)
	{
		DrawDebugSphere(
			World,
			TraceEnd,
			MeleeTraceRadius,
			12,
			TraceColor,
			false,
			MeleeTraceDebugDuration);
	}
	else
	{
		// 구체 Sweep이 지나간 부피를 캡슐 형태로 표시
		const FVector CapsuleCenter = (TraceStart + TraceEnd) * 0.5f;
		const float CapsuleHalfHeight = TraceLength * 0.5f + MeleeTraceRadius;
		const FQuat CapsuleRotation = FRotationMatrix::MakeFromZ(TraceDelta).ToQuat();

		DrawDebugCapsule(
			World,
			CapsuleCenter,
			CapsuleHalfHeight,
			MeleeTraceRadius,
			CapsuleRotation,
			TraceColor,
			false,
			MeleeTraceDebugDuration);
	}

	if (bHit)
	{
		DrawDebugPoint(
			World,
			HitResult.ImpactPoint,
			12.0f,
			FColor::Yellow,
			false,
			MeleeTraceDebugDuration);
	}
}

bool UGA_VanguardBaseAttack::TryAdvanceGroundCombo()
{
	if (CurrentGroundComboIndex == INDEX_NONE ||
		CurrentGroundComboIndex >= GroundComboSectionNames.Num() - 1 ||
		!GroundComboMontage)
	{
		return false;
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr;
	if (!AnimInstance || !AnimInstance->Montage_IsPlaying(GroundComboMontage))
	{
		return false;
	}

	const int32 NextComboIndex = CurrentGroundComboIndex + 1;
	const FName NextSectionName = GroundComboSectionNames.IsValidIndex(NextComboIndex)
		? GroundComboSectionNames[NextComboIndex]
		: NAME_None;
	if (NextSectionName.IsNone())
	{
		return false;
	}

	// 다음 콤보 단계 기록 후 몽타주 섹션 이동
	CurrentGroundComboIndex = NextComboIndex;
	bComboInputBuffered = false;
	bComboAdvancedInCurrentWindow = true;
	AnimInstance->Montage_JumpToSection(NextSectionName, GroundComboMontage);

	return true;
}

void UGA_VanguardBaseAttack::StartAirSlam()
{
	if (bLogVanguardAttackMode)
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Log,
			"뱅가드 공중 공격 모드 선택");
	}

	if (!PlayAttackMontageAndWait(AirSlamMontage, AttackMontagePlayRate))
	{
		FinishInstantMode();
		return;
	}
	
	StartAirSlamHover();
}

void UGA_VanguardBaseAttack::StartAirSlamHover()
{
	if (!JumpToAirSlamSection(AirSlamHoverSectionName))
	{
		FinishInstantMode();
		return;
	}
	
	if (AirSlamHoverDuration <= 0.0f)
	{
		StartAirSlamDive();
		return;
	}
	
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		FinishInstantMode();
		return;
	}
	
	// 현재 위치에서 짧은 체공
	AirSlamHoverTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this,
		TEXT("VanguardAirSlamHover"),
		Character->GetActorLocation(),
		AirSlamHoverDuration,
		true,
		MOVE_Flying,
		true,
		nullptr,
		ERootMotionFinishVelocityMode::SetVelocity,
		FVector::ZeroVector,
		0.0f);
	
	if (!AirSlamHoverTask)
	{
		StartAirSlamDive();
		return;
	}
	
	AirSlamHoverTask->OnTimedOut.AddDynamic(this, &ThisClass::OnAirSlamHoverFinished);
	AirSlamHoverTask->OnTimedOutAndDestinationReached.AddDynamic(this, &ThisClass::OnAirSlamHoverFinished);
	AirSlamHoverTask->ReadyForActivation();
}

void UGA_VanguardBaseAttack::StartAirSlamDive()
{
	if (!JumpToAirSlamSection(AirSlamDiveSectionName))
	{
		FinishInstantMode();
		return;
	}
	
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		FinishInstantMode();
		return;
	}
	
	FVector TargetLocation = FVector::ZeroVector;
	if (!TryGetAirSlamTargetLocation(TargetLocation))
	{
		FinishInstantMode();
		return;
	}
	
	const float DiveDistance = FVector::Dist(Character->GetActorLocation(), TargetLocation);
	const float DiveDuration = FMath::Max(DiveDistance / FMath::Max(AirSlamDiveSpeed, 0.01f), 0.01f);
	
	// 지면 목표 지점까지 빠른 낙하 이동
	AirSlamDiveTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this,
		TEXT("VanguardAirSlamDive"),
		TargetLocation,
		DiveDuration,
		true,
		MOVE_Flying,
		true,
		nullptr,
		ERootMotionFinishVelocityMode::SetVelocity,
		FVector::ZeroVector,
		0.0f);
	
	if (!AirSlamDiveTask)
	{
		StartAirSlamImpact();
		return;
	}
	
	AirSlamDiveTask->OnTimedOut.AddDynamic(this, &ThisClass::OnAirSlamDiveFinished);
	AirSlamDiveTask->OnTimedOutAndDestinationReached.AddDynamic(this, &ThisClass::OnAirSlamDiveFinished);
	AirSlamDiveTask->ReadyForActivation();
}

void UGA_VanguardBaseAttack::StartAirSlamImpact()
{
	if (!JumpToAirSlamSection(AirSlamImpactSectionName))
	{
		FinishInstantMode();
	}
}

bool UGA_VanguardBaseAttack::JumpToAirSlamSection(FName SectionName) const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr;
	if (!AnimInstance || !AirSlamMontage || SectionName.IsNone())
	{
		return false;
	}
	
	if (!AnimInstance->Montage_IsPlaying(AirSlamMontage))
	{
		return false;
	}
	
	AnimInstance->Montage_JumpToSection(SectionName, AirSlamMontage);
	return true;
}

bool UGA_VanguardBaseAttack::TryGetAirSlamTargetLocation(FVector& OutTargetLocation) const
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return false;
	}
	
	const UWorld* World = Character->GetWorld();
	if (!World)
	{
		return false;
	}
	
	const FVector TraceStart = Character->GetActorLocation();
	const FVector TraceEnd = TraceStart - FVector::UpVector * AirSlamGroundTraceDistance;
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NSVanguardAirSlamGroundTrace), false, Character);
	if (const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(Character))
	{
		if (ANSWeaponBase* CurrentWeapon = PlayerCharacter->GetCurrentWeapon())
		{
			QueryParams.AddIgnoredActor(CurrentWeapon);
		}
	}
	
	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams);
	
	if (!bHit)
	{
		return false;
	}
	
	OutTargetLocation = HitResult.ImpactPoint + FVector::UpVector * AirSlamImpactGroundOffset;
	return true;
}

void UGA_VanguardBaseAttack::RestoreAirSlamMovementMode() const
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return;
	}
	
	UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}
	
	// 공중 내려찍기 RootMotion 종료 후 잔여 속도 제거
	MovementComponent->StopMovementImmediately();
	
	const EMovementMode RestoreMode = MovementComponent->IsMovingOnGround()
		? MOVE_Walking
		: MOVE_Falling;
	MovementComponent->SetMovementMode(RestoreMode);
}

void UGA_VanguardBaseAttack::StartDashCharge()
{
	// 대쉬 차지 진입 시 대쉬공격 입력 창 소비
	ConsumeDashAttackWindow();

	if (bLogVanguardAttackMode)
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Log,
			"뱅가드 대쉬공격 차지 시작");
	}

	// 릴리즈 시 차지 비율 계산 기준 시각
	DashChargeStartTime = FPlatformTime::Seconds();

	if (!PlayAttackMontageAndWait(DashChargeMontage, AttackMontagePlayRate))
	{
		FinishInstantMode();
	}
}

void UGA_VanguardBaseAttack::FinishDashCharge()
{
	if (ActiveAttackMode != ENSVanguardBaseAttackMode::DashCharge)
	{
		return;
	}

	// 현재는 로그 확인용 계산. 후속 단계에서 거리/데미지 배율에 사용
	const float ChargeElapsedTime = GetDashChargeElapsedTime();
	const float FinalMaxDashChargeTime = GetFinalDashChargeTime();
	const float ChargeRatio = FMath::Clamp(
		ChargeElapsedTime / FMath::Max(FinalMaxDashChargeTime, 0.01f),
		0.0f,
		1.0f);

	if (bLogVanguardAttackMode)
	{
		NS_ACTOR_LOG(GetAvatarActorFromActorInfo(), LogNSGAS, Log,
			"대쉬공격 차징 끝. 차징 시간 = {ChargeElapsedTime}, 차징 퍼센트 = {ChargeRatio}",
			("ChargeElapsedTime", ChargeElapsedTime),
			("ChargeRatio", ChargeRatio));
	}

	StartDashAttack(ChargeRatio);
}

void UGA_VanguardBaseAttack::StartDashAttack(float ChargeRatio)
{
	// 대쉬공격 실행 상태로 전환
	ActiveAttackMode = ENSVanguardBaseAttackMode::DashAttack;
	CurrentDashAttackChargeRatio = FMath::Clamp(ChargeRatio, 0.0f, 1.0f);
	bDashAttackMoveStarted = false;
	bDashAttackMoveFinished = false;
	bDashAttackMontageStarted = false;
	bDashAttackMontageFinished = false;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// 차징 종료 상태태그 제거
		ASC->RemoveLooseGameplayTag(NSGameplayTags::State_Vanguard_ChargingDashAttack);
	}

	// 차지 비율에 따른 전방 돌진 시작
	bDashAttackMoveStarted = StartDashAttackMovement(ChargeRatio);
	bDashAttackMoveFinished = !bDashAttackMoveStarted;

	StartDashAttackRecoverEventTask();

	// 몽타주의 공격 Section으로 이동
	bDashAttackMontageStarted = JumpToDashAttackSection();
	bDashAttackMontageFinished = !bDashAttackMontageStarted;
	if (bDashAttackMontageStarted)
	{
		AddAttackFlashGameplayCue();
	}

	if (!bDashAttackMoveStarted && !bDashAttackMontageStarted)
	{
		EndAbility(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfo(),
			true,
			false);
	}
}

bool UGA_VanguardBaseAttack::JumpToDashAttackSection()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr;
	if (!AnimInstance || !DashChargeMontage || DashAttackSectionName.IsNone())
	{
		return false;
	}

	if (!AnimInstance->Montage_IsPlaying(DashChargeMontage))
	{
		return false;
	}

	// Charge Loop에서 Attack Section으로 즉시 분기
	AnimInstance->Montage_JumpToSection(DashAttackSectionName, DashChargeMontage);
	return true;
}

bool UGA_VanguardBaseAttack::StartDashAttackMovement(float ChargeRatio)
{
	float FinalMinDistance = 0.0f;
	float FinalMaxDistance = 0.0f;
	float FinalDuration = 0.0f;
	if (!TryResolveDashAttackMovementStats(FinalMinDistance, FinalMaxDistance, FinalDuration))
	{
		return false;
	}

	FVector DashAttackDirection = FVector::ZeroVector;
	if (!TryGetDashAttackDirection(DashAttackDirection))
	{
		return false;
	}

	const float ClampedChargeRatio = FMath::Clamp(ChargeRatio, 0.0f, 1.0f);
	const float DashAttackDistance = FMath::Lerp(FinalMinDistance, FinalMaxDistance, ClampedChargeRatio);
	const float DashAttackSpeed = DashAttackDistance / FMath::Max(FinalDuration, 0.01f);

	// GameplayAbility RootMotionSource 기반 강제 이동
	DashAttackMoveTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this,
		TEXT("VanguardDashAttack"),
		DashAttackDirection,
		DashAttackSpeed,
		FinalDuration,
		false,
		nullptr,
		ERootMotionFinishVelocityMode::SetVelocity,
		FVector::ZeroVector,
		0.0f,
		bEnableGravityDuringDashAttack);

	if (!DashAttackMoveTask)
	{
		return false;
	}

	DashAttackMoveTask->OnFinish.AddDynamic(this, &ThisClass::OnDashAttackMoveFinished);
	DashAttackMoveTask->ReadyForActivation();
	return true;
}

void UGA_VanguardBaseAttack::TryEndDashAttack()
{
	if (ActiveAttackMode != ENSVanguardBaseAttackMode::DashAttack)
	{
		return;
	}

	const bool bMoveFinished = !bDashAttackMoveStarted || bDashAttackMoveFinished;
	const bool bMontageFinished = !bDashAttackMontageStarted || bDashAttackMontageFinished;
	if (!bMoveFinished || !bMontageFinished)
	{
		return;
	}

	// 몽타주와 돌진 이동이 모두 끝난 뒤 Ability 종료
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		false);
}

void UGA_VanguardBaseAttack::StartDashAttackRecoverEventTask()
{
	// 대쉬공격 Recover 시작 Notify 이벤트 대기
	DashAttackRecoverEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		NSGameplayTags::Event_Vanguard_DashAttackRecoverStarted,
		nullptr,
		true,
		true);

	if (!DashAttackRecoverEventTask)
	{
		return;
	}

	DashAttackRecoverEventTask->EventReceived.AddDynamic(this, &ThisClass::OnDashAttackRecoverStarted);
	DashAttackRecoverEventTask->ReadyForActivation();
}

void UGA_VanguardBaseAttack::AddAttackFlashGameplayCue()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// 대쉬공격 연출 Cue 시작
		ASC->AddGameplayCue(NSGameplayTags::GameplayCue_Vanguard_BaseAttack_Flash);
	}
}

void UGA_VanguardBaseAttack::RemoveAttackFlashGameplayCue()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// 대쉬공격 연출 Cue 종료
		ASC->RemoveGameplayCue(NSGameplayTags::GameplayCue_Vanguard_BaseAttack_Flash);
	}
}

bool UGA_VanguardBaseAttack::TryGetDashAttackDirection(FVector& OutDirection) const
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return false;
	}

	FVector AimTarget = FVector::ZeroVector;
	if (TryGetCrosshairTarget(AimTarget))
	{
		// 화면 중앙 조준점 기준 수평 돌진 방향 계산
		OutDirection = (AimTarget - Character->GetActorLocation()).GetSafeNormal2D();
		if (!OutDirection.IsNearlyZero())
		{
			return true;
		}
	}

	OutDirection = Character->GetBaseAimRotation().Vector().GetSafeNormal2D();
	if (OutDirection.IsNearlyZero())
	{
		OutDirection = Character->GetActorForwardVector().GetSafeNormal2D();
	}

	return !OutDirection.IsNearlyZero();
}

bool UGA_VanguardBaseAttack::TryGetCrosshairTarget(FVector& OutTarget) const
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return false;
	}

	const UWorld* World = Character->GetWorld();
	if (!World)
	{
		return false;
	}

	const APawn* OwnerPawn = Cast<APawn>(Character);
	const APlayerController* PlayerController = OwnerPawn
		? Cast<APlayerController>(OwnerPawn->GetController())
		: nullptr;

	if (PlayerController && PlayerController->IsLocalController())
	{
		int32 ViewportSizeX = 0;
		int32 ViewportSizeY = 0;
		PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);

		FVector ScreenWorldLocation = FVector::ZeroVector;
		FVector ScreenWorldDirection = FVector::ForwardVector;
		if (ViewportSizeX > 0 && ViewportSizeY > 0 &&
			PlayerController->DeprojectScreenPositionToWorld(
				ViewportSizeX * 0.5f,
				ViewportSizeY * 0.5f,
				ScreenWorldLocation,
				ScreenWorldDirection))
		{
			FVector TraceStart = ScreenWorldLocation;
			if (const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(Character))
			{
				PlayerCharacter->TryGetAimTraceStartLocation(TraceStart);
			}

			const FVector TraceEnd = TraceStart + ScreenWorldDirection.GetSafeNormal() * DashAttackAimTraceRange;
			FHitResult HitResult;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NSVanguardDashAttackAimTrace), false, Character);
			if (const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(Character))
			{
				if (ANSWeaponBase* CurrentWeapon = PlayerCharacter->GetCurrentWeapon())
				{
					QueryParams.AddIgnoredActor(CurrentWeapon);
				}
			}

			const bool bHit = World->LineTraceSingleByChannel(
				HitResult,
				TraceStart,
				TraceEnd,
				NSCollisionChannels::PlayerWeaponTrace,
				QueryParams);

			OutTarget = bHit ? HitResult.ImpactPoint : TraceEnd;
			return true;
		}
	}

	OutTarget = Character->GetActorLocation() + Character->GetBaseAimRotation().Vector() * DashAttackAimTraceRange;
	return true;
}

void UGA_VanguardBaseAttack::FinishInstantMode()
{
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		false);
}

bool UGA_VanguardBaseAttack::PlayAttackMontageAndWait(UAnimMontage* Montage, float PlayRate)
{
	if (!Montage)
	{
		return false;
	}

	const float FinalPlayRate = FMath::Max(PlayRate, 0.01f);

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			Montage,
			FinalPlayRate,
			NAME_None,
			true);

	if (!MontageTask)
	{
		return false;
	}

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnAttackMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnAttackMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnAttackMontageInterrupted);
	MontageTask->ReadyForActivation();

	return true;
}

void UGA_VanguardBaseAttack::AddVanguardStateTags()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// 다른 Vanguard 기본공격 재진입 방지용 공통 공격 상태태그
	ASC->AddLooseGameplayTag(NSGameplayTags::State_Vanguard_Attacking);

	if (ActiveAttackMode == ENSVanguardBaseAttackMode::DashCharge)
	{
		// 차지 중 애니메이션/이펙트 분기용 별도 상태태그
		ASC->AddLooseGameplayTag(NSGameplayTags::State_Vanguard_ChargingDashAttack);
	}
}

void UGA_VanguardBaseAttack::RemoveVanguardStateTags()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	ASC->RemoveLooseGameplayTag(NSGameplayTags::State_Vanguard_Attacking);
	ASC->RemoveLooseGameplayTag(NSGameplayTags::State_Vanguard_ChargingDashAttack);
}

void UGA_VanguardBaseAttack::ConsumeDashAttackWindow()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->SetLooseGameplayTagCount(NSGameplayTags::State_DashAttackWindow, 0);
	}
}

float UGA_VanguardBaseAttack::GetDashChargeElapsedTime() const
{
	if (DashChargeStartTime <= 0.0)
	{
		return 0.0f;
	}

	return static_cast<float>(FPlatformTime::Seconds() - DashChargeStartTime);
}

float UGA_VanguardBaseAttack::GetVanguardFinalStatOrDefault(
	const FGameplayTag& StatTag,
	float DefaultValue) const
{
	return GetFinalAbilityStatOrDefault(
		NSGameplayTags::Ability_Vanguard_BaseAttack,
		StatTag,
		DefaultValue);
}

float UGA_VanguardBaseAttack::GetFinalDashChargeTime() const
{
	const float FinalChargingTime = GetVanguardFinalStatOrDefault(
		NSGameplayTags::CombatStat_ChargingTime,
		MaxDashChargeTime);

	return FMath::Max(FinalChargingTime, 0.01f);
}

bool UGA_VanguardBaseAttack::TryResolveDashAttackMovementStats(
	float& OutMinDistance,
	float& OutMaxDistance,
	float& OutDuration) const
{
	OutMinDistance = FMath::Max(
		GetVanguardFinalStatOrDefault(
			NSGameplayTags::CombatStat_MinSkillRange,
			DashAttackMinDistance),
		0.0f);

	OutMaxDistance = FMath::Max(
		GetVanguardFinalStatOrDefault(
			NSGameplayTags::CombatStat_SkillRange,
			DashAttackMaxDistance),
		0.0f);

	OutDuration = FMath::Max(
		GetVanguardFinalStatOrDefault(
			NSGameplayTags::CombatStat_Duration,
			DashAttackDuration),
		0.01f);

	if (OutMinDistance > OutMaxDistance)
	{
		Swap(OutMinDistance, OutMaxDistance);
	}

	return OutMaxDistance > 0.0f;
}
