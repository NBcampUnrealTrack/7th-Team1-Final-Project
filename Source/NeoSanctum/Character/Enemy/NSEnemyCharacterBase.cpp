// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"
#include "NeoSanctum/System/Component/NSDissolveComponent.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NeoSanctum/AI/Enemy/Controller/NSEnemyAIController.h"
#include "NeoSanctum/Collision/NSCollisionProfiles.h"
#include "NeoSanctum/Combat/Component/NSEnemyAttackComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyMeleeComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyTargetComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyWeaponComponent.h"
#include "NeoSanctum/Combat/HitReaction/NSHitReactionComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/System/Component/NSDamageFlashComponent.h"

ANSEnemyCharacterBase::ANSEnemyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCapsuleComponent()->SetCollisionProfileName(NSCollisionProfiles::EnemyCharacter);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->bUseControllerDesiredRotation = false;
	Movement->RotationRate = FRotator(0.0f, 360.0f, 0.0f);

	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UNSMonsterAttributeSet>(TEXT("AttributeSet"));

	DissolveComponent = CreateDefaultSubobject<UNSDissolveComponent>(TEXT("DissolveComponent"));
	WeaponComponent = CreateDefaultSubobject<UNSEnemyWeaponComponent>(TEXT("WeaponComponent"));
	DamageFlashComponent = CreateDefaultSubobject<UNSDamageFlashComponent>(TEXT("DamageFlashComponent"));
	HitReactionComponent = CreateDefaultSubobject<UNSHitReactionComponent>(TEXT("HitReactionComponent"));
	PhaseComponent = CreateDefaultSubobject<UNSEnemyPhaseComponent>(TEXT("PhaseComponent"));
	CoreComponent = CreateDefaultSubobject<UNSEnemyCoreComponent>(TEXT("CoreComponent"));
	AttackComponent = CreateDefaultSubobject<UNSEnemyAttackComponent>(TEXT("AttackComponent"));
	TargetComponent = CreateDefaultSubobject<UNSEnemyTargetComponent>(TEXT("TargetComponent"));
	ThreatComponent = CreateDefaultSubobject<UNSEnemyThreatComponent>(TEXT("ThreatComponent"));
	MeleeComponent = CreateDefaultSubobject<UNSEnemyMeleeComponent>(TEXT("MeleeComponent"));

	HitReactionComponent->SetTargetType(ENSHitFeedbackTargetType::Enemy);


	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 200.0f;
	GetCharacterMovement()->AvoidanceWeight = 0.5f;
}

void ANSEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (!ASC) return;

	ASC->InitAbilityActorInfo(this, this);

	InitializeFromData(true);

	if (CoreComponent)
	{
		CoreComponent->OnEnemyDataChanged.AddUObject(
			this,
			&ANSEnemyCharacterBase::HandleEnemyDataChanged);
	}

	// 디졸브 완료 콜백 바인딩
	if (DissolveComponent && HasAuthority())
	{
		DissolveComponent->OnDissolveComplete.BindUObject(this, &ANSEnemyCharacterBase::OnDissolveFinished);
	}

	if (HasAuthority())
	{
		OnHitGaugeThresholdReached.AddUObject(this, &ThisClass::HandleHitGaugeThresholdReached);
	}
}

void ANSEnemyCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANSEnemyCharacterBase, bIsDead);
	DOREPLIFETIME(ANSEnemyCharacterBase, bIsInPool);
	DOREPLIFETIME(ANSEnemyCharacterBase, bHasCombatAimTarget);
	DOREPLIFETIME(ANSEnemyCharacterBase, CombatAimTargetLocation);
	DOREPLIFETIME(ANSEnemyCharacterBase, bIsRetreating);
	DOREPLIFETIME(ANSEnemyCharacterBase, bIsHitReacting);
}

ANSEnemyWeaponBase* ANSEnemyCharacterBase::GetCurrentWeapon() const
{
	return WeaponComponent ? WeaponComponent->GetCurrentWeapon() : nullptr;
}

void ANSEnemyCharacterBase::SetCurrentAttackRow(const FNSEnemyAttackRow& InAttackRow)
{
	if (CombatComponent)
	{
		CombatComponent->SetAttackRow(InAttackRow);
	}
}

const FNSEnemyAttackRow* ANSEnemyCharacterBase::GetCurrentAttackRow() const
{
	return CombatComponent ? CombatComponent->GetAttackRow() : nullptr;
}

void ANSEnemyCharacterBase::ClearCurrentAttackRow()
{
	if (CombatComponent)
	{
		CombatComponent->ClearAttackRow();
	}
}

