// Copyright 2026 One Team. All rights reserved.


#include "NSDroneProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "NeoSanctum/Combat/NSDamageRules.h"


ANSDroneProjectile::ANSDroneProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SetRootComponent(CollisionComp);
	CollisionComp->InitSphereRadius(5.f);
	
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel1);
	
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	CollisionComp->SetCanEverAffectNavigation(false);
	
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComp->SetupAttachment(CollisionComp);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetCanEverAffectNavigation(false);
	
	ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComp->bRotationFollowsVelocity = true;
	ProjectileMovementComp->bShouldBounce = false;
	ProjectileMovementComp->ProjectileGravityScale = 0.f;
	
	bReplicates = true;
	SetReplicateMovement(false);
	InitialLifeSpan = 3.f;
}

void ANSDroneProjectile::InitProjectile(const FVector& Direction, APawn* InInstigator,
	const FGameplayEffectSpecHandle& InDamageSpec, float ProjectileSpeed)
{
	if (!InDamageSpec.IsValid() || ProjectileSpeed <= 0.f) return;
	
	DamageSpecHandle = InDamageSpec;
	InitialSpeed = ProjectileSpeed;
	
	SetInstigator(InInstigator);
	SetOwner(InInstigator);
}

void ANSDroneProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		CollisionComp->OnComponentHit.AddDynamic(this, &ANSDroneProjectile::OnHit);
		ApplyVelocity();
	}
	
}

void ANSDroneProjectile::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ANSDroneProjectile, InitialSpeed);
}

void ANSDroneProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority()) return;
	if (OtherActor == nullptr) return;
	if (OtherActor == GetInstigator()) return;

	DrawDebugSphere(
		GetWorld(),
		Hit.ImpactPoint,
		5.f,
		16,
		FColor::Green,
		false,
		2.f);
	
	UAbilitySystemComponent* EnemyASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (!EnemyASC) return;

	if (!NSDamageRules::CanApplyDamage(GetInstigator(), OtherActor)) return;
	
	if (DamageSpecHandle.IsValid())
	{
		EnemyASC->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data.Get());
	}
	
	Destroy();
}

void ANSDroneProjectile::OnRep_InitialSpeed()
{
	ApplyVelocity();
}

void ANSDroneProjectile::ApplyVelocity()
{
	if (!ProjectileMovementComp) return;
	ProjectileMovementComp->Velocity = GetActorForwardVector() * InitialSpeed;
}


