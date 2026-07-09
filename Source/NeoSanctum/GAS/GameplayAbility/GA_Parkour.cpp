// Copyright 2026 One Team. All rights reserved.

#include "GA_Parkour.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/RootMotionSource.h"
#include "MotionWarpingComponent.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "DrawDebugHelpers.h"

UGA_Parkour::UGA_Parkour()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Common_Parkour);
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dashing);
}

bool UGA_Parkour::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	FNSParkourTarget ParkourTarget;
	return TryFindParkourTarget(Character, ParkourTarget);
}

void UGA_Parkour::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	FNSParkourTarget ParkourTarget;
	if (!Character || !TryFindParkourTarget(Character, ParkourTarget))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AddParkourStateTags();
	UpdateMotionWarpTarget(ParkourTarget);
	PlayParkourMontage(ParkourTarget);

	if (!StartParkourMove(ParkourTarget))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UGA_Parkour::EndAbility(
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

	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	RestoreMovementMode();
	RemoveParkourStateTags();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Parkour::OnParkourMoveFinished()
{
	MoveTask = nullptr;
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Parkour::OnParkourMontageCompleted()
{
	MontageTask = nullptr;
}

void UGA_Parkour::OnParkourMontageInterrupted()
{
	MontageTask = nullptr;
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

bool UGA_Parkour::TryFindParkourTarget(const ACharacter* Character, FNSParkourTarget& OutTarget) const
{
	if (!Character)
	{
		return false;
	}

	const UWorld* World = Character->GetWorld();
	const UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	const UCapsuleComponent* CapsuleComponent = Character->GetCapsuleComponent();
	if (!World || !MovementComponent || !CapsuleComponent || MovementComponent->IsFalling())
	{
		return false;
	}

	FVector ForwardDirection = FVector::ZeroVector;
	if (!TryGetParkourForwardDirection(Character, ForwardDirection))
	{
		return false;
	}

	const float CapsuleHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
	const float CapsuleRadius = CapsuleComponent->GetScaledCapsuleRadius();
	const FVector ActorLocation = Character->GetActorLocation();
	const float ActorFloorZ = ActorLocation.Z - CapsuleHalfHeight;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NSParkourTrace), false, Character);

	// 전방 벽면 탐색
	const FVector WallTraceStart =
		FVector(ActorLocation.X, ActorLocation.Y, ActorFloorZ + WallTraceHeight);
	const FVector WallTraceEnd = WallTraceStart + ForwardDirection * ForwardTraceDistance;

	FHitResult WallHit;
	const bool bWallHit = World->LineTraceSingleByChannel(
		WallHit,
		WallTraceStart,
		WallTraceEnd,
		TraceChannel,
		QueryParams);
	if (bDrawDebugTrace)
	{
		DrawDebugLine(World, WallTraceStart, WallTraceEnd, bWallHit ? FColor::Green : FColor::Red, false, DebugTraceDuration);
	}

	if (!bWallHit || !WallHit.bBlockingHit)
	{
		return false;
	}

	const FVector WallNormal2D = WallHit.ImpactNormal.GetSafeNormal2D();
	if (FVector::DotProduct(-ForwardDirection, WallNormal2D) < MinWallFacingDot)
	{
		return false;
	}

	// 장애물 상단 탐색
	const FVector TopTraceBase = WallHit.ImpactPoint + ForwardDirection * TopTraceForwardOffset;
	const FVector TopTraceStart(
		TopTraceBase.X,
		TopTraceBase.Y,
		ActorFloorZ + MaxObstacleHeight + TopTraceExtraHeight);
	const FVector TopTraceEnd(
		TopTraceBase.X,
		TopTraceBase.Y,
		ActorFloorZ + MinObstacleHeight);

	FHitResult TopHit;
	const bool bTopHit = World->LineTraceSingleByChannel(
		TopHit,
		TopTraceStart,
		TopTraceEnd,
		TraceChannel,
		QueryParams);
	if (bDrawDebugTrace)
	{
		DrawDebugLine(World, TopTraceStart, TopTraceEnd, bTopHit ? FColor::Blue : FColor::Red, false, DebugTraceDuration);
	}

	if (!bTopHit || !TopHit.bBlockingHit || TopHit.ImpactNormal.Z < MinLandingNormalZ)
	{
		return false;
	}

	// Vault 가능 높이 확인
	const float ObstacleHeight = TopHit.ImpactPoint.Z - ActorFloorZ;
	if (ObstacleHeight < MinObstacleHeight || ObstacleHeight > MaxObstacleHeight)
	{
		return false;
	}

	// 착지 후보 지점 탐색
	const FVector LandingTraceBase = TopHit.ImpactPoint + ForwardDirection * LandingForwardOffset;
	const FVector LandingTraceStart = LandingTraceBase + FVector::UpVector * LandingTraceUpOffset;
	const FVector LandingTraceEnd = LandingTraceStart - FVector::UpVector * LandingTraceDownDistance;

	FHitResult LandingHit;
	const bool bLandingHit = World->LineTraceSingleByChannel(
		LandingHit,
		LandingTraceStart,
		LandingTraceEnd,
		TraceChannel,
		QueryParams);
	if (bDrawDebugTrace)
	{
		DrawDebugLine(World, LandingTraceStart, LandingTraceEnd, bLandingHit ? FColor::Cyan : FColor::Red, false, DebugTraceDuration);
	}

	if (!bLandingHit || !LandingHit.bBlockingHit || LandingHit.ImpactNormal.Z < MinLandingNormalZ)
	{
		return false;
	}

	// 착지 위치 캡슐 여유 확인
	const FVector TargetActorLocation = LandingHit.ImpactPoint + FVector::UpVector * (CapsuleHalfHeight + 2.0f);
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
	const bool bBlockedAtTarget = World->OverlapBlockingTestByChannel(
		TargetActorLocation,
		FQuat::Identity,
		CapsuleComponent->GetCollisionObjectType(),
		CapsuleShape,
		QueryParams);
	if (bDrawDebugTrace)
	{
		DrawDebugCapsule(
			World,
			TargetActorLocation,
			CapsuleHalfHeight,
			CapsuleRadius,
			FQuat::Identity,
			bBlockedAtTarget ? FColor::Red : FColor::Green,
			false,
			DebugTraceDuration);
	}

	if (bBlockedAtTarget)
	{
		return false;
	}

	OutTarget.ActorLocation = TargetActorLocation;
	OutTarget.ActorRotation = ForwardDirection.Rotation();
	OutTarget.SurfaceLocation = LandingHit.ImpactPoint;
	OutTarget.ObstacleHeight = ObstacleHeight;
	return true;
}

bool UGA_Parkour::TryGetParkourForwardDirection(const ACharacter* Character, FVector& OutDirection) const
{
	if (!Character)
	{
		return false;
	}

	const APawn* Pawn = Cast<APawn>(Character);
	const AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	if (Controller)
	{
		OutDirection = FRotator(0.0f, Controller->GetControlRotation().Yaw, 0.0f).Vector().GetSafeNormal2D();
	}

	if (OutDirection.IsNearlyZero())
	{
		OutDirection = Character->GetActorForwardVector().GetSafeNormal2D();
	}

	return !OutDirection.IsNearlyZero();
}

void UGA_Parkour::AddParkourStateTags()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(NSGameplayTags::State_Input_BlockInputMove);
	}
}