FVector ANSEnemyCharacterBase::GetAimLocation() const
{
	const USkeletalMeshComponent* MeshComponent = GetMesh();
	if (MeshComponent)
	{
		return MeshComponent->Bounds.Origin;
	}

	return GetActorLocation();
}

void ANSEnemyCharacterBase::Die()
{
	if (bIsDead) return;

	if (HasAuthority())
	{
		bIsDead = true;
		FinishHitReaction();
		ResetHitGauge();
		SetRetreating(false);
		ClearCurrentAttackRow();
		ClearCombatAimTarget();
		ApplyDeadVisual();
		// (이용호 추가) 죽을 때 게임모드에 알림
		AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
		if (GameMode && GameMode->Implements<UNSRunGameModeInterface>())
		{
			INSRunGameModeInterface::Execute_NotifyEnemyKilled(GameMode, this);
		}

		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->UnPossess();
			AIController->Destroy();
		}

		if (ASC && DeathAbilityClass)
		{
			ASC->TryActivateAbilityByClass(DeathAbilityClass);
		}
	}
}

void ANSEnemyCharacterBase::OnRep_bIsDead()
{
	if (bIsDead)
	{
		ApplyDeadVisual();
	}
	else
	{
		ApplyAliveVisual();
	}
}

void ANSEnemyCharacterBase::HandleEnemyDataChanged(UNSEnemyData* NewEnemyData)
{
	ApplyVisualData();
}

void ANSEnemyCharacterBase::ApplyDeadVisual()
{
	// 물리 캡슐 콜리전 비활성화
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

	if (GetMesh())
	{
		// 애니메이션 인스턴스 중단
		GetMesh()->bPauseAnims = true;

		// 콜리전 프로필을 Ragdoll로 변경
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));

		// 스켈레탈 메시의 물리 시뮬레이션을 활성화
		GetMesh()->SetSimulatePhysics(true);

		// 디졸브 효과 적용
		if (DissolveComponent)
		{
			DissolveComponent->StartDissolve();
		}
	}

	OnEnemyDead.Broadcast();
}

void ANSEnemyCharacterBase::ApplyVisualData()
{
	UNSEnemyData* EnemyData = GetEnemyData();
	if (!EnemyData || !GetMesh())
	{
		return;
	}

	if (EnemyData->SkeletalMesh)
	{
		GetMesh()->SetSkeletalMeshAsset(EnemyData->SkeletalMesh);
		InitializeRuntimeMaterials();
	}

	if (EnemyData->AnimClass)
	{
		GetMesh()->SetAnimInstanceClass(EnemyData->AnimClass);
	}

	SetActorScale3D(EnemyData->DrawScale);
}

void ANSEnemyCharacterBase::InitializeFromData(bool bFullInit)
{
	UNSEnemyData* EnemyData = GetEnemyData();
	if (!EnemyData)
	{
		return;
	}

	if (CoreComponent)
	{
		CoreComponent->InitializeFromData(
			bFullInit,
			AttributeSet,
			DeathAbilityClass);
	}

	if (bFullInit)
	{
		ApplyVisualData();

		if (WeaponComponent)
		{
			WeaponComponent->EquipWeapon();
		}
	}
}

void ANSEnemyCharacterBase::ApplyAliveVisual()
{
	SetActorHiddenInGame(false);

	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionProfileName(NSCollisionProfiles::EnemyCharacter);
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetSimulatePhysics(false);
		MeshComp->AttachToComponent(
			GetCapsuleComponent(),
			FAttachmentTransformRules::SnapToTargetIncludingScale);
		MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		MeshComp->SetCollisionProfileName(TEXT("CharacterMesh"));
		MeshComp->bPauseAnims = false;
	}

	if (DissolveComponent)
	{
		DissolveComponent->ResetDissolve();
	}
}

void ANSEnemyCharacterBase::OnDissolveFinished()
{
	if (!HasAuthority())
	{
		return;
	}

	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (GameMode && GameMode->Implements<UNSRunGameModeInterface>())
	{
		INSRunGameModeInterface::Execute_ReturnMonsterToPool(GameMode, this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("풀 매니저 없음, 풀 반환 실패"));
	}
}

void ANSEnemyCharacterBase::OnRep_bIsInPool()
{
	// 클라이언트에 남아있는 콜리전 정리
	SetActorHiddenInGame(bIsInPool);
	SetActorEnableCollision(!bIsInPool);
}

