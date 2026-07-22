// Copyright 2026 One Team. All rights reserved.

#include "NSBaseDroneAI.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "AbilitySystemComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "AIController.h"
#include "NeoSanctum/AI/Components/NSFlyingLocomotionComponent.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Data/AI/NSCompanionAbilitySet.h"
#include "NeoSanctum/Data/AI/NSBaseDroneDefinition.h"
#include "Net/UnrealNetwork.h"

ANSBaseDroneAI::ANSBaseDroneAI()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// AIController 자동빙의
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	SphereComponent = CreateDefaultSubobject<USphereComponent>("Collision");
	SetRootComponent(SphereComponent);
	SphereComponent->SetSimulatePhysics(false);
	
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMesh");
	SkeletalMeshComponent->SetupAttachment(SphereComponent);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// 무브먼트 컴포넌트는 여전히 폰이 소유 (로코모션 컴포넌트는 참조만 함)
	FlyingMovementComponent = CreateDefaultSubobject<UNSFlyingLocomotionComponent>("FloatingPawnMovement");
	FlyingMovementComponent->MaxSpeed = 1000.f;
	FlyingMovementComponent->Acceleration = 4000.f;
	FlyingMovementComponent->Deceleration = 4000.f;
	FlyingMovementComponent->TurningBoost = 8.f;
	
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	
	bReplicates = true;
	SetReplicateMovement(true);
}

UAbilitySystemComponent* ANSBaseDroneAI::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}


void ANSBaseDroneAI::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// 움직임 관련 로직은 전부 LocomotionComponent가 자체 Tick에서 처리
	// 폰 레벨의 추가 로직이 필요하면 여기에 작성
}

void ANSBaseDroneAI::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ANSBaseDroneAI, CurrentDefinition);
}

void ANSBaseDroneAI::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	if (!HasAuthority() || !IsValid(NewController)) return;
	
	AAIController* DroneAIController = Cast<AAIController>(NewController);
	if (!IsValid(DroneAIController)) return;
	CachedAIController = DroneAIController;
	
	InitializeFromData();
}

void ANSBaseDroneAI::BeginPlay()
{
	Super::BeginPlay();
	
	InitAbilityActorInfo();
}

void ANSBaseDroneAI::MoveTowards(const FVector& TargetLocation)
{
	// 서버 권한 확인 및 로코모션 컴포넌트 유효성 확인 후 위임
	if (!HasAuthority() || !IsValid(FlyingMovementComponent)) return;
	
	FlyingMovementComponent->RequestMoveTowards(TargetLocation);
}

void ANSBaseDroneAI::SetCurrentEnemy(AActor* InEnemy)
{
	// 폰 상태 갱신
	CurrentEnemy = InEnemy;
	
	// 로코모션 컴포넌트에 회전 타겟 전파 (nullptr 허용 - velocity 회전 복귀)
	if (IsValid(FlyingMovementComponent))
	{
		FlyingMovementComponent->SetRotationTarget(InEnemy);
	}
}

void ANSBaseDroneAI::SetPendingDefinition(const UNSBaseDroneDefinition* InDefinition)
{
	if (!InDefinition) return;
	
	CurrentDefinition = InDefinition;
}

void ANSBaseDroneAI::InitAbilityActorInfo()
{
	checkf(AbilitySystemComponent, TEXT("Can't Found ASC %s"), *GetName());
	
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

void ANSBaseDroneAI::InitializeDefaultStats()
{
	if (!HasAuthority()) return;
	
	if (!AbilitySystemComponent || !DefaultStatsEffect) return;
	
	FGameplayEffectContextHandle ContextHandle =
		AbilitySystemComponent->MakeEffectContext();
	
	FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(
		DefaultStatsEffect,
		1.f,
		ContextHandle
		);
	
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void ANSBaseDroneAI::GiveDefaultAbilities()
{
	if (!HasAuthority() || bDefaultAbilitiesGranted) return;
	
	if (!AbilitySystemComponent) return;
	
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (!AbilityClass) continue;
		
		FGameplayAbilitySpec AbilitySpec(
		AbilityClass,
		1,
		INDEX_NONE,
		this
		);
		
		AbilitySystemComponent->GiveAbility(AbilitySpec);
	}
	
	bDefaultAbilitiesGranted = true;
}

void ANSBaseDroneAI::ApplyDroneDefinition(const UNSBaseDroneDefinition* NewDefinition)
{
	if (!HasAuthority() || !NewDefinition) return;
	
	//@민재 TODO : 예외처리 생각하기
	/*if (CurrentDefinition == NewDefinition) return;
	UE_LOG(LogTemp, Warning, TEXT("CurrentDefinition == NewDefinition"));*/
	
	if (!IsValid(NewDefinition->AbilitySet)) return;
	
	CurrentAbilityHandles.TakeFromAbilitySystem(AbilitySystemComponent);
	
	NewDefinition->AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, &CurrentAbilityHandles, this);
	
	FGameplayEffectContextHandle ContextHandle =
	AbilitySystemComponent->MakeEffectContext();
	
	FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(
		NewDefinition->TypeStatsEffect,
		1.f,
		ContextHandle
		);
	
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
	
	ApplyDroneVisual(NewDefinition);
	
	CurrentDefinition = NewDefinition;
}

void ANSBaseDroneAI::ApplyDroneVisual(const UNSBaseDroneDefinition* NewDefinition)
{
	if (!NewDefinition) return;
	
	if (NewDefinition->Mesh.Get() != nullptr)
	{
		SkeletalMeshComponent->SetSkeletalMesh(NewDefinition->Mesh.Get());
	}
	else
	{
		const TSoftObjectPtr<USkeletalMesh> MeshToLoad = NewDefinition->Mesh;
		
		FStreamableManager& StreamableManager = UAssetManager::Get().GetStreamableManager();
		StreamableManager.RequestAsyncLoad(
			MeshToLoad.ToSoftObjectPath(),
			FStreamableDelegate::CreateWeakLambda(this, [this, MeshToLoad, NewDefinition]()
			{
				if (NewDefinition != CurrentDefinition) return;
				
				if (USkeletalMesh* Loaded = MeshToLoad.Get())
				{
					SkeletalMeshComponent->SetSkeletalMesh(Loaded);
				}
			})
		);
	}
}

void ANSBaseDroneAI::OnRep_CurrentDefinition()
{
	ApplyDroneVisual(CurrentDefinition);
}

void ANSBaseDroneAI::InitializeFromData()
{
	// 서브클래스에서 오버라이드
}