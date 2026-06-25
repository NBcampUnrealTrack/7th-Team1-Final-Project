// Copyright 2026 One Team. All rights reserved.


#include "NSGrenade.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NeoSanctum/Combat/NSDamageRules.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"

ANSGrenade::ANSGrenade()
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

void ANSGrenade::InitializeGrenade(const FNSExplosiveTypeConfig& InConfig)
{
	DamageEffectClass = InConfig.DamageEffectClass;

	// 설정 태그가 비어 있으면 수류탄 전용 기본 GameplayCue를 사용
	ExplosionCueTag = InConfig.ExplosionCueTag.IsValid()
		? InConfig.ExplosionCueTag
		: NSGameplayTags::GameplayCue_Ranger_Grenade_Explosion;
	FuseTime = InConfig.FuseTime;
	bExplodeOnImpact = InConfig.bExplodeOnImpact;
	LifeSpanAfterThrow = InConfig.LifeSpanAfterThrow;

	// DataTable이 float만 지원하므로 0 초과 값을 true로 해석해 충돌 즉시 폭발 여부를 덮어씀
	bool bRuntimeExplodeOnImpact = false;
	if (TryGetRuntimeStatBool(NSGameplayTags::CombatStat_bExplodeOnImpact, bRuntimeExplodeOnImpact))
	{
		bExplodeOnImpact = bRuntimeExplodeOnImpact;
	}

	if (LifeSpanAfterThrow > 0.0f)
	{
		SetLifeSpan(LifeSpanAfterThrow);
	}

	if (HasAuthority() && FuseTime > 0.0f)
	{
		// BeginPlay에서 기본 타이머가 이미 걸렸을 수 있으므로 설정 주입 후 타이머를 다시 잡음
		GetWorldTimerManager().ClearTimer(FuseTimerHandle);
		GetWorldTimerManager().SetTimer(
			FuseTimerHandle,
			this,
			&ThisClass::Explode,
			FuseTime,
			false
		);
	}
}

void ANSGrenade::BeginPlay()
{
	Super::BeginPlay();

	if (LifeSpanAfterThrow > 0.0f)
	{
		SetLifeSpan(LifeSpanAfterThrow);
	}

	if (UProjectileMovementComponent* Movement = GetProjectileMovementComponent())
	{
		Movement->OnProjectileBounce.AddDynamic(this, &ThisClass::OnProjectileBounce);
	}

	if (HasAuthority() && FuseTime > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			FuseTimerHandle,
			this,
			&ThisClass::Explode,
			FuseTime,
			false
		);
	}
}

void ANSGrenade::OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (!HasAuthority() || !bExplodeOnImpact)
	{
		return;
	}

	const FVector ExplosionLocation =
		ImpactResult.bBlockingHit ? FVector(ImpactResult.ImpactPoint) : GetActorLocation();

	// 벽 표면에서 폭발한 경우 벽 뒤 대상이 맞지 않도록 표면 바깥쪽에서 차폐 Trace를 시작
	const FVector OcclusionTraceStart =
		ImpactResult.bBlockingHit
			? ExplosionLocation + ImpactResult.ImpactNormal.GetSafeNormal() * ExplosionOcclusionTraceStartOffset
			: ExplosionLocation;
	const FVector ExplosionNormal =
		ImpactResult.bBlockingHit ? ImpactResult.ImpactNormal.GetSafeNormal() : FVector::UpVector;

	ExplodeAt(ExplosionLocation, ExplosionNormal, OcclusionTraceStart);
}

void ANSGrenade::Explode()
{
	ExplodeAt(GetActorLocation(), FVector::UpVector, GetActorLocation());
}

void ANSGrenade::ExplodeAt(
	const FVector& ExplosionLocation,
	const FVector& ExplosionNormal,
	const FVector& OcclusionTraceStart)
{
	if (bExploded || !HasAuthority())
	{
		return;
	}

	bExploded = true;
	GetWorldTimerManager().ClearTimer(FuseTimerHandle);

	ReportExplosionNoise(ExplosionLocation);
	
	// 데미지 적용 여부와 무관하게 폭발 시각/청각 연출부터 먼저 실행
	ExecuteExplosionCue(ExplosionLocation, ExplosionNormal);

	if (!DamageEffectClass || !GetWorld())
	{
		Destroy();
		return;
	}

	TArray<AActor*> TargetActors;

	// 반경 후보 수집 후 벽 뒤에 가려진 대상을 제외하고 남은 대상들에만 데미지 적용
	FindExplosionTargetActors(ExplosionLocation, TargetActors);
	FilterOccludedExplosionTargets(OcclusionTraceStart, TargetActors);
	ApplyExplosionDamage(ExplosionLocation, TargetActors);

	Destroy();
}

void ANSGrenade::ReportExplosionNoise(const FVector& ExplosionLocation) const
{
	APawn* NoiseInstigator = GetOwningPawn();
	
	if (!HasAuthority() || !IsValid(NoiseInstigator))
	{
		return;
	}
	
	float ExplosionNoiseLoudness = 0.0f;
	
	if (!TryGetRuntimeStatMagnitude(
		NSGameplayTags::CombatStat_NoiseExplosionLoudness, ExplosionNoiseLoudness))
	{
		return;
	}
	
	ExplosionNoiseLoudness = FMath::Max(ExplosionNoiseLoudness, 0.0f);
	
	if (ExplosionNoiseLoudness <= 0.0f)
	{
		return;
	}
	
	NoiseInstigator->MakeNoise(ExplosionNoiseLoudness, NoiseInstigator,	ExplosionLocation);
}

