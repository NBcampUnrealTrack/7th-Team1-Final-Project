// Copyright 2026 One Team. All rights reserved.


#include "GA_Dash.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "NeoSanctum/UI/HUD/NSSkillCooldownMessage.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "Engine/GameInstance.h"
#include "VerseVM/VVMRuntimeError.h"

UGA_Dash::UGA_Dash()
{
	// 캐릭터마다 Ability 인스턴스를 하나씩 만든다는 의미
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 로컬에서 예측
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Common_Dash);
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dashing);
}

void UGA_Dash::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
	if (!MoveComp || DashDistance <= 0.0f || DashDuration <= 0.0f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 우선순위에 따른 대시 방향 설정
	FVector DashDirection = MoveComp->GetCurrentAcceleration().GetSafeNormal2D();
	if (DashDirection.IsNearlyZero())
	{
		// 최우선 순위 : 현재 캐릭터가 움직이고 있는 방향
		DashDirection = Character->GetLastMovementInputVector().GetSafeNormal2D();
	}
	if (DashDirection.IsNearlyZero())
	{
		// 캐릭터 전방 방향
		DashDirection = Character->GetActorForwardVector().GetSafeNormal2D();
	}
	if (DashDirection.IsNearlyZero())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (MoveComp->IsMovingOnGround() && MoveComp->CurrentFloor.IsWalkableFloor())
	{
		// 간단하게 바닥의 Normal 값을 구할 수 있는 방법
		const FVector FloorNormal = MoveComp->CurrentFloor.HitResult.ImpactNormal;
		// 경사가 MinFloorNormalZ에서 설정한 값보다 급격하다면 종료
		if (FloorNormal.Z < MinFloorNormalZ)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
		
		const FVector ProjectedDirection = FVector::VectorPlaneProject(DashDirection, FloorNormal).GetSafeNormal();
		if (!ProjectedDirection.IsNearlyZero())
		{
			DashDirection = ProjectedDirection;
		}
	}
	
	//DashCount를 소모하기 위해 Ability Cost를 적용한다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float CurrentDashCount =
		ASC->GetNumericAttribute(
			UNSPlayerAttributeSet::GetDashCountAttribute());

	const float MaxDashCount =
		ASC->GetNumericAttribute(
			UNSPlayerAttributeSet::GetMaxDashCountAttribute());

	const float DashRegenRate =
		ASC->GetNumericAttribute(
			UNSPlayerAttributeSet::GetDashRegenRateAttribute());

	const float DashCooldownDuration =
		DashRegenRate > 0.0f
			? 1.0f / DashRegenRate
			: 0.0f;

	if (DashCooldownDuration > 0.0f)
	{
		if (APlayerController* PlayerController =
			ActorInfo->PlayerController.Get())
		{
			if (PlayerController->IsLocalController())
			{
				if (UGameInstance* GameInstance =
					PlayerController->GetGameInstance())
				{
					if (UGameplayMessageSubsystem* MessageSubsystem =
						GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
					{
						FNSSkillCooldownMessage Message;
						Message.SkillTag = NSGameplayTags::Ability_Common_Dash;
						Message.CooldownDuration = DashCooldownDuration;
						Message.CurrentCharge = FMath::Clamp(
							FMath::FloorToInt(CurrentDashCount),
							0,
							FMath::FloorToInt(MaxDashCount));
						Message.MaxCharge =
							FMath::FloorToInt(MaxDashCount);

						MessageSubsystem->BroadcastMessage(
							TAG_Message_UI_SkillCooldown_Start,
							Message);
					}
				}
			}
		}
	}
	
	
	ASC->AddLooseGameplayTag(NSGameplayTags::State_Dashing);
	// 대쉬 속도 = 거리 / 지속시간
	const float DashSpeed = DashDistance / DashDuration;
	DashTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this,
		TEXT("Dash"),
		DashDirection,
		DashSpeed,
		DashDuration,
		false,
		nullptr,
		ERootMotionFinishVelocityMode::SetVelocity,
		FVector::ZeroVector,
		0.0f,
		bEnableGravityDuringDash);

	if (!DashTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	DashTask->OnFinish.AddDynamic(this, &ThisClass::OnDashFinished);
	
	// 실제로 실행되는 부분
	DashTask->ReadyForActivation();
	ASC->AddGameplayCue(NSGameplayTags::GameplayCue_Common_Dash);
}

void UGA_Dash::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled
)
{
	if (DashTask)
	{
		DashTask->EndTask();
		DashTask = nullptr;
	}
	
	// 임시 상태태그 제거
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(NSGameplayTags::State_Dashing);
		ASC->RemoveGameplayCue(NSGameplayTags::GameplayCue_Common_Dash);
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_Dash::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}
	
	const UNSPlayerAttributeSet* AttributeSet = ActorInfo->AbilitySystemComponent->GetSet<UNSPlayerAttributeSet>();
	
	if (!AttributeSet)
	{
		return false;
	}
	
	// DashCount AttributeSet이 1 이상이어야 true가 되어 Ability가 활성화 될 수 있음.
	return AttributeSet->GetDashCount() >= 1.f;
}

void UGA_Dash::OnDashFinished()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