void ANSEnemyCharacterBase::UpdateCombatAimTarget(AActor* TargetActor)
{
	if (!HasAuthority() || !IsValid(TargetActor))
	{
		ClearCombatAimTarget();
		return;
	}

	FVector AimBoundsOrigin = TargetActor->GetActorLocation();
	FVector AimBoundsExtent = FVector::ZeroVector;

	if (const UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(TargetActor->GetRootComponent()))
	{
		AimBoundsOrigin = RootPrimitive->Bounds.Origin;
		AimBoundsExtent = RootPrimitive->Bounds.BoxExtent;
	}

	CombatAimTargetLocation = AimBoundsOrigin + FVector::UpVector * (AimBoundsExtent.Z * AimTargetZOffsetRatio);

	bHasCombatAimTarget = true;
}

void ANSEnemyCharacterBase::ClearCombatAimTarget()
{
	if (!HasAuthority())
	{
		return;
	}

	bHasCombatAimTarget = false;
	CombatAimTargetLocation = FVector::ZeroVector;
}

void ANSEnemyCharacterBase::SetEnemyData(UNSEnemyData* InEnemyData)
{
	if (!HasAuthority() || !InEnemyData || !CoreComponent)
	{
		return;
	}

	CoreComponent->SetEnemyData(InEnemyData);
}

void ANSEnemyCharacterBase::PrepareForReuse(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsInPool = false;
	bIsDead = false;
	bIsHitReacting = false;
	ClearCurrentAttackRow();
	ClearCombatAimTarget();

	SetRetreating(false);
	SetActorLocationAndRotation(
		SpawnLocation,
		SpawnRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	SetActorTickEnabled(true);
	SetActorEnableCollision(true);

	// 이동을 멈췄으므로 재가동
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
	}

	ApplyAliveVisual();

	// 종료할 때 전부 없앴으므로 전부 재주입
	InitializeFromData(true);

	// BT 정상 작동을 위해 AIControllerClass로 재빙의
	SpawnDefaultController();
}

void ANSEnemyCharacterBase::DeactivateForPool()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsInPool = true;
	SetRetreating(false);
	FinishHitReaction();
	ResetHitGauge();
	ClearCurrentAttackRow();
	ClearCombatAimTarget();

	// 이동 즉시 정지 및 비활성화
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}

	// 진행 중인 몽타주 정지
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* Anim = MeshComp->GetAnimInstance())
		{
			Anim->StopAllMontages(0.0f);
		}
	}

	// 살아있는 채로 반환된 경우 AI, 컨트롤러 정리
	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
		AICon->UnPossess();
		AICon->Destroy();
	}

	// GAS 정리(실행 중 어빌리티 취소,활성 이펙트 전부 제거,그랜트 해제)
	if (ASC)
	{
		ASC->CancelAllAbilities();
		ASC->RemoveActiveEffects(FGameplayEffectQuery());
		ASC->ClearAllAbilities();
	}

	if (WeaponComponent)
	{
		WeaponComponent->UnEquipWeapon();
	}

	if (DamageFlashComponent)
	{
		DamageFlashComponent->CancelFlash();
	}

	// 물리적 중지
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void ANSEnemyCharacterBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (!bNavLinkJumping)
	{
		return;
	}
	bNavLinkJumping = false;

	// NavMesh 밖 착지 복구: 가까운 NavMesh 지점으로 스냅
	if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		const FVector Here = GetActorLocation();
		FNavLocation NavLoc;
		if (NavSys->ProjectPointToNavigation(Here, NavLoc, FVector(100.0f, 100.0f, 200.0f)))
		{
			const UCapsuleComponent* Capsule = GetCapsuleComponent();
			const float Radius = Capsule ? Capsule->GetScaledCapsuleRadius() : 50.0f;
			const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 88.0f;

			// 캡슐 반경 이상 벗어났을 때만 보정 (정상 착지는 그대로 둠)
			if (FVector::DistSquared2D(Here, NavLoc.Location) > FMath::Square(Radius))
			{
				// 투영점은 바닥 높이, 캡슐 절반 높이만큼 올려 박힘 방지
				SetActorLocation(NavLoc.Location + FVector(0.0f, 0.0f, HalfHeight), false);
			}
		}
	}
}

