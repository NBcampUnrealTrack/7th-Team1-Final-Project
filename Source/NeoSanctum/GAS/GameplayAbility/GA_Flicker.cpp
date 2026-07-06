// Copyright 2026 One Team. All rights reserved.

#include "GA_Flicker.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Combat/NSDamageRules.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_Flicker::UGA_Flicker()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Vanguard_Flicker);
	SetAssetTags(AssetTags);

	SkillAbilityTag = NSGameplayTags::Ability_Vanguard_Flicker;
	ActivationPolicy = ENSAbilityActivationPolicy::OnInputTriggered;
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dashing);
}

void UGA_Flicker::ActivateAbility(
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

	// 발동 전 유효 타겟 탐색
	AActor* TargetActor = nullptr;
	FVector TargetLocation = FVector::ZeroVector;
	if (!TryFindBestTarget(TargetActor, TargetLocation))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 타겟이 확정된 뒤 Cost/Cooldown Commit
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 타겟과 겹치지 않는 공격 위치 계산
	FVector AttackLocation = FVector::ZeroVector;
	if (!TryBuildAttackLocation(TargetActor, TargetLocation, AttackLocation))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentTarget = TargetActor;
	CurrentTargetLocation = TargetLocation;

	AddDashingState();

	// 이동 시작 실패 시 즉시 공격 처리
	if (!StartFlickerMove(AttackLocation))
	{
		ApplyDamageToTarget();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_Flicker::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (MoveTask)
	{
		MoveTask->EndTask();
		MoveTask = nullptr;
	}

	RestoreMovementMode();
	RemoveDashingState();

	CurrentTarget.Reset();
	CurrentTargetLocation = FVector::ZeroVector;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Flicker::OnFlickerMoveFinished()
{
	MoveTask = nullptr;
	ApplyDamageToTarget();

	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		false);
}

bool UGA_Flicker::TryFindBestTarget(AActor*& OutTargetActor, FVector& OutTargetLocation) const
{
	OutTargetActor = nullptr;
	OutTargetLocation = FVector::ZeroVector;

	// 탐색 기준 Actor와 World 확인
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!IsValid(AvatarActor) || !World)
	{
		return false;
	}

	// 크로스헤어 기준 조준선 계산
	FVector AimStart = FVector::ZeroVector;
	FVector AimDirection = FVector::ForwardVector;
	if (!TryBuildAimRay(AimStart, AimDirection))
	{
		return false;
	}

	// CombatStat.SkillRange 조회
	float SkillRange = 0.0f;
	if (!SkillAbilityTag.IsValid() ||
		!TryGetFinalAbilityStat(SkillAbilityTag, NSGameplayTags::CombatStat_SkillRange, SkillRange))
	{
		return false;
	}

	SkillRange = FMath::Max(SkillRange, 0.0f);
	if (SkillRange <= 0.0f)
	{
		return false;
	}

	// 사거리 내 Pawn 후보 수집
	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FlickerTargetOverlap), false, AvatarActor);

	const bool bHasOverlap = World->OverlapMultiByObjectType(
		OverlapResults,
		AvatarActor->GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(SkillRange),
		QueryParams);

	if (!bHasOverlap)
	{
		return false;
	}

	const float MinAimDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(MaxTargetAngleDegrees, 0.0f, 90.0f)));
	float BestCrosshairDistanceSq = TNumericLimits<float>::Max();
	float BestDistanceSq = TNumericLimits<float>::Max();

	// 크로스헤어에 가장 가까운 유효 타겟 선택
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* CandidateActor = OverlapResult.GetActor();
		if (!IsValid(CandidateActor) || CandidateActor == AvatarActor)
		{
			continue;
		}

		if (!NSDamageRules::CanApplyDamage(AvatarActor, CandidateActor))
		{
			continue;
		}

		FVector CandidateOrigin = CandidateActor->GetActorLocation();
		FVector CandidateExtent = FVector::ZeroVector;
		CandidateActor->GetActorBounds(false, CandidateOrigin, CandidateExtent);
		const FVector CandidateLocation = CandidateOrigin;

		const FVector ToCandidate = CandidateLocation - AvatarActor->GetActorLocation();
		const float DistanceSq = ToCandidate.SizeSquared();
		if (DistanceSq > FMath::Square(SkillRange))
		{
			continue;
		}

		const FVector AimToCandidate = CandidateLocation - AimStart;
		const FVector AimToCandidateDirection = AimToCandidate.GetSafeNormal();
		if (AimToCandidateDirection.IsNearlyZero())
		{
			continue;
		}

		const float AimDot = FVector::DotProduct(AimDirection, AimToCandidateDirection);
		if (AimDot < MinAimDot)
		{
			continue;
		}

		if (!HasSightToTarget(AimStart, CandidateActor, CandidateLocation))
		{
			continue;
		}

		const float ProjectedDistance = FMath::Max(FVector::DotProduct(AimToCandidate, AimDirection), 0.0f);
		const FVector ClosestPointOnAimRay = AimStart + AimDirection * ProjectedDistance;
		const float CrosshairDistanceSq = FVector::DistSquared(CandidateLocation, ClosestPointOnAimRay);

		if (CrosshairDistanceSq < BestCrosshairDistanceSq ||
			(FMath::IsNearlyEqual(CrosshairDistanceSq, BestCrosshairDistanceSq) && DistanceSq < BestDistanceSq))
		{
			BestCrosshairDistanceSq = CrosshairDistanceSq;
			BestDistanceSq = DistanceSq;
			OutTargetActor = CandidateActor;
			OutTargetLocation = CandidateLocation;
		}
	}

	return IsValid(OutTargetActor);
}

