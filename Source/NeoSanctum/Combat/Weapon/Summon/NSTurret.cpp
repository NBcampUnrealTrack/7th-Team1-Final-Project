// Copyright 2026 One Team. All rights reserved.

#include "NSTurret.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GenericTeamAgentInterface.h"
#include "NeoSanctum/GAS/AttributeSet/NSTurretAttributeSet.h"
#include "NeoSanctum/GAS/GameplayAbility/GA_ThrowProjectile.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "NeoSanctum/Type/NSTeamTypes.h"

ANSTurret::ANSTurret()
{
	PrimaryActorTick.bCanEverTick = true;
	// 처음부터 Tick을 활성화 한 채로 시작하지 않기 위한 설정
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	SetReplicateMovement(true);

	ASC = CreateDefaultSubobject<UNSAbilitySystemComponent>(TEXT("ASC"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UNSTurretAttributeSet>(TEXT("AttributeSet"));

	HitCollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HitCollisionComponent"));
	SetRootComponent(HitCollisionComponent);
	HitCollisionComponent->InitCapsuleSize(50.0f, 100.0f);
	HitCollisionComponent->SetCollisionProfileName(TEXT("Pawn"));

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetupAttachment(HitCollisionComponent);
	SceneRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -HitCollisionComponent->GetUnscaledCapsuleHalfHeight()));

	BaseMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMeshComponent"));
	BaseMeshComponent->SetupAttachment(SceneRoot);
	
	JointPivotComponent = CreateDefaultSubobject<USceneComponent>(TEXT("JointPivotComponent"));
	JointPivotComponent->SetupAttachment(BaseMeshComponent);
	
	JointMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("JointMeshComponent"));
	JointMeshComponent->SetupAttachment(JointPivotComponent);
	
	HeadPivotComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HeadPivotComponent"));
	HeadPivotComponent->SetupAttachment(JointMeshComponent);

	HeadMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMeshComponent"));
	HeadMeshComponent->SetupAttachment(HeadPivotComponent);

	DetectionSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphereComponent"));
	DetectionSphereComponent->SetupAttachment(SceneRoot);
	DetectionSphereComponent->InitSphereRadius(0.0f);
	DetectionSphereComponent->SetGenerateOverlapEvents(true);
}

UAbilitySystemComponent* ANSTurret::GetAbilitySystemComponent() const
{
	return ASC;
}

float ANSTurret::GetSpawnSurfaceOffset() const
{
	return HitCollisionComponent ? HitCollisionComponent->GetScaledCapsuleHalfHeight() : 0.0f;
}

void ANSTurret::InitializeTurret(const FNSTurretConfig& InConfig, APawn* InOwningPawn, AController* InOwningController)
{
	OwningPawn = InOwningPawn;
	OwningController = InOwningController;

	if (OwningPawn)
	{
		SetOwner(OwningPawn);
		SetInstigator(OwningPawn);
	}
	
	TargetRefreshInterval = InConfig.TargetRefreshInterval;
	YawTurnSpeed = InConfig.YawTurnSpeed;
	PitchTurnSpeed = InConfig.PitchTurnSpeed;
	InitialAttributeEffectClass = InConfig.InitialAttributeEffectClass;
	
	InitializeAbilityActorInfo();
	BindAttributeChangeDelegates();
	ApplyInitialAttributeEffect();
}

void ANSTurret::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RotateJointToTarget(DeltaSeconds);
	RotateHeadToTarget(DeltaSeconds);
}

