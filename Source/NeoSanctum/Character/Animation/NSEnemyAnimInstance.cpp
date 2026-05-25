// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"

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
		GroundSpeed = EnemyCharacter->GetVelocity().Size2D();
		bIsMoving = (GroundSpeed > MovingSpeedThreshold) 
				&& (MovementComponent->GetCurrentAcceleration().Size2D() > 0.0f);
	}
}
