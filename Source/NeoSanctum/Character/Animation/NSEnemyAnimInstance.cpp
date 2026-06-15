// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Combat/Weapon/NSEnemyWeaponBase.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"

void UNSEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	EnemyCharacter = Cast<ANSEnemyCharacterBase>(TryGetPawnOwner());
	if (EnemyCharacter)
	{
		MovementComponent = EnemyCharacter->GetCharacterMovement();
	}
}

void UNSEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 초기화 실패 시 혹은 런타임 예외 발생 시 다시 캐싱
	if (!EnemyCharacter || !MovementComponent)
	{
		EnemyCharacter = Cast<ANSEnemyCharacterBase>(TryGetPawnOwner());
		if (EnemyCharacter)
		{
			MovementComponent = EnemyCharacter->GetCharacterMovement();
		}
	}

	if (MovementComponent && EnemyCharacter)
	{
		const FVector Velocity = EnemyCharacter->GetVelocity();
		
		// 수평 속도만 사용하여 Idle과 Walk/Run 구분
		GroundSpeed = Velocity.Size2D();
		bIsMoving = GroundSpeed > MovingSpeedThreshold;
		
		// 양수면 상승, 음수면 낙하 중
		VerticalVelocity = Velocity.Z;
		
		// LaunchCharacter가 실행되면 MovementMode가 Falling으로 변경됨
		bIsInAir = MovementComponent->IsFalling();
	}
	else
	{
		GroundSpeed = 0.0f;
		VerticalVelocity = 0.0f;
		bIsMoving = false;
		bIsInAir = false;
	}

	UpdateLeftHandIK(DeltaSeconds);
}

void UNSEnemyAnimInstance::UpdateLeftHandIK(float DeltaSeconds)
{
	float TargetAlpha = 0.0f;

	if (!IsValid(EnemyCharacter) || EnemyCharacter->IsDead())
	{
		UpdateLeftHandIKAlpha(TargetAlpha, DeltaSeconds);
		return;
	}

	const ANSEnemyWeaponBase* CurrentWeapon = EnemyCharacter->GetCurrentWeapon();

	if (!IsValid(CurrentWeapon))
	{
		UpdateLeftHandIKAlpha(TargetAlpha, DeltaSeconds);
		return;
	}
	
	const UAbilitySystemComponent* ASC = EnemyCharacter->GetAbilitySystemComponent();
	const bool bIsInCombat = IsValid(ASC) && ASC->HasMatchingGameplayTag(NSGameplayTags::State_Enemy_Combat);
	const bool bShouldUseIK = CurrentWeapon->ShouldUseLeftHandIKWhileEquipped() || bIsInCombat;
	
	if (!bShouldUseIK)
	{
		UpdateLeftHandIKAlpha(TargetAlpha, DeltaSeconds);
		return;
	}

	FTransform SocketWorldTransform;
	if (!CurrentWeapon->TryGetLeftHandIKTransform(SocketWorldTransform))
	{
		UpdateLeftHandIKAlpha(TargetAlpha, DeltaSeconds);
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetOwningComponent();

	if (!IsValid(MeshComponent) ||
		!MeshComponent->DoesSocketExist(TEXT("hand_r")))
	{
		UpdateLeftHandIKAlpha(TargetAlpha, DeltaSeconds);
		return;
	}

	FVector BoneSpaceLocation = FVector::ZeroVector;
	FRotator BoneSpaceRotation = FRotator::ZeroRotator;

	MeshComponent->TransformToBoneSpace(
		TEXT("hand_r"),
		SocketWorldTransform.GetLocation(),
		SocketWorldTransform.Rotator(),
		BoneSpaceLocation,
		BoneSpaceRotation);

	LeftHandIKTransform = FTransform(
		BoneSpaceRotation,
		BoneSpaceLocation,
		FVector::OneVector);

	TargetAlpha = 1.0f;
	UpdateLeftHandIKAlpha(TargetAlpha, DeltaSeconds);
}

void UNSEnemyAnimInstance::UpdateLeftHandIKAlpha(float TargetAlpha, float DeltaSeconds)
{
	LeftHandIKAlpha = FMath::FInterpTo(
		LeftHandIKAlpha,
		TargetAlpha,
		DeltaSeconds,
		LeftHandIKInterpSpeed);

	if (LeftHandIKAlpha <= UE_KINDA_SMALL_NUMBER)
	{
		LeftHandIKAlpha = 0.0f;
	}
}
