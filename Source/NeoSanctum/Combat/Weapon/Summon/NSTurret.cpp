// Copyright 2026 One Team. All rights reserved.

#include "NSTurret.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "GenericTeamAgentInterface.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NeoSanctum/Collision/NSCollisionProfiles.h"
#include "NeoSanctum/Combat/NSDamageRules.h"
#include "NeoSanctum/Combat/Component/NSMeleeAttackReservationComponent.h"
#include "NeoSanctum/Combat/HitReaction/NSHitReactionComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/GAS/AttributeSet/NSTurretAttributeSet.h"
#include "NeoSanctum/GAS/GameplayAbility/GA_ThrowProjectile.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/GAS/Stats/NSCombatStatComponent.h"
#include "NeoSanctum/System/Component/NSDissolveComponent.h"
#include "NeoSanctum/System/Component/NSDamageFlashComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_Cue.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AISense_Hearing.h"

namespace
{
	const FName TurretFireNoiseTag(TEXT("TurretFire"));
}

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
	HitCollisionComponent->SetCollisionProfileName(NSCollisionProfiles::PlayerTurret);

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
	DetectionSphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionSphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionSphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DetectionSphereComponent->SetCollisionResponseToChannel(NSCollisionChannels::Enemy, ECR_Overlap);
	DetectionSphereComponent->SetGenerateOverlapEvents(true);

	DissolveComponent = CreateDefaultSubobject<UNSDissolveComponent>(TEXT("DissolveComponent"));
	HitReactionComponent = CreateDefaultSubobject<UNSHitReactionComponent>(TEXT("HitReactionComponent"));
	HitReactionComponent->SetTargetType(ENSHitFeedbackTargetType::Turret);
	DamageFlashComponent = CreateDefaultSubobject<UNSDamageFlashComponent>(TEXT("DamageFlashComponent"));
	
	MeleeAttackReservationComponent = CreateDefaultSubobject<UNSMeleeAttackReservationComponent>(
			TEXT("MeleeAttackReservationComponent"));
	MeleeAttackReservationComponent->SetMaxConcurrentAttackers(4);
}

UAbilitySystemComponent* ANSTurret::GetAbilitySystemComponent() const
{
	return ASC;
}

void ANSTurret::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ANSTurret, bDeathPresentationStarted);
	DOREPLIFETIME(ANSTurret, bReplicatedAimActive);
	DOREPLIFETIME(ANSTurret, ReplicatedJointRelativeRotation);
	DOREPLIFETIME(ANSTurret, ReplicatedHeadRelativeRotation);
}

float ANSTurret::GetSpawnSurfaceOffset() const
{
	return HitCollisionComponent ? HitCollisionComponent->GetScaledCapsuleHalfHeight() : 0.0f;
}

void ANSTurret::InitializeTurret(
	const FNSTurretConfig& InConfig,
	APawn* InOwningPawn,
	AController* InOwningController,
	const TArray<FNSSetByCallerMagnitude>& InSetByCallerMagnitudes,
	const TArray<FNSCombatStatMagnitude>& InRuntimeStatMagnitudes)
{
	// 터렛을 소환한 Pawn, Controller 전달
	OwningPawn = InOwningPawn;
	OwningController = InOwningController;
	// 초기 Attribute GE에 사용할 payload 저장
	SetByCallerMagnitudes = InSetByCallerMagnitudes;
	RuntimeStatMagnitudes = InRuntimeStatMagnitudes;
	SourceAbilityTag = InConfig.SourceAbilityTag;

	if (const ANSPlayerState* NSPlayerState = InOwningPawn ? InOwningPawn->GetPlayerState<ANSPlayerState>() : nullptr)
	{
		OwningCombatStatComponent = NSPlayerState->GetCombatStatComponent();
	}
	
	if (OwningPawn)
	{
		SetOwner(OwningPawn);
		SetInstigator(OwningPawn);
	}
	
	TargetRefreshInterval = InConfig.TargetRefreshInterval;
	YawTurnSpeed = InConfig.YawTurnSpeed;
	PitchTurnSpeed = InConfig.PitchTurnSpeed;
	FireAngleTolerance = InConfig.FireAngleTolerance;
	DamageEffectClass = InConfig.DamageEffectClass;
	InitialAttributeEffectClass = InConfig.InitialAttributeEffectClass;
	
	InitializeAbilityActorInfo();
	BindAttributeChangeDelegates();
	ApplyInitialAttributeEffect();
	StartLifetimeTimer();
	
	// 
	if (HasActorBegunPlay())
	{
		RefreshDetectionRange();
		if (HasAuthority())
		{
			// 타겟 탐색은 서버에서만 수행하도록 옮기고, 클라이언트에 복제
			InitializeTargets();
			RestartTargetRefreshTimer();
			UpdateAutoTarget();
		}
	}
}

