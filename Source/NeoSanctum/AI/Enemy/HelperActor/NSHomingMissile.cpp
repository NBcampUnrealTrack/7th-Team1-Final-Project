// Copyright 2026 One Team. All rights reserved.


#include "NSHomingMissile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NeoSanctum/Collision/NSCollisionProfiles.h"
#include "NeoSanctum/Combat/NSDamageRules.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"


ANSHomingMissile::ANSHomingMissile()
{
	PrimaryActorTick.bCanEverTick = true;
	
	bReplicates = true;
	SetReplicateMovement(true);
	
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	
	CollisionComponent->InitSphereRadius(12.0f);
	CollisionComponent->SetCollisionProfileName(NSCollisionProfiles::EnemyMissile);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->SetCanEverAffectNavigation(false);
	CollisionComponent->OnComponentHit.AddDynamic(this, &ThisClass::OnMissileHit);
	
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCanEverAffectNavigation(false);
	MeshComponent->SetGenerateOverlapEvents(false);
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
}

void ANSHomingMissile::InitializeMissile(UAbilitySystemComponent* InSourceASC,
	TSubclassOf<UGameplayEffect> InDamageEffectClass, float InExplosionDamage, float InExplosionRadius,
	USceneComponent* InHomingTarget, const TArray<AActor*>& InIgnoredActors)
{
	SourceASC = InSourceASC;
	DamageEffectClass = InDamageEffectClass;
	ExplosionDamage = InExplosionDamage;
	if (InExplosionRadius > 0.0f)
	{
		ExplosionRadius = InExplosionRadius;
	}
	ProjectileMovement->HomingTargetComponent = InHomingTarget;
	IgnoreCollisionForActors(InIgnoredActors);
}

void ANSHomingMissile::LaunchMissile(const FVector& LaunchDirection)
{
	const FVector NormalDir = LaunchDirection.GetSafeNormal();
	
	if (NormalDir.IsNearlyZero())
	{
		return;
	}
	
	SetActorRotation(NormalDir.Rotation());
	ProjectileMovement->Velocity = NormalDir * InitialSpeed;
	CurrentSpeed = InitialSpeed;
}

void ANSHomingMissile::BeginPlay()
{
	Super::BeginPlay();
	IgnoreCollisionForActors({GetOwner(), GetInstigator()});
	ProjectileMovement->HomingAccelerationMagnitude = HomingAccelerationMagnitude;
	SetLifeSpan(LifeSeconds);
}

void ANSHomingMissile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	CurrentSpeed = FMath::Min(CurrentSpeed + Acceleration * DeltaTime, MaxSpeed);
	ProjectileMovement->Velocity = ProjectileMovement->Velocity.GetSafeNormal() * CurrentSpeed;
	
}

void ANSHomingMissile::IgnoreCollisionForActors(const TArray<AActor*>& ActorsToIgnore)
{
	for (AActor* A : ActorsToIgnore)
	{
		if (IsValid(A))
		{
			CollisionComponent->IgnoreActorWhenMoving(A, true);
		}
	}
}

