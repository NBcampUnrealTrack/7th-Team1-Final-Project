// Copyright 2026 One Team. All rights reserved.

#include "NSTurretSpawner.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NeoSanctum/Combat/Weapon/Summon/NSTurret.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"

ANSTurretSpawner::ANSTurretSpawner()
{
	UProjectileMovementComponent* Movement = GetProjectileMovementComponent();
	if (Movement)
	{
		Movement->bShouldBounce = true;
		// 탄력
		Movement->Bounciness = 0.55f;
		// 마찰력
		Movement->Friction = 0.2f;
	}
}

void ANSTurretSpawner::InitializeTurretSpawner(const FNSTurretSpawnerTypeConfig& InConfig)
{
	TurretClass = InConfig.TurretClass;
	TurretConfig = InConfig.TurretConfig;

	// 데이터 기반 소환 가능 경사각 적용
	float RuntimeMaxSpawnableAngle = 0.0f;
	if (TryGetRuntimeStatMagnitude(NSGameplayTags::CombatStat_MaxSpawnableAngle, RuntimeMaxSpawnableAngle))
	{
		MaxSpawnableAngle = RuntimeMaxSpawnableAngle;
	}
}

void ANSTurretSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (SpawnerLifeSpan > 0.0f)
	{
		SetLifeSpan(SpawnerLifeSpan);
	}
	
	if (UProjectileMovementComponent* Movement = GetProjectileMovementComponent())
	{
		Movement->OnProjectileBounce.AddDynamic(this, &ThisClass::OnProjectileBounce);
	}
}

void ANSTurretSpawner::OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (bSpawned || !HasAuthority())
	{
		return;
	}
	
	if (!IsSpawnableSurface(ImpactResult))
	{
		return;
	}
	
	bSpawned = true;
	
	if (UProjectileMovementComponent* Movement = GetProjectileMovementComponent())
	{
		Movement->StopMovementImmediately();
		Movement->Deactivate();
	}
	
	SpawnTurret(ImpactResult);
	Destroy();
}

bool ANSTurretSpawner::IsSpawnableSurface(const FHitResult& ImpactResult) const
{
	const float MinGroundNormalZ = FMath::Cos(FMath::DegreesToRadians(MaxSpawnableAngle));

	return ImpactResult.bBlockingHit && ImpactResult.ImpactNormal.GetSafeNormal().Z >= MinGroundNormalZ;
}

void ANSTurretSpawner::SpawnTurret(const FHitResult& ImpactResult)
{
	if (!TurretClass || !GetWorld())
	{
		return;
	}

	const ANSTurret* TurretCDO = TurretClass->GetDefaultObject<ANSTurret>();
	const float SpawnSurfaceOffset = TurretCDO ? TurretCDO->GetSpawnSurfaceOffset() : 0.0f;
	const FVector SpawnLocation = ImpactResult.ImpactPoint + ImpactResult.ImpactNormal.GetSafeNormal() * SpawnSurfaceOffset;
	const FVector OwnerForward = GetOwningPawn() ? GetOwningPawn()->GetActorForwardVector() : GetActorForwardVector();

	FVector ProjectedForward =
		FVector::VectorPlaneProject(OwnerForward, ImpactResult.ImpactNormal).GetSafeNormal();

	if (ProjectedForward.IsNearlyZero())
	{
		ProjectedForward =
			FVector::VectorPlaneProject(GetActorForwardVector(), ImpactResult.ImpactNormal).GetSafeNormal();
	}

	const FRotator SpawnRotation =
		FRotationMatrix::MakeFromZX(ImpactResult.ImpactNormal.GetSafeNormal(), ProjectedForward).Rotator();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwningPawn();
	SpawnParams.Instigator = GetOwningPawn();
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ANSTurret* Turret = GetWorld()->SpawnActor<ANSTurret>(
		TurretClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	if (Turret)
	{
		// Turret 초기화에 SetByCaller payload 전달
		Turret->InitializeTurret(
			TurretConfig,
			GetOwningPawn(),
			GetOwningController(),
			GetSetByCallerMagnitudes(),
			GetRuntimeStatMagnitudes()
		);

		ExecuteDeployCue(Turret, ImpactResult.ImpactNormal.GetSafeNormal());
	}
}

void ANSTurretSpawner::ExecuteDeployCue(const ANSTurret* Turret, const FVector& SurfaceNormal) const
{
	if (!Turret)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwningPawn());
	if (!SourceASC)
	{
		return;
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Instigator = GetOwningPawn();
	CueParameters.Location = Turret->GetActorLocation();
	CueParameters.Normal = SurfaceNormal.IsNearlyZero() ? FVector::UpVector : SurfaceNormal;

	// 설치 완료 위치에서 모든 클라이언트에 연출 실행
	SourceASC->ExecuteGameplayCue(
		NSGameplayTags::GameplayCue_Engineer_SpawnTurret_Deploy,
		CueParameters
	);
}