void ANSEnemyCharacterBase::StartNavLinkJump(const FVector& DestPoint)
{
	bNavLinkJumping = true;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	// 점프가 순간의 실효 중력으로 계산
	FVector LaunchVelocity = FVector::ZeroVector;
	const bool bFoundVelocity =
		UGameplayStatics::SuggestProjectileVelocity_CustomArc(
			this,
			LaunchVelocity,
			GetActorLocation(),
			DestPoint,
			Move->GetGravityZ(), // 실효 중력
			0.5f);

	if (!bFoundVelocity)
	{
		return;
	}

	bNavLinkJumping = true;
	LaunchCharacter(LaunchVelocity, true, true); // XY/Z Override
}

void ANSEnemyCharacterBase::SetRetreating(bool bInRetreating)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsRetreating = bInRetreating;
}

float ANSEnemyCharacterBase::GetHitGauge() const
{
	return AttributeSet ? AttributeSet->GetHitGauge() : 0.0f;
}

float ANSEnemyCharacterBase::GetMaxHitGauge() const
{
	return AttributeSet ? AttributeSet->GetMaxHitGauge() : 0.0f;
}

void ANSEnemyCharacterBase::ResetHitGauge()
{
	if (!HasAuthority() || !AttributeSet)
	{
		return;
	}

	AttributeSet->ResetHitGauge();
}

void ANSEnemyCharacterBase::NotifyHitGaugeThresholdReached()
{
	if (!HasAuthority() || bIsDead || bIsInPool)
	{
		return;
	}

	OnHitGaugeThresholdReached.Broadcast();
}

void ANSEnemyCharacterBase::FinishHitReaction()
{
	if (!HasAuthority() || !bIsHitReacting)
	{
		return;
	}

	SetHitReactionState(false);
}

void ANSEnemyCharacterBase::HandleHitGaugeThresholdReached()
{
	UNSEnemyData* EnemyData = GetEnemyData();

	if (!HasAuthority() ||
		bIsDead ||
		bIsInPool ||
		bIsHitReacting ||
		!ASC ||
		!EnemyData ||
		!EnemyData->HitReactionAbilityClass)
	{
		return;
	}

	// 공격 GA 취소보다 먼저 상태를 설정해야 BT Task가 예약을 반환하지 않음
	SetHitReactionState(true);

	const bool bActivated = ASC->TryActivateAbilityByClass(EnemyData->HitReactionAbilityClass);

	if (!bActivated)
	{
		FinishHitReaction();
	}
}

void ANSEnemyCharacterBase::SetHitReactionState(bool bNewHitReacting)
{
	if (!HasAuthority() || bIsHitReacting == bNewHitReacting)
	{
		return;
	}

	bIsHitReacting = bNewHitReacting;
	ANSEnemyAIController* EnemyController = Cast<ANSEnemyAIController>(GetController());

	if (!EnemyController)
	{
		return;
	}

	if (bIsHitReacting)
	{
		EnemyController->HandleHitReactionStarted();
	}
	else
	{
		EnemyController->HandleHitReactionFinished();
	}
}

void ANSEnemyCharacterBase::InitializeRuntimeMaterials()
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	UNSEnemyData* EnemyData = GetEnemyData();

	if (!MeshComponent || !EnemyData)
	{
		return;
	}

	if (DamageFlashComponent)
	{
		DamageFlashComponent->ClearMaterialFlashTargets();
	}

	RuntimeVisualMaterials.Reset();

	TArray<UMaterialInstanceDynamic*> FlashTargets;

	for (const FNSEnemyMaterialDefinition& Definition : EnemyData->MaterialDefinitions)
	{
		const int32 MaterialIndex = MeshComponent->GetMaterialIndex(Definition.MaterialSlotName);

		if (MaterialIndex == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy material slot not found: %s"),
			       *Definition.MaterialSlotName.ToString());

			continue;
		}

		UMaterialInterface* InitialMaterial = Definition.InitialMaterial
			                                      ? Definition.InitialMaterial.Get()
			                                      : MeshComponent->GetMaterial(MaterialIndex);

		if (!InitialMaterial)
		{
			continue;
		}

		MeshComponent->SetMaterial(MaterialIndex, InitialMaterial);

		UMaterialInstanceDynamic* MID =
			MeshComponent->CreateDynamicMaterialInstance(MaterialIndex, InitialMaterial);

		if (!MID)
		{
			continue;
		}

		MID->SetVectorParameterValue(TEXT("MonsterTint"), Definition.MonsterTint);
		MID->SetScalarParameterValue(TEXT("HitFlashAmount"), 0.0f);

		RuntimeVisualMaterials.Add(MID);
		FlashTargets.Add(MID);
	}

	if (DamageFlashComponent)
	{
		DamageFlashComponent->SetMaterialFlashTargets(FlashTargets);
	}
}
