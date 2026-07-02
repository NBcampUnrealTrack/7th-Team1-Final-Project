// Copyright 2026 One Team. All rights reserved.

#include "GA_VanguardBaseAttack.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
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
	RemoveVanguardStateTags();
	ActiveAttackMode = ENSVanguardBaseAttackMode::None;
	DashChargeStartTime = 0.0;
	ComboWindowEventTask = nullptr;
	CurrentGroundComboIndex = INDEX_NONE;
	bComboInputBuffered = false;
	bComboAdvancedInCurrentWindow = false;
	bGroundComboInitialInputReleased = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_VanguardBaseAttack::OnAttackMontageCompleted()
{
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
	}
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

	PlayAttackMontageOnly(DashChargeMontage, AttackMontagePlayRate);
}

void UGA_VanguardBaseAttack::FinishDashCharge()
{
	if (ActiveAttackMode != ENSVanguardBaseAttackMode::DashCharge)
	{
		return;
	}

	// 현재는 로그 확인용 계산. 후속 단계에서 거리/데미지 배율에 사용
	const float ChargeElapsedTime = GetDashChargeElapsedTime();
	const float ChargeRatio = FMath::Clamp(
		ChargeElapsedTime / FMath::Max(MaxDashChargeTime, 0.01f),
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
	ActiveAttackMode = ENSVanguardBaseAttackMode::DashAttack;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(NSGameplayTags::State_Vanguard_ChargingDashAttack);
	}

	if (!PlayAttackMontageAndWait(DashAttackMontage, AttackMontagePlayRate))
	{
		EndAbility(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfo(),
			true,
			false);
	}
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

bool UGA_VanguardBaseAttack::PlayAttackMontageOnly(UAnimMontage* Montage, float PlayRate)
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