void ANSTurret::BeginPlay()
{
	Super::BeginPlay();

	InitializeAbilityActorInfo();
	BindAttributeChangeDelegates();
	ApplyInitialAttributeEffect();

	if (DetectionSphereComponent)
	{
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

void ANSTurret::InitializeAbilityActorInfo()
{
	if (!ASC || bAbilityActorInfoInitialized)
	{
		return;
	}
	
	ASC->InitAbilityActorInfo(this, this);
	bAbilityActorInfoInitialized = true;
}

void ANSTurret::ApplyInitialAttributeEffect()
{
	if (!HasAuthority() || !ASC || !InitialAttributeEffectClass || bInitialAttributeEffectApplied)
	{
		return;
	}
	
	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = 
		ASC->MakeOutgoingSpec(InitialAttributeEffectClass,1.0f,EffectContext);

	if (SpecHandle.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
	
	bInitialAttributeEffectApplied = true;
	
	RefreshDetectionRange();
}

void ANSTurret::BindAttributeChangeDelegates()
{
	if (!ASC || !AttributeSet || bAttributeChangeDelegatesBound)
	{
		return;
	}
	
	ASC->GetGameplayAttributeValueChangeDelegate(
		UNSTurretAttributeSet::GetDetectionRangeAttribute()
	).AddUObject(this, &ThisClass::HandleDetectionRangeChanged);
	
	bAttributeChangeDelegatesBound = true;
}

void ANSTurret::HandleDetectionRangeChanged(const FOnAttributeChangeData& Data)
{
	if (DetectionSphereComponent)
	{
		DetectionSphereComponent->SetSphereRadius(FMath::Max(Data.NewValue, 0.0f));
	}
}

void ANSTurret::RefreshDetectionRange()
{
	if (!DetectionSphereComponent || !AttributeSet)
	{
		return;
	}
	
	const float NewDetectionRange = AttributeSet->GetDetectionRange();
	if (NewDetectionRange > 0.0f)
	{
		DetectionSphereComponent->SetSphereRadius(NewDetectionRange);
	}
}

bool ANSTurret::CanSeeTarget(const AActor* TargetActor) const
{
	if (!IsValid(TargetActor) || !GetWorld())
	{
		return false;
	}

	const FVector TraceStart = HeadMeshComponent ? HeadMeshComponent->GetComponentLocation() : GetActorLocation();
	const FVector TraceEnd = TargetActor->GetActorLocation();

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TurretLineOfSight), false, this);
	if (OwningPawn)
	{
		QueryParams.AddIgnoredActor(OwningPawn);
	}

	FHitResult HitResult;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	return bHit && HitResult.GetActor() == TargetActor;
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

		if (!CanSeeTarget(TargetActor))
		{
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

void ANSTurret::RotateJointToTarget(float DeltaSeconds)
{
	AActor* TargetActor = AutoTarget.Get();
	if (!IsValidTargetActor(TargetActor) || !JointPivotComponent)
	{
		return;
	}

	const FVector ToTarget = TargetActor->GetActorLocation() - JointPivotComponent->GetComponentLocation();
	const FVector LocalDirection = JointPivotComponent->GetAttachParent()
		? JointPivotComponent->GetAttachParent()->GetComponentTransform().InverseTransformVectorNoScale(ToTarget)
		: ToTarget;

	const FVector FlatLocalDirection(LocalDirection.X, LocalDirection.Y, 0.0f);
	if (FlatLocalDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator DesiredRelativeRotation = FlatLocalDirection.Rotation();
	const FRotator NewRelativeRotation = FMath::RInterpConstantTo(
		JointPivotComponent->GetRelativeRotation(),
		DesiredRelativeRotation,
		DeltaSeconds,
		YawTurnSpeed
	);

	JointPivotComponent->SetRelativeRotation(NewRelativeRotation);
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

	if (LocalDirection.IsNearlyZero())
	{
		return;
	}

	const float DesiredPitch = LocalDirection.Rotation().Pitch;
	const FRotator DesiredRelativeRotation(DesiredPitch, 0.0f, 0.0f);
	const FRotator NewRelativeRotation = FMath::RInterpConstantTo(
		HeadPivotComponent->GetRelativeRotation(),
		DesiredRelativeRotation,
		DeltaSeconds,
		PitchTurnSpeed
	);

	HeadPivotComponent->SetRelativeRotation(NewRelativeRotation);
}
