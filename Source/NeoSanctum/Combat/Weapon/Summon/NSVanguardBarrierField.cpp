// Copyright 2026 One Team. All rights reserved.

#include "NSVanguardBarrierField.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/OverlapResult.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NeoSanctum/Combat/NSDamageRules.h"

ANSVanguardBarrierField::ANSVanguardBarrierField()
{
	SphereBarrierCollisionComponent =
		CreateDefaultSubobject<USphereComponent>(TEXT("BarrierCollisionComponent"));
	SphereBarrierCollisionComponent->InitSphereRadius(DefaultRadius);
	InitializeBarrierCollisionComponent(SphereBarrierCollisionComponent);
}

void ANSVanguardBarrierField::InitializeBarrierField(
	const FNSShieldFieldTypeConfig& InConfig,
	APawn* InOwningPawn,
	AController* InOwningController,
	float InRadius,
	float InDuration,
	float InDamageInterval,
	const TArray<FNSSetByCallerMagnitude>& InSetByCallerMagnitudes)
{
	DamageEffectClass = InConfig.DamageEffectClass;
	DamageInterval = FMath::Max(InDamageInterval, 0.01f);
	DamageSetByCallerMagnitudes = InSetByCallerMagnitudes;

	// 체력, 반경, 지속시간 초기화
	InitializeBarrier(
		InOwningPawn,
		InOwningController,
		InRadius,
		InDuration,
		InConfig.InitialAttributeEffectClass,
		InSetByCallerMagnitudes
	);

	if (HasActorBegunPlay())
	{
		StartDamageTimer();
	}
}

void ANSVanguardBarrierField::BeginPlay()
{
	Super::BeginPlay();
	StartDamageTimer();
}

void ANSVanguardBarrierField::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DamageTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void ANSVanguardBarrierField::ApplyCollisionRadius(float Radius)
{
	if (SphereBarrierCollisionComponent)
	{
		SphereBarrierCollisionComponent->SetSphereRadius(Radius);
	}
}

void ANSVanguardBarrierField::StartDamageTimer()
{
	if (!HasAuthority() || !DamageEffectClass || DamageInterval <= 0.0f)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(DamageTimerHandle);
	GetWorldTimerManager().SetTimer(
		DamageTimerHandle,
		this,
		&ThisClass::ApplyPeriodicDamage,
		DamageInterval,
		true,
		0.0f
	);
}

void ANSVanguardBarrierField::ApplyPeriodicDamage()
{
	TArray<AActor*> TargetActors;
	FindDamageTargetActors(TargetActors);
	ApplyDamageToTargets(TargetActors);
}

void ANSVanguardBarrierField::FindDamageTargetActors(TArray<AActor*>& OutTargetActors) const
{
	OutTargetActors.Reset();

	UWorld* World = GetWorld();
	if (!World || CurrentRadius <= 0.0f)
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(VanguardBarrierFieldDamage), false, this);
	if (GetOwningPawn())
	{
		QueryParams.AddIgnoredActor(GetOwningPawn());
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	// 적 및 파괴 가능 대상만 피해 후보로 수집 : 파괴가능 대상도 피해후보로 해야 Stage2에서 사용할 수 있을 것 같음
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(NSCollisionChannels::Enemy);
	ObjectQueryParams.AddObjectTypesToQuery(NSCollisionChannels::DestructibleObject);

	TArray<FOverlapResult> OverlapResults;
	const bool bHasOverlap = World->OverlapMultiByObjectType(
		OverlapResults,
		GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(CurrentRadius),
		QueryParams
	);

	if (!bHasOverlap)
	{
		return;
	}

	TSet<AActor*> UniqueTargetActors;
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* TargetActor = OverlapResult.GetActor();
		if (IsValid(TargetActor) && TargetActor != this && TargetActor != GetOwningPawn())
		{
			UniqueTargetActors.Add(TargetActor);
		}
	}

	OutTargetActors.Reserve(UniqueTargetActors.Num());
	for (AActor* TargetActor : UniqueTargetActors)
	{
		OutTargetActors.Add(TargetActor);
	}
}

void ANSVanguardBarrierField::ApplyDamageToTargets(const TArray<AActor*>& TargetActors)
{
	if (TargetActors.IsEmpty() || !DamageEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponent();
	if (!SourceASC)
	{
		return;
	}

	AActor* SourceActor = GetOwningPawn() ? Cast<AActor>(GetOwningPawn()) : this;

	for (AActor* TargetActor : TargetActors)
	{
		if (!IsValid(TargetActor) || !NSDamageRules::CanApplyDamage(SourceActor, TargetActor))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (!TargetASC)
		{
			continue;
		}

		FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
		EffectContext.AddSourceObject(this);
		EffectContext.AddOrigin(GetActorLocation());
		// 피해 숫자와 공격 피드백은 소환자 기준
		EffectContext.AddInstigator(GetOwningPawn(), this);

		FGameplayEffectSpecHandle DamageSpecHandle =
			SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, EffectContext);
		if (!DamageSpecHandle.IsValid() || !DamageSpecHandle.Data.IsValid())
		{
			continue;
		}

		for (const FNSSetByCallerMagnitude& SetByCallerMagnitude : DamageSetByCallerMagnitudes)
		{
			if (SetByCallerMagnitude.SetByCallerTag.IsValid())
			{
				// GA_ThrowProjectile에서 계산된 데미지 payload 전달
				DamageSpecHandle.Data->SetSetByCallerMagnitude(
					SetByCallerMagnitude.SetByCallerTag,
					SetByCallerMagnitude.Magnitude
				);
			}
		}

		SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
	}
}