void ANSHomingMissile::FindSplashTargetActors(const FVector& ExplosionLocation, TArray<AActor*>& OutTargetActors) const
{
	OutTargetActors.Reset();
	
	UWorld* World = GetWorld();
	
	if (!World || ExplosionRadius <= 0.0f)
	{
		return;
	}
	
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(NSCollisionChannels::Player);
	ObjectQueryParams.AddObjectTypesToQuery(NSCollisionChannels::Enemy);
	ObjectQueryParams.AddObjectTypesToQuery(NSCollisionChannels::DestructibleObject);
	ObjectQueryParams.AddObjectTypesToQuery(NSCollisionChannels::PlayerConstruct);
	
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HomingMissileSplash), false);
	QueryParams.AddIgnoredActor(this);
	
	AActor* OwnerActor = GetOwner();
	APawn* InstigatorPawn = GetInstigator();
	
	if (IsValid(OwnerActor))
	{
		QueryParams.AddIgnoredActor(OwnerActor);
	}
	
	if (IsValid(InstigatorPawn))
	{
		QueryParams.AddIgnoredActor(InstigatorPawn);
	}
	
	TArray<FOverlapResult> OverlapResults;
	const FCollisionShape SplashShape = FCollisionShape::MakeSphere(ExplosionRadius);
	
	World->OverlapMultiByObjectType(
		OverlapResults,
		ExplosionLocation,
		FQuat::Identity,
		ObjectQueryParams,
		SplashShape,
		QueryParams
	);
	
	TSet<AActor*> UniqueTargetActors;
	
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* TargetActor = OverlapResult.GetActor();
		
		if (!IsValid(TargetActor))
		{
			continue;
		}
		
		// QueryParams 무시 목록을 통과한 경우에도 한 번 더 안전하게 거름
		if (TargetActor == this || TargetActor == OwnerActor || TargetActor == InstigatorPawn)
		{
			continue;
		}
		
		UniqueTargetActors.Add(TargetActor);
	}
	
	OutTargetActors.Reserve(UniqueTargetActors.Num());
	
	for (AActor* TargetActor : UniqueTargetActors)
	{
		OutTargetActors.Add(TargetActor);
	}
	
	if (bDrawDebugExplosion)
	{
		DrawDebugSphere(
			World,
			ExplosionLocation,
			ExplosionRadius,
			24,
			FColor::Red,
			false,
			2.0f
		);
	}
}

void ANSHomingMissile::FilterOccludedSplashTargets(const FVector& TraceStart, TArray<AActor*>& TargetActors) const
{
	if (!bExcludeOccludedSplashTargets || TargetActors.IsEmpty())
	{
		return;
	}
	
	const int32 BeforeCount = TargetActors.Num();
	
	// TargetActors 안의 각 TargetActor를 하나 씩 검사
	// IsSplashTraceOcclusion(TraceStart, TargetActor)가 true면 제거
	// false면 배열에 남김
	TargetActors.RemoveAll([this, &TraceStart](const AActor* TargetActor)
	{
		return IsSplashTargetOccluded(TraceStart, TargetActor);
	});
	
	const int32 RemovedCount = BeforeCount - TargetActors.Num();
	
	if (RemovedCount > 0)
	{
		NS_ACTOR_LOG(this, LogNSGAS, Log,
			"벽에 가려진 스플래시 대상을 제외. 제외수={RemovedCount}, 남은대상수={RemainingCount}",
			("RemovedCount", RemovedCount),
			("RemainingCount", TargetActors.Num())
		);
	}
}

bool ANSHomingMissile::IsSplashTargetOccluded(const FVector& TraceStart, const AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return true;
	}
	
	UWorld* World = GetWorld();
	
	if (!World)
	{
		return false;
	}
	
	const FVector TraceEnd = TargetActor->GetActorLocation();
	
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HomingMissileSplashOcclusion), false);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(TargetActor);
	
	AActor* OwnerActor = GetOwner();
	APawn* InstigatorPawn = GetInstigator();
	
	if (IsValid(OwnerActor))
	{
		QueryParams.AddIgnoredActor(OwnerActor);
	}
	
	if (IsValid(InstigatorPawn))
	{
		QueryParams.AddIgnoredActor(InstigatorPawn);
	}
	
	FHitResult OcclusionHit;
	bool bBlocked = World->LineTraceSingleByChannel(
		OcclusionHit,
		TraceStart,
		TraceEnd,
		NSCollisionChannels::ExplosionTrace,
		QueryParams
	);
	
	// 초록 선: 벽에 가려지지 않은 대상
	// 빨간 선: 벽에 가려져 제외된 대상
	if (bDrawDebugSplashOcclusion)
	{
		DrawDebugLine(
			World,
			TraceStart,
			TraceEnd,
			bBlocked ? FColor::Red : FColor::Green,
			false,
			2.0f,
			0,
			1.5f
		);
	}
	
	return bBlocked;
}