void ANSTurret::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		// 서버는 조준과 사격을 판정하고 회전값을 복제
		RotateJointToTarget(DeltaSeconds);
		RotateHeadToTarget(DeltaSeconds);
		TryFire();
	}
	else
	{
		// 클라이언트는 복제된 회전값으로 시각만 갱신
		ApplyReplicatedAimRotation(DeltaSeconds);
	}
}

void ANSTurret::BeginPlay()
{
	Super::BeginPlay();

	InitializeAbilityActorInfo();
	BindAttributeChangeDelegates();
	ApplyInitialAttributeEffect();
	StartLifetimeTimer();

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

		if (HasAuthority())
		{
			// 타겟 목록은 서버 권한으로만 관리
			InitializeTargets();
		}
	}

	if (HasAuthority())
	{
		// 타겟 갱신 타이머도 서버에서만
		RestartTargetRefreshTimer();
		UpdateAutoTarget();
	}
}

void ANSTurret::OnRep_DeathPresentationStarted()
{
	if (!bDeathPresentationStarted)
	{
		return;
	}
	
	ApplyDeathState();
	StartDeathPresentation();
}

void ANSTurret::OnRep_AimReplicationState()
{
	// 복제된 회전값을 클라이언트 보간 목표로 저장
	TargetJointRelativeRotation = ReplicatedJointRelativeRotation;
	TargetHeadRelativeRotation = ReplicatedHeadRelativeRotation;

	if (!HasAuthority())
	{
		SetActorTickEnabled(bReplicatedAimActive && !bDeathPresentationStarted);
	}
}

void ANSTurret::OnDetectionSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

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
	if (!HasAuthority())
	{
		return;
	}

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

bool ANSTurret::CanDamageHitActor(const AActor* HitActor) const
{
	if (!IsValid(HitActor) || HitActor == this)
	{
		return false;
	}

	// Player 팀 대상은 사격 피해에서 제외
	const IGenericTeamAgentInterface* TargetTeam = Cast<IGenericTeamAgentInterface>(HitActor);
	if (TargetTeam &&
		TargetTeam->GetGenericTeamId() == FGenericTeamId(static_cast<uint8>(ETeamId::Player)))
	{
		return false;
	}

	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(HitActor);
	const UAbilitySystemComponent* TargetASC =
		AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;

	return NSDamageRules::CanApplyDamage(this, HitActor);
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

	FGameplayEffectSpecHandle SpecHandle =
		ASC->MakeOutgoingSpec(InitialAttributeEffectClass, 1.0f, EffectContext);

	if (SpecHandle.IsValid())
	{
		// Attribute 초기화 값 주입
		ApplySetByCallerMagnitudes(SpecHandle);
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
	
	bInitialAttributeEffectApplied = true;
	
	RefreshDetectionRange();
}

void ANSTurret::ApplySetByCallerMagnitudes(FGameplayEffectSpecHandle& SpecHandle) const
{
	// SetByCaller payload를 GE Spec에 주입
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return;
	}

	for (const FNSSetByCallerMagnitude& SetByCallerMagnitude : SetByCallerMagnitudes)
	{
		if (!SetByCallerMagnitude.SetByCallerTag.IsValid())
		{
			continue;
		}

		SpecHandle.Data->SetSetByCallerMagnitude(
			SetByCallerMagnitude.SetByCallerTag,
			SetByCallerMagnitude.Magnitude
		);
	}
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
	
	// 사망처리 바인딩
	AttributeSet->OnOutOfHealth.RemoveAll(this);
	AttributeSet->OnOutOfHealth.AddUObject(this, &ANSTurret::HandleOutOfHealth);
	
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
	
	// 소환자 Ignore 처리
	if (OwningPawn)
	{
		QueryParams.AddIgnoredActor(OwningPawn);
	}

	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		NSCollisionChannels::CombatSight,
		QueryParams
	);

	return !bHit || HitResult.GetActor() == TargetActor;
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
	if (!HasAuthority())
	{
		return;
	}

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
	// 서버 타겟 유무를 클라이언트 조준 Tick 상태로 복제
	bReplicatedAimActive = AutoTarget.IsValid();
	// 타겟이 있다면 Tick을 활성화해서 RotateHeadToTarget 로직이 돌아갈 수 있도록 함 
	// 타겟이 없으면 Tick을 비활성화.
	SetActorTickEnabled(bReplicatedAimActive);
}