void UGA_Parkour::RemoveParkourStateTags()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(NSGameplayTags::State_Input_BlockInputMove);
	}
}

void UGA_Parkour::RestoreMovementMode()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UCharacterMovementComponent* MovementComponent = Character ? Character->GetCharacterMovement() : nullptr;
	if (!MovementComponent)
	{
		return;
	}

	MovementComponent->StopMovementImmediately();

	if (PreviousMovementMode.IsSet() && PreviousMovementMode.GetValue() != MOVE_Flying)
	{
		MovementComponent->SetMovementMode(PreviousMovementMode.GetValue());
	}
	else
	{
		const EMovementMode RestoreMode = MovementComponent->IsMovingOnGround()
			? MOVE_Walking
			: MOVE_Falling;
		MovementComponent->SetMovementMode(RestoreMode);
	}

	PreviousMovementMode.Reset();
}

void UGA_Parkour::UpdateMotionWarpTarget(const FNSParkourTarget& Target) const
{
	if (!bEnableMotionWarping || MotionWarpTargetName.IsNone())
	{
		return;
	}

	const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(GetAvatarActorFromActorInfo());
	UMotionWarpingComponent* MotionWarpingComponent =
		PlayerCharacter ? PlayerCharacter->GetMotionWarpingComponent() : nullptr;
	if (!MotionWarpingComponent)
	{
		return;
	}

	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(
		MotionWarpTargetName,
		Target.ActorLocation,
		Target.ActorRotation);
}

bool UGA_Parkour::PlayParkourMontage(const FNSParkourTarget& Target)
{
	if (!ParkourMontage)
	{
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	USkeletalMeshComponent* MeshComponent = Character ? Character->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return false;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		TEXT("ParkourMontageTask"),
		ParkourMontage,
		FMath::Max(ParkourMontagePlayRate, 0.01f),
		SelectParkourMontageSection(),
		false);

	if (!MontageTask)
	{
		return false;
	}

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnParkourMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnParkourMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnParkourMontageInterrupted);
	MontageTask->ReadyForActivation();
	return true;
}

FName UGA_Parkour::SelectParkourMontageSection() const
{
	// 빈 섹션 이름 제외
	TArray<FName> ValidSectionNames;
	for (const FName& SectionName : ParkourMontageSectionNames)
	{
		if (!SectionName.IsNone())
		{
			ValidSectionNames.Add(SectionName);
		}
	}

	if (ValidSectionNames.IsEmpty())
	{
		return NAME_None;
	}

	// 후보 중 하나를 랜덤 선택
	return ValidSectionNames[FMath::RandHelper(ValidSectionNames.Num())];
}

bool UGA_Parkour::StartParkourMove(const FNSParkourTarget& Target)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UCharacterMovementComponent* MovementComponent = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Character || !MovementComponent)
	{
		return false;
	}

	if (!PreviousMovementMode.IsSet())
	{
		PreviousMovementMode = MovementComponent->MovementMode;
	}

	MovementComponent->StopMovementImmediately();

	MoveTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this,
		TEXT("ParkourMove"),
		Target.ActorLocation,
		FMath::Max(MoveDuration, 0.01f),
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

	MoveTask->OnTimedOut.AddDynamic(this, &ThisClass::OnParkourMoveFinished);
	MoveTask->OnTimedOutAndDestinationReached.AddDynamic(this, &ThisClass::OnParkourMoveFinished);
	MoveTask->ReadyForActivation();
	return true;
}