void ANSHomingMissile::OnMissileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& HitResult)
{
	if (!HasAuthority()) return;
	ExecuteImpactCue(HitResult);
	
	const FVector ExplosionLocation = GetActorLocation();
	const FVector OcclusionTraceStart = ExplosionLocation;
	
	TArray<AActor*> FindTargetActors;
	
	FindSplashTargetActors(ExplosionLocation,FindTargetActors);
	FilterOccludedSplashTargets(OcclusionTraceStart, FindTargetActors);
	ApplyExplosionDamage(ExplosionLocation, FindTargetActors);
	
	Destroy();
}

void ANSHomingMissile::ExecuteImpactCue(const FHitResult& HitResult)
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
	
	SourceASC->ExecuteGameplayCue(ImpactCueTag, CueParameters);
}

void ANSHomingMissile::ApplyExplosionDamage(const FVector& ExplosionLocation, const TArray<AActor*>& TargetActors) const
{
	if (TargetActors.IsEmpty())
	{
		return;
	}
	
	if (!SourceASC || !DamageEffectClass)
	{
		NS_ACTOR_LOG(this, LogNSGAS, Warning,
			"스플래시 데미지를 건너 뜀. SourceASC유효={HasSourceASC}, 이펙트클래스유효={HasEffectClass}",
			("HasSourceASC", SourceASC != nullptr),
			("HasEffectClass", DamageEffectClass != nullptr)
		);
		
		return;
	}
	
	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	EffectContext.AddOrigin(ExplosionLocation);
	EffectContext.AddInstigator(SourceASC->GetAvatarActor(), const_cast<ANSHomingMissile*>(this));
	
	const FGameplayEffectSpecHandle DamageSpecHandle =
		SourceASC->MakeOutgoingSpec(
			DamageEffectClass,
			1.0f,
			EffectContext
		);
	
	if (!DamageSpecHandle.IsValid() || !DamageSpecHandle.Data.IsValid())
	{
		NS_ACTOR_LOG(this, LogNSGAS, Warning, "스플래시 데미지 스펙 생성에 실패");

		return;
	}
	
	// 대상별 거리 감쇠가 없으므로 하나의 Spec에 같은 폭발 데미지 값을 설정
	DamageSpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_Damage_Base, ExplosionDamage);
	
	NS_ACTOR_LOG(this, LogNSGAS, Log,
		"스플래시 데미지 SetByCaller 설정. Damage={Damage}, Tag={Tag}",
		("Damage", ExplosionDamage),
		("Tag", NSGameplayTags::Effect_Damage_Base.GetTag().ToString())
	);
	
	int32 AppliedCount = 0;
	AActor* SourceActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;

	if (!IsValid(SourceActor))
	{
		SourceActor = GetInstigator();
	}

	if (!IsValid(SourceActor))
	{
		SourceActor = GetOwner();
	}
	
	for (AActor* TargetActor : TargetActors)
	{
		if (!IsValid(TargetActor))
		{
			continue;
		}
		
		UAbilitySystemComponent* TargetASC = 
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		
		if (!TargetASC)
		{
			NS_ACTOR_LOG(TargetActor, LogNSGAS, Warning, "스플래시 대상에게 ASC가 없어 데미지를 적용안함");

			continue;
		}

		if (!NSDamageRules::CanApplyDamage(SourceActor, TargetActor))
		{
			continue;
		}
		
		// 서버 Projectile 충돌 결과로만 스플래시 데미지를 적용
		SourceASC->ApplyGameplayEffectSpecToTarget(
			*DamageSpecHandle.Data.Get(),
			TargetASC
		);
		
		++AppliedCount;
	}
	
	NS_ACTOR_LOG(this, LogNSGAS, Log,
		"스플래시 데미지 적용 완료. 적용수={AppliedCount}, 대상수={TargetCount}",
		("AppliedCount", AppliedCount),
		("TargetCount", TargetActors.Num())
	);
}