void ANSTurret::RestartTargetRefreshTimer()
{
	GetWorldTimerManager().ClearTimer(TargetRefreshTimerHandle);

	const float RefreshInterval = FMath::Max(TargetRefreshInterval, 0.01f);
	GetWorldTimerManager().SetTimer(
		TargetRefreshTimerHandle,
		this,
		&ThisClass::UpdateAutoTarget,
		RefreshInterval,
		true
	);
}

void ANSTurret::RotateJointToTarget(float DeltaSeconds)
{
	AActor* TargetActor = AutoTarget.Get();
	if (!IsValidTargetActor(TargetActor) || !JointPivotComponent)
	{
		return;
	}
	
	
	const FVector ToTarget = TargetActor->GetActorLocation() - JointPivotComponent->GetComponentLocation();
	const FVector LocalDirection =
		JointPivotComponent->GetAttachParent()
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
	// 서버에서 계산한 Joint 회전값 복제
	ReplicatedJointRelativeRotation = NewRelativeRotation;
}

void ANSTurret::RotateHeadToTarget(float DeltaSeconds)
{
	AActor* TargetActor = AutoTarget.Get();
	if (!IsValidTargetActor(TargetActor) || !HeadPivotComponent)
	{
		return;
	}

	const FVector ToTarget = TargetActor->GetActorLocation() - HeadPivotComponent->GetComponentLocation();
	const FVector LocalDirection =
		HeadPivotComponent->GetAttachParent()
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
	// 서버에서 계산한 Head 회전값 복제
	ReplicatedHeadRelativeRotation = NewRelativeRotation;
}

void ANSTurret::ApplyReplicatedAimRotation(float DeltaSeconds)
{
	if (!bReplicatedAimActive)
	{
		// 타겟이 없으면 클라이언트 시각 Tick 중지
		SetActorTickEnabled(false);
		return;
	}

	if (JointPivotComponent)
	{
		const FRotator NewJointRotation = FMath::RInterpConstantTo(
			JointPivotComponent->GetRelativeRotation(),
			TargetJointRelativeRotation,
			DeltaSeconds,
			YawTurnSpeed
		);
		JointPivotComponent->SetRelativeRotation(NewJointRotation);
	}

	if (HeadPivotComponent)
	{
		const FRotator NewHeadRotation = FMath::RInterpConstantTo(
			HeadPivotComponent->GetRelativeRotation(),
			TargetHeadRelativeRotation,
			DeltaSeconds,
			PitchTurnSpeed
		);
		HeadPivotComponent->SetRelativeRotation(NewHeadRotation);
	}
}

