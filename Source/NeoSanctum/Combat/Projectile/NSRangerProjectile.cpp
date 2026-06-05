// Copyright 2026 One Team. All rights reserved.


#include "NSRangerProjectile.h"

#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"


ANSRangerProjectile::ANSRangerProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	
	bReplicates = true;
	SetReplicateMovement(true);
	
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	
	CollisionComponent->InitSphereRadius(12.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->OnComponentHit.AddDynamic(this, &ThisClass::OnProjectileHit);
	
	VisualMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMeshComponent"));
	VisualMeshComponent->SetupAttachment(CollisionComponent);
	VisualMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMeshComponent->SetGenerateOverlapEvents(false);
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 2500.0f;
	ProjectileMovement->MaxSpeed = 2500.0f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
}

void ANSRangerProjectile::InitializeProjectile(UAbilitySystemComponent* InSourceASC)
{
	SourceASC = InSourceASC;
}

void ANSRangerProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	IgnoreSourceActorCollision();

	SetLifeSpan(LifeSeconds);
}


void ANSRangerProjectile::LaunchProjectile(const FVector& LaunchDirection)
{
	if (!ProjectileMovement)
	{
		return;
	}
	
	const FVector SafeLaunchDirection = LaunchDirection.GetSafeNormal();
	
	if (SafeLaunchDirection.IsNearlyZero())
	{
		return;
	}
	
	// SpawnRotation만 믿지 않고 ProjectileMovement 속도를 명시적으로 지정
	SetActorRotation(SafeLaunchDirection.Rotation());
	ProjectileMovement->Velocity = SafeLaunchDirection * ProjectileMovement->InitialSpeed;
}

void ANSRangerProjectile::IgnoreSourceActorCollision()
{
	if (!CollisionComponent)
	{
		return;
	}
	
	AActor* OwnerActor = GetOwner();
	APawn* InstigatorPawn = GetInstigator();
	
	// 발사 직후 자기 캐릭터와 부딪혀 사라지는 것 방지
	if (IsValid(OwnerActor))
	{
		CollisionComponent->IgnoreActorWhenMoving(OwnerActor, true);
	}
	
	if (IsValid(InstigatorPawn) && InstigatorPawn != OwnerActor)
	{
		CollisionComponent->IgnoreActorWhenMoving(InstigatorPawn, true);
	}
}

void ANSRangerProjectile::OnProjectileHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& HitResult)
{
	if (!HasAuthority())
	{
		return;
	}
	
	// TODO: 폭발 데미지는 다음 단계에서 추가
	ExecuteImpactCue(HitResult);
	
	Destroy();
}

void ANSRangerProjectile::ExecuteImpactCue(const FHitResult& HitResult)
{
	if (!SourceASC || !HitResult.bBlockingHit)
	{
		return;
	}
	
	FGameplayCueParameters CueParameters;
	CueParameters.Instigator = GetInstigator();
	CueParameters.EffectCauser = this;
	CueParameters.Location = HitResult.ImpactPoint;
	CueParameters.Normal = HitResult.ImpactNormal;
	
	SourceASC->ExecuteGameplayCue(NSGameplayTags::GameplayCue_Ranger_ProjectileShot_Impact, CueParameters);
}