bool UGA_Flicker::TryBuildAimRay(FVector& OutRayStart, FVector& OutRayDirection) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const APawn* Pawn = Cast<APawn>(AvatarActor);
	if (!IsValid(AvatarActor) || !IsValid(Pawn))
	{
		return false;
	}

	// 플레이어 캐릭터 기준 AimTrace 시작점 사용
	if (const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(AvatarActor))
	{
		PlayerCharacter->TryGetAimTraceStartLocation(OutRayStart);
	}

	// 로컬 컨트롤러의 화면 중앙 Deproject 우선 사용
	const APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
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
			if (OutRayStart.IsNearlyZero())
			{
				OutRayStart = ScreenWorldLocation;
			}

			OutRayDirection = ScreenWorldDirection.GetSafeNormal();
			return !OutRayDirection.IsNearlyZero();
		}
	}

	// 서버 또는 Deproject 실패 시 BaseAimRotation 사용
	if (OutRayStart.IsNearlyZero())
	{
		OutRayStart = AvatarActor->GetActorLocation();
	}

	OutRayDirection = Pawn->GetBaseAimRotation().Vector().GetSafeNormal();
	if (OutRayDirection.IsNearlyZero())
	{
		OutRayDirection = AvatarActor->GetActorForwardVector().GetSafeNormal();
	}

	return !OutRayDirection.IsNearlyZero();
}

bool UGA_Flicker::HasSightToTarget(
	const FVector& SightStart,
	AActor* TargetActor,
	const FVector& TargetLocation) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!IsValid(AvatarActor) || !IsValid(TargetActor) || !World)
	{
		return false;
	}

	// 시야를 막는 첫 BlockingHit 확인
	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FlickerSightTrace), false, AvatarActor);
	QueryParams.AddIgnoredActor(AvatarActor);

	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		SightStart,
		TargetLocation,
		TargetTraceChannel,
		QueryParams);

	if (!bHit)
	{
		return true;
	}

	AActor* HitActor = HitResult.GetActor();
	return HitActor == TargetActor || (IsValid(HitActor) && HitActor->GetAttachParentActor() == TargetActor);
}

bool UGA_Flicker::TryBuildAttackLocation(
	AActor* TargetActor,
	const FVector& TargetLocation,
	FVector& OutAttackLocation) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !IsValid(TargetActor))
	{
		return false;
	}

	// 현재 위치에서 타겟을 바라보는 방향의 반대편 공격 위치 계산
	FVector DirectionFromTargetToAvatar = AvatarActor->GetActorLocation() - TargetLocation;
	if (DirectionFromTargetToAvatar.IsNearlyZero())
	{
		DirectionFromTargetToAvatar = -TargetActor->GetActorForwardVector();
	}

	OutAttackLocation = TargetLocation + DirectionFromTargetToAvatar.GetSafeNormal() * FMath::Max(AttackDistance, 0.0f);
	return true;
}

