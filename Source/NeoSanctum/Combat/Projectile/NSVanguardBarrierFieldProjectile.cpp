// Copyright 2026 One Team. All rights reserved.

#include "NSVanguardBarrierFieldProjectile.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NeoSanctum/Combat/Weapon/Summon/NSVanguardBarrierField.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"

ANSVanguardBarrierFieldProjectile::ANSVanguardBarrierFieldProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	if (UProjectileMovementComponent* Movement = GetProjectileMovementComponent())
	{
		// 거의 직선으로 날아가는 투사체 설정
		Movement->ProjectileGravityScale = 0.05f;
		Movement->bShouldBounce = false;
		Movement->bRotationFollowsVelocity = true;
	}
}

void ANSVanguardBarrierFieldProjectile::InitializeBarrierFieldProjectile(
	const FNSShieldFieldTypeConfig& InConfig)
{
	ShieldFieldConfig = InConfig;
}

void ANSVanguardBarrierFieldProjectile::BeginPlay()
{
	Super::BeginPlay();

	StartLocation = GetActorLocation();

	if (UProjectileMovementComponent* Movement = GetProjectileMovementComponent())
	{
		Movement->OnProjectileStop.AddDynamic(this, &ThisClass::OnProjectileStopped);
	}
}

void ANSVanguardBarrierFieldProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateVisualRoll(DeltaSeconds);

	if (!HasAuthority() || bFieldDeployed)
	{
		return;
	}

	const float MaxTravelDistance = GetMaxTravelDistance();
	if (MaxTravelDistance <= 0.0f)
	{
		return;
	}

	if (FVector::DistSquared(StartLocation, GetActorLocation()) >= FMath::Square(MaxTravelDistance))
	{
		const FVector DeployNormal = -GetVelocity().GetSafeNormal();
		// 충돌하지 않은 경우 최대 사거리 지점에 설치
		DeployField(GetActorLocation(), DeployNormal.IsNearlyZero() ? FVector::UpVector : DeployNormal);
	}
}

void ANSVanguardBarrierFieldProjectile::UpdateVisualRoll(float DeltaSeconds) const
{
	UStaticMeshComponent* VisualMeshComponent = GetMeshComponent();
	if (!VisualMeshComponent || FMath::IsNearlyZero(VisualRollSpeed))
	{
		return;
	}

	VisualMeshComponent->AddLocalRotation(FRotator(0.0f, 0.0f, VisualRollSpeed * DeltaSeconds));
}

void ANSVanguardBarrierFieldProjectile::OnProjectileStopped(const FHitResult& ImpactResult)
{
	if (!HasAuthority() || bFieldDeployed)
	{
		return;
	}

	const FVector DeployLocation =
		ImpactResult.bBlockingHit ? FVector(ImpactResult.ImpactPoint) : GetActorLocation();
	const FVector DeployNormal =
		ImpactResult.bBlockingHit ? ImpactResult.ImpactNormal.GetSafeNormal() : FVector::UpVector;

	// 첫 충돌 지점에 설치
	DeployField(DeployLocation, DeployNormal.IsNearlyZero() ? FVector::UpVector : DeployNormal);
}

void ANSVanguardBarrierFieldProjectile::DeployField(
	const FVector& DeployLocation,
	const FVector& DeployNormal)
{
	if (bFieldDeployed || !HasAuthority() || !ShieldFieldConfig.FieldClass || !GetWorld())
	{
		return;
	}

	bFieldDeployed = true;

	if (UProjectileMovementComponent* Movement = GetProjectileMovementComponent())
	{
		Movement->StopMovementImmediately();
		Movement->Deactivate();
	}

	const float FieldRadius = GetFieldRadius();
	// 표면에 너무 묻히지 않도록 충돌 Normal 방향으로 설치하도록 함
	const FVector SpawnLocation = DeployLocation + DeployNormal.GetSafeNormal() * FMath::Clamp(FieldRadius * 0.25f, 30.0f, 120.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwningPawn();
	SpawnParams.Instigator = GetOwningPawn();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	// 스폰 단계
	ANSVanguardBarrierField* Field = GetWorld()->SpawnActor<ANSVanguardBarrierField>(
		ShieldFieldConfig.FieldClass,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (Field)
	{
		Field->InitializeBarrierField(
			ShieldFieldConfig,
			GetOwningPawn(),
			GetOwningController(),
			FieldRadius,
			GetFieldDuration(),
			GetDamageInterval(),
			GetSetByCallerMagnitudes()
		);
		Field->ForceNetUpdate();
	}

	CompletePlayerAttackFeedbackGroup();
	Destroy();
}

float ANSVanguardBarrierFieldProjectile::GetFieldRadius() const
{
	float Radius = 0.0f;
	return TryGetRuntimeStatMagnitude(NSGameplayTags::CombatStat_Radius, Radius)
		? FMath::Max(Radius, 0.0f)
		: 300.0f;
}

float ANSVanguardBarrierFieldProjectile::GetFieldDuration() const
{
	float Duration = 0.0f;
	return TryGetRuntimeStatMagnitude(NSGameplayTags::CombatStat_Duration, Duration)
		? FMath::Max(Duration, 0.0f)
		: 4.0f;
}

float ANSVanguardBarrierFieldProjectile::GetDamageInterval() const
{
	float DamageInterval = 0.0f;
	return TryGetRuntimeStatMagnitude(NSGameplayTags::CombatStat_DamageInterval, DamageInterval)
		? FMath::Max(DamageInterval, 0.01f)
		: 0.5f;
}

float ANSVanguardBarrierFieldProjectile::GetMaxTravelDistance() const
{
	float MaxTravelDistance = 0.0f;
	return TryGetRuntimeStatMagnitude(NSGameplayTags::CombatStat_SkillRange, MaxTravelDistance)
		? FMath::Max(MaxTravelDistance, 0.0f)
		: 1800.0f;
}
