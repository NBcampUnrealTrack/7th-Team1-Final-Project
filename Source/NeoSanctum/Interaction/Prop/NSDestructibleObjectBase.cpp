// Copyright 2026 One Team. All rights reserved.


#include "NSDestructibleObjectBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
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
	DOREPLIFETIME(ANSDestructibleObjectBase, ImpactAnchor);
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
	
	ImpactAnchor = GetActorLocation();
	
	const FGameplayEffectContextHandle& Ctx = Attributes->LastDamageContext;
	if (Ctx.IsValid())
	{
		if (const FHitResult* Hit = Ctx.GetHitResult())
		{
			ImpactAnchor = Hit->ImpactPoint;
		}
		else if (const AActor* Causer = Ctx.GetEffectCauser())
		{
			ImpactAnchor = Causer->GetActorLocation();
		}
	}
	
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
	
	// 클러스터 강제 분해
	GeometryCollectionComp->CrumbleActiveClusters();
	
	//타격자 위치방향에서 임펄스 발생
	const FVector Center = StaticMeshComp->Bounds.Origin;
	const FVector ToImpact = FVector(ImpactAnchor) - Center;

	FVector ImpulseOrigin = Center;
	if (!ToImpact.IsNearlyZero())
	{
		// 타격 지점 쪽으로 원점을 당겨, 그 반대편으로 잔해가 밀려나게 함
		ImpulseOrigin = Center + ToImpact.GetSafeNormal() * ImpulseOriginOffset;
	}
	
	if (bShowImpulseDebug)
	{
		DrawDebugSphere(GetWorld(), ImpulseOrigin, BreakImpulseRadius, 16, FColor::Red, false, 5.f, 0, 1.5f);
		// 노랑: 임펄스 원점
       	DrawDebugSphere(GetWorld(), ImpulseOrigin, 12.f, 8, FColor::Yellow, false, 5.f, 0, 2.f);
        // 초록: 타격 앵커(공격이 들어온 지점)
        DrawDebugSphere(GetWorld(), FVector(ImpactAnchor), 12.f, 8, FColor::Green, false, 5.f, 0, 2.f);
        // 초록 화살표: 앵커 → 원점 (원점이 공격 쪽으로 당겨진 방향 확인)
        DrawDebugDirectionalArrow(GetWorld(), FVector(ImpactAnchor), ImpulseOrigin, 30.f, FColor::Green, false, 5.f, 0, 2.f);
	}
	
	GeometryCollectionComp->AddRadialImpulse(
		ImpulseOrigin, BreakImpulseRadius, BreakImpulseStrength,
		ERadialImpulseFalloff::RIF_Linear,true);
	
	GeometryCollectionComp->AddImpulse(
		-ToImpact.GetSafeNormal() * LinearPushStrength, NAME_None,true);
	
	// TODO: 파괴 VFX / SFX 트리거
}