bool UGA_Flicker::StartFlickerMove(const FVector& AttackLocation)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return false;
	}

	UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	if (!MovementComponent)
	{
		return false;
	}

	// 이동 종료 후 복구할 MovementMode 저장
	PreviousMovementMode = MovementComponent->MovementMode;

	const float FinalMoveDuration = FMath::Max(MoveDuration, 0.01f);

	// 높이 차이를 허용하는 Flying RootMotion 이동
	MoveTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this,
		TEXT("FlickerMove"),
		AttackLocation,
		FinalMoveDuration,
		true,
		MOVE_Flying,
		true,
		nullptr,
		ERootMotionFinishVelocityMode::SetVelocity,
		FVector::ZeroVector,
		0.0f);

	if (!MoveTask)
	{
		return false;
	}

	// 목적지 도달 또는 시간 만료 시 공격 처리
	MoveTask->OnTimedOut.AddDynamic(this, &ThisClass::OnFlickerMoveFinished);
	MoveTask->OnTimedOutAndDestinationReached.AddDynamic(this, &ThisClass::OnFlickerMoveFinished);
	MoveTask->ReadyForActivation();

	return true;
}

void UGA_Flicker::ApplyDamageToTarget()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	AActor* TargetActor = CurrentTarget.Get();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority() || !IsValid(TargetActor) || !DamageEffectClass)
	{
		return;
	}

	// Source/Target ASC 확인
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	if (!NSDamageRules::CanApplyDamage(AvatarActor, TargetActor))
	{
		return;
	}

	// DamageCoefficient 기반 데미지 계산
	float FinalDamage = 0.0f;
	if (!TryGetFinalDamage(FinalDamage))
	{
		return;
	}

	// 데미지 GE Spec 생성
	FGameplayEffectSpecHandle DamageSpecHandle =
		MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	if (!DamageSpecHandle.IsValid() || !DamageSpecHandle.Data.IsValid())
	{
		return;
	}

	FHitResult HitResult;
	HitResult.HitObjectHandle = FActorInstanceHandle(TargetActor);
	HitResult.Location = CurrentTargetLocation;
	HitResult.ImpactPoint = CurrentTargetLocation;
	HitResult.TraceStart = AvatarActor->GetActorLocation();
	HitResult.TraceEnd = CurrentTargetLocation;

	// SetByCaller 데미지와 HitResult 전달
	DamageSpecHandle.Data->SetSetByCallerMagnitude(
		NSGameplayTags::Effect_Damage_Base,
		FMath::Max(FinalDamage, 0.0f));
	DamageSpecHandle.Data->GetContext().AddHitResult(HitResult, true);
	DamageSpecHandle.Data->GetContext().AddInstigator(AvatarActor, AvatarActor);

	SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
}

bool UGA_Flicker::TryGetFinalDamage(float& OutDamage) const
{
	if (!SkillAbilityTag.IsValid())
	{
		return false;
	}

	// SkillAbilityTag 기준 최종 스킬 데미지 계산
	return TryGetFinalSkillDamage(SkillAbilityTag, OutDamage);
}

void UGA_Flicker::RestoreMovementMode() const
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UCharacterMovementComponent* MovementComponent = Character ? Character->GetCharacterMovement() : nullptr;
	if (!MovementComponent)
	{
		return;
	}

	// RootMotion 종료 후 잔여 속도 제거
	MovementComponent->StopMovementImmediately();

	if (PreviousMovementMode.IsSet())
	{
		MovementComponent->SetMovementMode(PreviousMovementMode.GetValue());
	}
}

void UGA_Flicker::AddDashingState()
{
	if (!bUseDashingStateTag)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(NSGameplayTags::State_Dashing);
	}
}

void UGA_Flicker::RemoveDashingState()
{
	if (!bUseDashingStateTag)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(NSGameplayTags::State_Dashing);
	}
}
