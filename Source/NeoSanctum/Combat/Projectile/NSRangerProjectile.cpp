// Copyright 2026 One Team. All rights reserved.


#include "NSRangerProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
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

void ANSRangerProjectile::InitializeProjectile(
	UAbilitySystemComponent* InSourceASC,
	TSubclassOf<UGameplayEffect> InSplashDamageEffectClass,
	float InSplashDamageEffectLevel,
	float InSplashDamage,
	float InExplosionRadius)
{
	SourceASC = InSourceASC;
	SplashDamageEffectClass = InSplashDamageEffectClass;
	SplashDamageEffectLevel = InSplashDamageEffectLevel;
	SplashDamage = FMath::Max(InSplashDamage, 0.0f);
	ExplosionRadius = FMath::Max(InExplosionRadius, 0.0f);
	
	NS_ACTOR_LOG(this, LogNSGAS, Log,
		"Projectile 초기화 완료. Damage={Damage}, Radius={Radius}",
		("Damage", SplashDamage),
		("Radius", ExplosionRadius)
	);
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

void ANSRangerProjectile::FindSplashTargetActors(
	const FVector& ExplosionLocation, TArray<AActor*>& OutTargetActors) const
{
	OutTargetActors.Reset();
	
	UWorld* World = GetWorld();
	
	if (!World || ExplosionRadius <= 0.0f)
	{
		return;
	}
	
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RangerProjectileSplash), false);
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

void ANSRangerProjectile::FilterOccludedSplashTargets(
	const FVector& TraceStart, TArray<AActor*>& TargetActors) const
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

bool ANSRangerProjectile::IsSplashTargetOccluded(const FVector& TraceStart, const AActor* TargetActor) const
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
	
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RangerProjectileSplashOcclusion), false);
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
	const bool bBlocked = World->LineTraceSingleByObjectType(
		OcclusionHit,
		TraceStart,
		TraceEnd,
		ObjectQueryParams,
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
	
	ExecuteImpactCue(HitResult);
	
	FVector ExplosionLocation = GetActorLocation();
	FVector OcclusionTraceStart = ExplosionLocation;
	
	if (HitResult.bBlockingHit)
	{
		ExplosionLocation = FVector(HitResult.ImpactPoint);
		OcclusionTraceStart =
			ExplosionLocation + FVector(HitResult.ImpactNormal) * SplashOcclusionTraceStartOffset;
	}
	
	TArray<AActor*> SplashTargetActors;
	FindSplashTargetActors(ExplosionLocation, SplashTargetActors);
	
	const int32 FoundTargetCount = SplashTargetActors.Num();
	
	FilterOccludedSplashTargets(OcclusionTraceStart, SplashTargetActors);
	
	NS_ACTOR_LOG(this, LogNSGAS, Log,
		"스플래시 대상 검색 완료. 검색수={FoundCount}, 유효대상수={ValidCount}, 반경={Radius}",
		("FoundCount", FoundTargetCount),
		("ValidCount", SplashTargetActors.Num()),
		("Radius", ExplosionRadius)
	);
	
	ApplySplashDamage(ExplosionLocation, SplashTargetActors);
	
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

void ANSRangerProjectile::ApplySplashDamage(const FVector& ExplosionLocation, const TArray<AActor*>& TargetActors) const
{
	if (TargetActors.IsEmpty())
	{
		return;
	}
	
	if (!SourceASC || !SplashDamageEffectClass)
	{
		NS_ACTOR_LOG(this, LogNSGAS, Warning,
			"스플래시 데미지를 건너 뜀. SourceASC유효={HasSourceASC}, 이펙트클래스유효={HasEffectClass}",
			("HasSourceASC", SourceASC != nullptr),
			("HasEffectClass", SplashDamageEffectClass != nullptr)
		);
		
		return;
	}
	
	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	EffectContext.AddOrigin(ExplosionLocation);
	
	const FGameplayEffectSpecHandle DamageSpecHandle =
		SourceASC->MakeOutgoingSpec(
			SplashDamageEffectClass,
			SplashDamageEffectLevel,
			EffectContext
		);
	
	if (!DamageSpecHandle.IsValid() || !DamageSpecHandle.Data.IsValid())
	{
		NS_ACTOR_LOG(this, LogNSGAS, Warning, "스플래시 데미지 스펙 생성에 실패");

		return;
	}
	
	int32 AppliedCount = 0;
	
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
