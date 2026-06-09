// Copyright 2026 One Team. All rights reserved.


#include "NSDestructibleObjectBase.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSDestructibleAttributeSet.h"
#include "Net/UnrealNetwork.h"


ANSDestructibleObjectBase::ANSDestructibleObjectBase()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	AActor::SetReplicateMovement(false);
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMeshComp->SetupAttachment(Root);
	StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	GeometryCollectionComp = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryCollection"));
	GeometryCollectionComp->SetupAttachment(Root);
	GeometryCollectionComp->SetVisibility(false);
	GeometryCollectionComp->SetSimulatePhysics(false);
	GeometryCollectionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	Attributes = CreateDefaultSubobject<UNSDestructibleAttributeSet>(TEXT("Attributes"));
}

void ANSDestructibleObjectBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANSDestructibleObjectBase, bDestroyed);
	DOREPLIFETIME(ANSDestructibleObjectBase, DestroyServerTime);
}

void ANSDestructibleObjectBase::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystem->InitAbilityActorInfo(this, this);

	if (HasAuthority())
	{
		Attributes->InitMaxHealth(InitialHealth);
		Attributes->InitHealth(InitialHealth);

		// 파괴 판정은 서버 권위로만
		AbilitySystem->GetGameplayAttributeValueChangeDelegate(UNSDestructibleAttributeSet::GetHealthAttribute())
		             .AddUObject(this, &ANSDestructibleObjectBase::HandleHealthChanged);
	}
}

void ANSDestructibleObjectBase::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	// 서버에서만 바인딩됨
	if (Data.NewValue > 0.f || bDestroyed)
	{
		return;
	}

	bDestroyed = true;
	DestroyServerTime = (GetWorld()->GetGameState())
		                    ? GetWorld()->GetGameState()->GetServerWorldTimeSeconds()
		                    : GetWorld()->GetTimeSeconds();

	// 서브클래스 확장 훅(배럴: 방사형 데미지로 연쇄). 베이스/엄폐물은 아무 동작 없음.
	OnServerDestroyed(GetActorLocation());

	// 호스트(리슨 서버) 본인 비주얼. 원격 클라는 OnRep_Destroyed 가 담당.
	StartDestruction(0.f);

	// 잔해 표시 구간이 끝나면 액터 자체를 제거
	// 이후 새로 접속/relevant 된 클라에겐 액터가 아예 복제되지 않으므로
	// 별도 처리 없이 이미 사라진 상태가 됨
	FTimerHandle DestroyTimer;
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		FTimerDelegate::CreateLambda([this]() { Destroy(); }),
		DebrisLifetime,
		false);
}

void ANSDestructibleObjectBase::OnRep_Destroyed()
{
	if (!bDestroyed)
	{
		return;
	}

	const float Now = (GetWorld()->GetGameState())
		                  ? GetWorld()->GetGameState()->GetServerWorldTimeSeconds()
		                  : GetWorld()->GetTimeSeconds();

	// 옆에서 보던 클라는 Elapsed = 0, 멀리서 돌아온 클라는 그동안의 경과 시간이 들어온다.
	StartDestruction(Now - DestroyServerTime);
}

void ANSDestructibleObjectBase::StartDestruction(float Elapsed)
{
	if (bDestructionStarted)
	{
		return;
	}
	bDestructionStarted = true;

	StaticMeshComp->SetVisibility(false);
	StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 잔해 구간을 한참 지나 relevant 된 클라:
	// 비결정적인 정착 위치를 재현할 수 없으므로 잔해를 새로 시뮬레이션하지 않고
	// 숨김 상태로만 둔다(서버가 곧 액터를 제거함).
	if (Elapsed > SettleApproxTime)
	{
		GeometryCollectionComp->SetVisibility(false);
		return;
	}

	// 정상 파괴 연출: 스태틱 끄고 GC 켜서 분해 + 임팩트
	GeometryCollectionComp->SetVisibility(true);
	GeometryCollectionComp->SetCollisionProfileName(DebrisCollisionProfile);
	GeometryCollectionComp->SetSimulatePhysics(true);


	GeometryCollectionComp->CrumbleActiveClusters(); // 클러스터 강제 분해
	GeometryCollectionComp->AddRadialImpulse(
		GetActorLocation(),
		BreakImpulseRadius,
		BreakImpulseStrength,
		ERadialImpulseFalloff::RIF_Linear,
		false);

	// TODO: 파괴 VFX / SFX 트리거
}
