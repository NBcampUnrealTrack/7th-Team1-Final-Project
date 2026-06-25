// Copyright 2026 One Team. All rights reserved.


#include "NSThrowProjectileBase.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"

ANSThrowProjectileBase::ANSThrowProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(15.0f);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionComponent);
	MeshComponent->SetGenerateOverlapEvents(false);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
	ProjectileMovementComponent->InitialSpeed = 1500.0f;
	ProjectileMovementComponent->MaxSpeed = 3000.0f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = true;
}

void ANSThrowProjectileBase::InitializeThrowActor(
	APawn* InOwningPawn,
	AController* InOwningController,
	const FVector& ThrowDirection)
{
	OwningPawn = InOwningPawn;
	OwningController = InOwningController;

	if (OwningPawn)
	{
		SetOwner(OwningPawn);
		SetInstigator(OwningPawn);
		CollisionComponent->IgnoreActorWhenMoving(OwningPawn, true);
	}

	if (!ThrowDirection.IsNearlyZero())
	{
		// CombatStat.ProjectileSpeed가 전달되면 CDO 기본 속도 대신 데이터 기반 속도로 투척
		float RuntimeProjectileSpeed = 0.0f;
		if (TryGetRuntimeStatMagnitude(NSGameplayTags::CombatStat_ProjectileSpeed, RuntimeProjectileSpeed) &&
			RuntimeProjectileSpeed > 0.0f)
		{
			ProjectileMovementComponent->InitialSpeed = RuntimeProjectileSpeed;
			ProjectileMovementComponent->MaxSpeed = FMath::Max(
				ProjectileMovementComponent->MaxSpeed,
				RuntimeProjectileSpeed
			);
		}

		const FVector NormalizedThrowDirection = ThrowDirection.GetSafeNormal();
		SetActorRotation(NormalizedThrowDirection.Rotation());
		ProjectileMovementComponent->Velocity = NormalizedThrowDirection * ProjectileMovementComponent->InitialSpeed;
	}
}

void ANSThrowProjectileBase::SetSetByCallerMagnitudes(
	const TArray<FNSSetByCallerMagnitude>& InSetByCallerMagnitudes)
{
	// GE SetByCaller payload 저장
	SetByCallerMagnitudes = InSetByCallerMagnitudes;
}

void ANSThrowProjectileBase::SetRuntimeStatMagnitudes(
	const TArray<FNSCombatStatMagnitude>& InRuntimeStatMagnitudes)
{
	// 런타임 stat payload 저장
	RuntimeStatMagnitudes = InRuntimeStatMagnitudes;
}

bool ANSThrowProjectileBase::TryGetRuntimeStatMagnitude(
	const FGameplayTag& CombatStatTag,
	float& OutMagnitude) const
{
	// 런타임 stat 값 조회
	for (const FNSCombatStatMagnitude& RuntimeStatMagnitude : RuntimeStatMagnitudes)
	{
		if (RuntimeStatMagnitude.CombatStatTag == CombatStatTag)
		{
			OutMagnitude = RuntimeStatMagnitude.Magnitude;
			return true;
		}
	}

	return false;
}

bool ANSThrowProjectileBase::TryGetRuntimeStatBool(
	const FGameplayTag& CombatStatTag,
	bool& OutValue) const
{
	// DataTable이 float Value만 지원하기 때문에 boolean 옵션은 0 초과를 true로 해석
	float Magnitude = 0.0f;
	if (!TryGetRuntimeStatMagnitude(CombatStatTag, Magnitude))
	{
		return false;
	}

	OutValue = Magnitude > 0.0f;
	return true;
}
