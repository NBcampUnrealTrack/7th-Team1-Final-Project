// Copyright 2026 One Team. All rights reserved.

#include "NSTurret.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SphereComponent.h"
#include "GenericTeamAgentInterface.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "NeoSanctum/Type/NSTeamTypes.h"

ANSTurret::ANSTurret()
{
	PrimaryActorTick.bCanEverTick = true;
	// 처음부터 Tick을 활성화 한 채로 시작하지 않기 위한 설정
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BaseMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMeshComponent"));
	BaseMeshComponent->SetupAttachment(SceneRoot);

	HeadPivotComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HeadPivotComponent"));
	HeadPivotComponent->SetupAttachment(BaseMeshComponent);

	HeadMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMeshComponent"));
	HeadMeshComponent->SetupAttachment(HeadPivotComponent);

	DetectionSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphereComponent"));
	DetectionSphereComponent->SetupAttachment(SceneRoot);
	DetectionSphereComponent->InitSphereRadius(DetectionRadius);
	DetectionSphereComponent->SetGenerateOverlapEvents(true);
}

void ANSTurret::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RotateHeadToTarget(DeltaSeconds);
}

void ANSTurret::BeginPlay()
{
	Super::BeginPlay();

	if (DetectionSphereComponent)
	{
		DetectionSphereComponent->SetSphereRadius(DetectionRadius);
		DetectionSphereComponent->OnComponentBeginOverlap.AddDynamic(
			this,
			&ThisClass::OnDetectionSphereBeginOverlap
		);
		DetectionSphereComponent->OnComponentEndOverlap.AddDynamic(
			this,
			&ThisClass::OnDetectionSphereEndOverlap
		);

		InitializeTargets();
	}

	const float RefreshInterval = FMath::Max(TargetRefreshInterval, 0.01f);
	GetWorldTimerManager().SetTimer(
		TargetRefreshTimerHandle,
		this,
		&ThisClass::UpdateAutoTarget,
		RefreshInterval,
		true
	);
	UpdateAutoTarget();
}

void ANSTurret::OnDetectionSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (IsValidTargetActor(OtherActor))
	{
		TargetSet.Add(OtherActor);
	}
}

void ANSTurret::OnDetectionSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		TargetSet.Remove(OtherActor);
	}
}

bool ANSTurret::IsValidTargetActor(const AActor* TargetActor) const
{
	if (!IsValid(TargetActor) || TargetActor == this)
	{
		return false;
	}

	const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(TargetActor);
	if (!TeamAgent || TeamAgent->GetGenericTeamId() != FGenericTeamId(static_cast<uint8>(ETeamId::Enemy)))
	{
		return false;
	}

	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(TargetActor);
	const UAbilitySystemComponent* TargetASC =
		AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;

	if (!TargetASC)
	{
		return false;
	}

	if (TargetASC->HasMatchingGameplayTag(NSGameplayTags::State_Dead))
	{
		return false;
	}

	return true;
}

void ANSTurret::InitializeTargets()
{
	if (!DetectionSphereComponent)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	DetectionSphereComponent->GetOverlappingActors(OverlappingActors);

	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (IsValidTargetActor(OverlappingActor))
		{
			TargetSet.Add(OverlappingActor);
		}
	}
}

void ANSTurret::UpdateAutoTarget()
{
	AActor* ClosestTarget = nullptr;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();

	for (TSet<TWeakObjectPtr<AActor>>::TIterator It(TargetSet); It; ++It)
	{
		AActor* TargetActor = It->Get();
		if (!IsValidTargetActor(TargetActor))
		{
			It.RemoveCurrent();
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(GetActorLocation(), TargetActor->GetActorLocation());
		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestTarget = TargetActor;
		}
	}

	AutoTarget = ClosestTarget;
	// 타겟이 있다면 Tick을 활성화해서 RotateHeadToTarget 로직이 돌아갈 수 있도록 함 
	// 타겟이 없으면 Tick을 비활성화.
	SetActorTickEnabled(AutoTarget.IsValid());
}

void ANSTurret::RotateHeadToTarget(float DeltaSeconds)
{
	AActor* TargetActor = AutoTarget.Get();
	if (!IsValidTargetActor(TargetActor) || !HeadPivotComponent)
	{
		return;
	}

	const FVector ToTarget = TargetActor->GetActorLocation() - HeadPivotComponent->GetComponentLocation();
	const FVector LocalDirection = HeadPivotComponent->GetAttachParent()
		? HeadPivotComponent->GetAttachParent()->GetComponentTransform().InverseTransformVectorNoScale(ToTarget)
		: ToTarget;

	const FVector FlatLocalDirection(LocalDirection.X, LocalDirection.Y, 0.0f);
	if (FlatLocalDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator DesiredRelativeRotation = FlatLocalDirection.Rotation();
	const FRotator NewRelativeRotation = FMath::RInterpConstantTo(
		HeadPivotComponent->GetRelativeRotation(),
		DesiredRelativeRotation,
		DeltaSeconds,
		HeadTurnSpeed
	);

	HeadPivotComponent->SetRelativeRotation(NewRelativeRotation);
}