void ANSGrenade::ExecuteExplosionCue(const FVector& ExplosionLocation, const FVector& ExplosionNormal)
{
	if (!ExplosionCueTag.IsValid())
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
	CueParameters.EffectCauser = this;
	CueParameters.Location = ExplosionLocation;
	CueParameters.Normal = ExplosionNormal;

	SourceASC->ExecuteGameplayCue(ExplosionCueTag, CueParameters);
}

void ANSGrenade::FindExplosionTargetActors(
	const FVector& ExplosionLocation,
	TArray<AActor*>& OutTargetActors) const
{
	OutTargetActors.Reset();

	UWorld* World = GetWorld();
	const float RuntimeExplosionRadius = GetExplosionRadius();

	if (!World || RuntimeExplosionRadius <= 0.0f)
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GrenadeExplosion), false, this);
	if (GetOwningPawn())
	{
		QueryParams.AddIgnoredActor(GetOwningPawn());
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(NSCollisionChannels::Player);
	ObjectQueryParams.AddObjectTypesToQuery(NSCollisionChannels::Enemy);
	ObjectQueryParams.AddObjectTypesToQuery(NSCollisionChannels::DestructibleObject);
	ObjectQueryParams.AddObjectTypesToQuery(NSCollisionChannels::PlayerConstruct);

	const bool bHasOverlap = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		ExplosionLocation,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(RuntimeExplosionRadius),
		QueryParams
	);

	if (bDrawDebugExplosion)
	{
		// ProjectileShot과 동일하게 실제 판정 반경을 시각화
		DrawDebugSphere(
			World,
			ExplosionLocation,
			RuntimeExplosionRadius,
			24,
			FColor::Red,
			false,
			2.0f
		);
	}

	if (bHasOverlap)
	{
		TSet<AActor*> UniqueTargetActors;

		for (const FOverlapResult& OverlapResult : OverlapResults)
		{
			AActor* TargetActor = OverlapResult.GetActor();
			if (!IsValid(TargetActor) ||
				TargetActor == this ||
				TargetActor == GetOwningPawn())
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
	}
}

void ANSGrenade::FilterOccludedExplosionTargets(
	const FVector& TraceStart,
	TArray<AActor*>& TargetActors) const
{
	if (!bExcludeOccludedExplosionTargets || TargetActors.IsEmpty())
	{
		return;
	}

	TargetActors.RemoveAll([this, &TraceStart](const AActor* TargetActor)
	{
		return IsExplosionTargetOccluded(TraceStart, TargetActor);
	});
}

bool ANSGrenade::IsExplosionTargetOccluded(const FVector& TraceStart, const AActor* TargetActor) const
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

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GrenadeExplosionOcclusion), false);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(TargetActor);

	if (GetOwningPawn())
	{
		QueryParams.AddIgnoredActor(GetOwningPawn());
	}

	FHitResult OcclusionHit;
	const FVector TraceEnd = TargetActor->GetActorLocation();
	bool bBlocked = World->LineTraceSingleByChannel(
		OcclusionHit,
		TraceStart,
		TraceEnd,
		NSCollisionChannels::ExplosionTrace,
		QueryParams
	);

	if (bDrawDebugExplosionOcclusion)
	{
		// 초록: 데미지 적용 가능, 빨강: 구조물에 가려져 제외
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

void ANSGrenade::ApplyExplosionDamage(
	const FVector& ExplosionLocation,
	const TArray<AActor*>& TargetActors)
{
	UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwningPawn());

	if (TargetActors.IsEmpty() || !SourceASC || !DamageEffectClass)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	EffectContext.AddOrigin(ExplosionLocation);
	EffectContext.AddInstigator(GetOwningPawn(), this);

	FGameplayEffectSpecHandle SpecHandle =
		SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, EffectContext);

	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return;
	}

	for (const FNSSetByCallerMagnitude& SetByCallerMagnitude : GetSetByCallerMagnitudes())
	{
		if (SetByCallerMagnitude.SetByCallerTag.IsValid())
		{
			// GA에서 계산한 데미지 등 SetByCaller 값을 폭발 데미지 GE에 그대로 전달
			SpecHandle.Data->SetSetByCallerMagnitude(
				SetByCallerMagnitude.SetByCallerTag,
				SetByCallerMagnitude.Magnitude
			);
		}
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
			continue;
		}

		if (!NSDamageRules::CanApplyDamage(GetOwningPawn(), TargetActor))
		{
			continue;
		}

		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

float ANSGrenade::GetExplosionRadius() const
{
	float RuntimeExplosionRadius = 0.0f;
	if (TryGetRuntimeStatMagnitude(NSGameplayTags::CombatStat_ExplosionRadius, RuntimeExplosionRadius))
	{
		return RuntimeExplosionRadius;
	}

	// 폭발 반경은 DataTable/RuntimeStat 기반으로만 사용하므로 값이 없으면 판정을 수행하지 않음
	return 0.0f;
}
