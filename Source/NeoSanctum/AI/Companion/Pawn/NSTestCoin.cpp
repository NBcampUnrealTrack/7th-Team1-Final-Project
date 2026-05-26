// Copyright 2026 One Team. All rights reserved.


#include "NSTestCoin.h"
#include "EngineUtils.h"
#include "NSDroneAI.h"
#include "DrawDebugHelpers.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Kismet/KismetSystemLibrary.h"


ANSTestCoin::ANSTestCoin()
{
	PrimaryActorTick.bCanEverTick = false;
	
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionComponent->SetupAttachment(RootComponent);
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	
	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>("StimuliSource");
	StimuliSource->bAutoRegister = true;
	StimuliSource->RegisterForSense(TSubclassOf<UAISense_Sight>(UAISense_Sight::StaticClass()));
	
	ProjectileMovementComponent->InitialSpeed = 0.f;
	ProjectileMovementComponent->MaxSpeed = 600.f;
	ProjectileMovementComponent->bRotationFollowsVelocity =true;
	ProjectileMovementComponent->ProjectileGravityScale = 0.f;
	
	CollisionComponent->SetSimulatePhysics(true);
	CollisionComponent->SetSphereRadius(200.f);
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ANSTestCoin::OnOverlapBegin);
}

void ANSTestCoin::CheckPlayerActor()
{
	// AI 도움 디버그 스피어 그리기
	DrawDebugSphere(
		GetWorld(),
		GetActorLocation(),  // 중심 위치
		MagneticRadius,               // SphereOverlapActors 반경과 동일하게
		16,                  // 세그먼트 수 (높을수록 부드러움)
		FColor::Green,       // 색상
		false,               // 영구 표시 여부
		0.5f                 // 표시 지속 시간 (타이머 간격과 맞추기)
	);
	
	// 감지할 오브젝트 타입 변수
	TArray<AActor*> OutActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel1));
	
	// 무시할 엑터
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this);
	
	// 범위 감지
	UKismetSystemLibrary::SphereOverlapActors(
	GetWorld(),
	GetActorLocation(),
	MagneticRadius,
	ObjectTypes,
	ANSDroneAI::StaticClass(),
	IgnoreActors,
	OutActors
	);
	
	// 찾은 엑터 판별
	for (AActor* FindActor : OutActors)
	{
		if (FindActor->ActorHasTag(TEXT("DroneAI")))
		{
			// AI도움 디버그 스피어 감지됐을 때 색상 변경으로 확인
			DrawDebugSphere(
				GetWorld(),
				GetActorLocation(),
				MagneticRadius,
				16,
				FColor::Red,  // 감지 시 빨간색
				false,
				0.5f
			);
			
			CollisionComponent->SetSimulatePhysics(false);
			
			// 추적 로직 활성화
			if (!IsValid(ProjectileMovementComponent)) return;
			ProjectileMovementComponent->bIsHomingProjectile = true;
			ProjectileMovementComponent->HomingAccelerationMagnitude = 3000.f;
			ProjectileMovementComponent->HomingTargetComponent = FindActor->GetRootComponent();
			ProjectileMovementComponent->Activate(true);
			// 추적시작 타이머 비활성화
			GetWorldTimerManager().ClearTimer(CheckPlayerTimerHandle);
			break;
		}
	}
}

void ANSTestCoin::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValid(OtherActor)) return;
	
	if (OtherActor->ActorHasTag(TEXT("DroneAI")))
	{
		Destroy();
	}
}

void ANSTestCoin::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<ANSDroneAIController> It(GetWorld()); It; ++It)
	{
		ANSDroneAIController* DroneAIC = *It;
		if (DroneAIC)
		{
			CacheDroneAIControllers.Add(DroneAIC);
		}
	}
	
	GetWorldTimerManager().SetTimer(CheckPlayerTimerHandle,this,&ThisClass::CheckPlayerActor,0.5f,true);
}

void ANSTestCoin::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	for (const TWeakObjectPtr<ANSDroneAIController>& CacheDroneAIController : CacheDroneAIControllers)
	{
		if (CacheDroneAIController.IsValid())
		{
			CacheDroneAIController->RemoveTargetCoin(this);
		}
	}
}