void ANSTurret::TryFire()
{
	if (!HasAuthority() || !AttributeSet)
	{
		return;
	}
	
	if (ASC->HasMatchingGameplayTag(NSGameplayTags::State_Dead))
	{
		return;
	}
	
	const float FireRate = GetCurrentFireRate();
	if (FireRate <= 0.0f)
	{
		return;
	}
	
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const float FireInterval = 1.0f / FireRate;
	
	// 발사 타이밍을 먼저 검사하고 아직 쿨다운 중인 경우에 return
	if (bHasFired && CurrentTime - LastFireTime < FireInterval)
	{
		return;
	}
	
	if (!CanFireToCurrentTarget())
	{
		return;
	}
	
	bHasFired = true;
	LastFireTime = CurrentTime;
	FireHitscan();
}

bool ANSTurret::CanFireToCurrentTarget() const
{
	const AActor* TargetActor = AutoTarget.Get();
	if (!IsValidTargetActor(TargetActor) || !AttributeSet || !HeadMeshComponent)
	{
		return false;
	}

	const float FireRate = GetCurrentFireRate();
	const float AttackRange = AttributeSet->GetAttackRange();
	if (FireRate <= 0.0f || AttackRange <= 0.0f)
	{
		return false;
	}

	const FTransform TraceTransform = GetTraceSocketTransform();
	const FVector TraceLocation = TraceTransform.GetLocation();
	const FVector TraceForward = TraceTransform.GetRotation().GetForwardVector();
	const FVector ToTarget = TargetActor->GetActorLocation() - TraceLocation;

	if (ToTarget.IsNearlyZero() || ToTarget.SizeSquared() > FMath::Square(AttackRange))
	{
		return false;
	}

	if (!CanSeeTarget(TargetActor))
	{
		return false;
	}

	const float AimDot = FVector::DotProduct(TraceForward, ToTarget.GetSafeNormal());
	const float AimAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(AimDot, -1.0f, 1.0f)));
	
	// 일정 각도 이내에 적이 있는 경우 발사 가능
	return AimAngle <= FireAngleTolerance;
}

void ANSTurret::FireHitscan()
{
	if (!GetWorld() || !AttributeSet)
	{
		return;
	}

	const FTransform TraceTransform = GetTraceSocketTransform();
	const FVector TraceStart = TraceTransform.GetLocation();
	
	ReportFireNoise(TraceStart);
	
	const FVector TraceForward = TraceTransform.GetRotation().GetForwardVector();
	
	const float AttackRange = AttributeSet->GetAttackRange();
	const float Accuracy = FMath::Clamp(AttributeSet->GetAccuracy(), 0.0f, 1.0f);
	const float SpreadAngle = FMath::Lerp(MaxSpreadAngle, 0.0f, Accuracy);
	
	const FVector ShotDirection = FMath::VRandCone(TraceForward, FMath::DegreesToRadians(SpreadAngle));
	const FVector TraceEnd = TraceStart + ShotDirection * AttackRange;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TurretFireHitscan), false, this);
	
	// 소환자 Ignore 처리
	if (OwningPawn)
	{
		QueryParams.AddIgnoredActor(OwningPawn);
	}

	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		NSCollisionChannels::PlayerWeaponTrace,
		QueryParams
	);
	
	// 히트된 대상에 GE Damage 적용
	if (bHit && CanDamageHitActor(HitResult.GetActor()))
	{
		AActor* TargetActor = HitResult.GetActor();

		if (IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(TargetActor))
		{
			UAbilitySystemComponent* TargetASC = TargetInterface->GetAbilitySystemComponent();
			UAbilitySystemComponent* TurretASC = GetAbilitySystemComponent();

			if (TargetASC && TurretASC && DamageEffectClass)
			{
				// GE 발생 정보 생성
				FGameplayEffectContextHandle EffectContext = TurretASC->MakeEffectContext();
				EffectContext.AddSourceObject(this);
				EffectContext.AddHitResult(HitResult);
				// 터렛 피해도 설치한 플레이어 화면에 데미지 숫자를 보여줌.
				EffectContext.AddInstigator(GetOwningPawn(), this);

				FGameplayEffectSpecHandle NewSpecHandle = TurretASC->MakeOutgoingSpec(
					DamageEffectClass, 1.0f, EffectContext);
				if (NewSpecHandle.IsValid())
				{
					// 소환자의 현재 CritChance/CritDamage를 전달해 크리티컬이 적용되게 함
					ApplyCritOverrideToSpec(NewSpecHandle);

					// 타겟 ASC에 GE 적용
					TurretASC->ApplyGameplayEffectSpecToTarget(*NewSpecHandle.Data.Get(), TargetASC);
				}
			}
		}
	}
	
	const FTransform MuzzleTransform = GetMuzzleTransform();
	
	// Gameplay Cue 재생
	FGameplayCueParameters MuzzleFireCueParameters;
	MuzzleFireCueParameters.Instigator = this;
	MuzzleFireCueParameters.EffectCauser = this;
	MuzzleFireCueParameters.Location = MuzzleTransform.GetLocation();
	MuzzleFireCueParameters.Normal = MuzzleTransform.GetRotation().GetForwardVector();
	
	FGameplayCueParameters BulletTrailCueParameters;
	BulletTrailCueParameters.Instigator = this;
	BulletTrailCueParameters.EffectCauser = this;
	BulletTrailCueParameters.Location = MuzzleTransform.GetLocation();;
	BulletTrailCueParameters.Normal = (TraceEnd - TraceStart).GetSafeNormal();
	BulletTrailCueParameters.RawMagnitude = AttackRange;
	
	FGameplayCueParameters ImpactCueParameters;
	ImpactCueParameters.Instigator = this;
	ImpactCueParameters.EffectCauser = this;
	ImpactCueParameters.Location = HitResult.ImpactPoint;
	ImpactCueParameters.Normal = HitResult.ImpactNormal;
	
	ASC->ExecuteGameplayCue(NSGameplayTags::GameplayCue_Engineer_SpawnTurret_MuzzleFire, MuzzleFireCueParameters);
	ASC->ExecuteGameplayCue(NSGameplayTags::GameplayCue_Engineer_SpawnTurret_BulletTrail, BulletTrailCueParameters);
	ASC->ExecuteGameplayCue(NSGameplayTags::GameplayCue_Engineer_SpawnTurret_Impact, ImpactCueParameters);
	
	// 임시 디버그 라인 처리. 쉬핑할 때는 제거.
#if !UE_BUILD_SHIPPING
	
	const FVector DebugEnd = bHit ? HitResult.ImpactPoint : TraceEnd;
	
	DrawDebugLine(
		GetWorld(),
		TraceStart,
		DebugEnd,
		bHit ? FColor::Red : FColor::Green,
		false,
		0.4f,
		0,
		1.0f
	);
#endif
}

void ANSTurret::ReportFireNoise(const FVector& NoiseLocation)
{
	if (!HasAuthority())
	{
		return;
	}
	
	float Loudness = 0.0f;
	
	if (!TryGetRuntimeStatMagnitude(NSGameplayTags::CombatStat_NoiseFireLoudness, Loudness))
	{
		return;
	}
	
	Loudness = FMath::Max(Loudness, 0.0f);
	
	if (Loudness <= 0.0f)
	{
		return;
	}
	
	// AI가 인식하는 소음 주체를 설치 플레이어가 아닌 터렛으로 유지.
	UAISense_Hearing::ReportNoiseEvent(
		this,
		NoiseLocation,
		Loudness,
		this,
		0.0f,
		TurretFireNoiseTag
	);
}

FTransform ANSTurret::GetMuzzleTransform() const
{
	if (HeadMeshComponent && HeadMeshComponent->DoesSocketExist(MuzzleSocketName))
	{
		return HeadMeshComponent->GetSocketTransform(MuzzleSocketName);
	}

	return HeadMeshComponent ? HeadMeshComponent->GetComponentTransform() : GetActorTransform();
}

FTransform ANSTurret::GetTraceSocketTransform() const
{
	if (HeadMeshComponent && HeadMeshComponent->DoesSocketExist(TraceSocketName))
	{
		return HeadMeshComponent->GetSocketTransform(TraceSocketName);
	}

	return HeadMeshComponent ? HeadMeshComponent->GetComponentTransform() : GetActorTransform();
}

void ANSTurret::HandleOutOfHealth()
{
	DeactivateTurret();
}

void ANSTurret::DeactivateTurret()
{
	if (bDeathPresentationStarted)
	{
		return;
	}
	
	ApplyDeathState();
	StartDeathPresentation();
}

void ANSTurret::ApplyDeathState()
{
	bReplicatedAimActive = false;
	SetActorTickEnabled(false);
	GetWorldTimerManager().ClearTimer(TargetRefreshTimerHandle);
	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);

	TargetSet.Empty();
	AutoTarget.Reset();

	if (HitCollisionComponent)
	{
		HitCollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HitCollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	if (DetectionSphereComponent)
	{
		DetectionSphereComponent->SetGenerateOverlapEvents(false);
		DetectionSphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (!ASC)
	{
		return;
	}
	
	// 임시 : 강제로 Dead 상태 태그 부여 -> 이후 GA_Death로 로직을 빼고 GE에서 변경하도록 할 예정 
	ASC->AddLooseGameplayTag(NSGameplayTags::State_Dead);
}

void ANSTurret::StartLifetimeTimer()
{
	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);

	float Duration = 0.0f;
	if (!TryGetRuntimeStatMagnitude(NSGameplayTags::CombatStat_Duration, Duration) || Duration <= 0.0f)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		LifetimeTimerHandle,
		this,
		&ThisClass::DeactivateTurret,
		Duration,
		false
	);
}

bool ANSTurret::TryGetRuntimeStatMagnitude(
	const FGameplayTag& CombatStatTag,
	float& OutMagnitude) const
{
	for (const FNSCombatStatMagnitude& RuntimeStatMagnitude : RuntimeStatMagnitudes)
	{
		if (RuntimeStatMagnitude.CombatStatTag == CombatStatTag)
		{
			OutMagnitude = RuntimeStatMagnitude.Magnitude;
			return true;
		}
	}

	return false;
}

float ANSTurret::GetCurrentFireRate() const
{
	float FireRate = 0.0f;

	if (OwningCombatStatComponent && SourceAbilityTag.IsValid() &&
		OwningCombatStatComponent->TryGetFinalAbilityStat(
			SourceAbilityTag, NSGameplayTags::CombatStat_FireRate, FireRate))
	{
		return FMath::Max(FireRate, 0.0f);
	}

	// 소환자 정보가 없으면 소환 시점 스냅샷으로 대체
	return AttributeSet ? FMath::Max(AttributeSet->GetFireRate(), 0.0f) : 0.0f;
}

void ANSTurret::ApplyCritOverrideToSpec(FGameplayEffectSpecHandle& SpecHandle) const
{
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return;
	}

	const ANSPlayerState* NSPlayerState = OwningPawn ? OwningPawn->GetPlayerState<ANSPlayerState>() : nullptr;
	const UNSPlayerAttributeSet* PlayerAttributeSet = NSPlayerState ? NSPlayerState->GetPlayerAttributeSet() : nullptr;

	// 소환자를 찾지 못하면 GEC 캡처 기본값(크리티컬 없음)이 그대로 적용됨
	if (!PlayerAttributeSet)
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(
		NSGameplayTags::Effect_Damage_CritChanceOverride, PlayerAttributeSet->GetCritChance());
	SpecHandle.Data->SetSetByCallerMagnitude(
		NSGameplayTags::Effect_Damage_CritDamageOverride, PlayerAttributeSet->GetCritDamage());
}

void ANSTurret::StartDeathPresentation()
{
	bDeathPresentationStarted = true;
	
	FGameplayCueParameters CueParameters;
	CueParameters.Instigator = this;
	CueParameters.EffectCauser = this;
	CueParameters.Location = this->GetActorLocation();
	CueParameters.Normal = this->GetActorForwardVector();

	ASC->ExecuteGameplayCue(NSGameplayTags::GameplayCue_Engineer_SpawnTurret_Deactivate, CueParameters);

	if (DissolveComponent)
	{
		DissolveComponent->StartDissolve(true);
	}
}
