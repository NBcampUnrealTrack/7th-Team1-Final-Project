// Copyright 2026 One Team. All rights reserved.


#include "NSProjectileManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GenericTeamAgentInterface.h"
#include "NSProjectileTypes.h"


UNSProjectileManagerComponent::UNSProjectileManagerComponent()
{
	// Tick 기능 사용
	PrimaryComponentTick.bCanEverTick = true;

	// 게임 시작할 때부터 투사체 위치 지속적으로 갱신
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// 이 ActorComponent 자체를 네트워크 복제하지 않음
	SetIsReplicatedByDefault(false);
}

void UNSProjectileManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ProjectileManagerComponent를 소유할 Actor: ANSRunGameState를 지칭함.
	AActor* OwnerActor = GetOwner();

	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		// ANSRunGameState에 붙어 클라이언트에도 이 ActorComponent가 존재하므로 
		// 서버에서만 수행하도록 함.
		SetComponentTickEnabled(false);
		return;
	}

	// 최대 예상 개수만큼 메모리를 미리 확보
	// 발사 중 TArray 재할당이 반복되는 것을 줄임
	ActiveProjectiles.Reserve(MaxActiveProjectiles);
}

bool UNSProjectileManagerComponent::FireProjectile(const FNSProjectileFireRequest& Request)
{
	AActor* OwnerActor = GetOwner();

	// 서버에서만 수행하도록 함.
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	// 활성 투사체가 설정된 최대 개수에 도달하면 실행 X
	if (ActiveProjectiles.Num() >= MaxActiveProjectiles)
	{
		return false;
	}

	// 이동 방향 정규화
	const FVector NormalizedDirection = Request.Direction.GetSafeNormal();

	// 이동할 수 없는 잘못된 요청 거부
	if (NormalizedDirection.IsNearlyZero() ||
		Request.Speed <= 0.0f ||
		Request.MaxLifeTime <= 0.0f ||
		Request.Radius <= 0.0f ||
		!Request.DamageEffectClass)
	{
		return false;
	}

	// 배열에 기본값으로 초기화된 원소를 하나 추가하고 그 원소의 참조를 바로 받음
	FNSServerProjectileData& Projectile = ActiveProjectiles.AddDefaulted_GetRef();

	// 발사 요청을 서버 런타임 데이터로 복사함
	Projectile.CurrentLocation = Request.StartLocation;
	Projectile.Direction = NormalizedDirection;
	Projectile.Speed = Request.Speed;
	Projectile.LifeTime = 0.0f;
	Projectile.MaxLifeTime = Request.MaxLifeTime;
	Projectile.Radius = Request.Radius;
	Projectile.TraceChannel = Request.TraceChannel;
	Projectile.SourceActor = Request.SourceActor;
	Projectile.DamageEffectClass = Request.DamageEffectClass;

	return true;
}

void UNSProjectileManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* OwnerActor = GetOwner();

	// 서버에서만 수행하도록 함.
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	// 뒤에서 앞으로 순회한다.
	// 순회 중 RemoveAtSwap으로 제거해도 아직 처리하지 않은
	// 배열 인덱스가 영향을 받지 않게 하기 위함이다.
	for (int32 Index = ActiveProjectiles.Num() - 1; Index >= 0; --Index)
	{
		FNSServerProjectileData& Projectile = ActiveProjectiles[Index];

		// 현재 투사체에 남아 있는 수명
		const float RemainingLifeTime = Projectile.MaxLifeTime - Projectile.LifeTime;

		// 최대 수명에 도달한 투사체를 배열에서 제거
		if (RemainingLifeTime <= 0.0f)
		{
			ActiveProjectiles.RemoveAtSwap(
				Index,
				1,
				EAllowShrinking::No);
			continue;
		}

		// 마지막 프레임에서 최대 수명을 초과한 시간만큼
		// 더 이동하지 않도록 실제 계산 시간을 제한한다.
		const float SimulationTime = FMath::Min(DeltaTime, RemainingLifeTime);

		// Debug Line의 시작점으로 사용할 이동 전 위치
		const FVector PreviousLocation = Projectile.CurrentLocation;

		// 충돌이 없다고 가정했을 때의 다음 위치
		const FVector NextLocation = PreviousLocation + Projectile.Direction * Projectile.Speed * SimulationTime;

		Projectile.LifeTime += SimulationTime;

		// Sweep 충돌 검사 옵션
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NSProjectile), false);

		// 투사체를 발사한 Enemy 자신은 충돌 검사에서 제외
		if (AActor* SourceActor = Projectile.SourceActor.Get())
		{
			QueryParams.AddIgnoredActor(SourceActor);
		}

		// 충돌 결과를 받을 구조체
		FHitResult HitResult;

		// Sweep 충돌 검사
		const bool bHit = World->SweepSingleByChannel(
			HitResult,
			PreviousLocation,
			NextLocation,
			FQuat::Identity,
			Projectile.TraceChannel,
			FCollisionShape::MakeSphere(Projectile.Radius),
			QueryParams
		);

		if (bHit)
		{
			Projectile.CurrentLocation = HitResult.Location;

			const bool bAppliedDamage = TryApplyProjectileDamage(Projectile, HitResult);

			if (bDrawDebugTrajectory)
			{
				const FColor HitColor = bAppliedDamage ? FColor::Purple : FColor::Red;

				// 실제로 충돌한 이동 구간
				DrawDebugLine(
					World,
					PreviousLocation,
					HitResult.Location,
					HitColor,
					false,
					1.0f,
					0,
					2.0f);

				// 충돌 순간 투사체 Sphere의 위치와 크기
				DrawDebugSphere(
					World,
					HitResult.Location,
					Projectile.Radius,
					12,
					HitColor,
					false,
					1.0f,
					0,
					1.0f);

				// 실제 표면 접촉 지점
				DrawDebugPoint(
					World,
					HitResult.ImpactPoint,
					12.0f,
					FColor::Orange,
					false,
					1.0f);
			}

			// 충돌한 투사체 제거
			ActiveProjectiles.RemoveAtSwap(
				Index,
				1,
				EAllowShrinking::No);
			continue;
		}

		// 충돌하지 않은 경우 처리
		Projectile.CurrentLocation = NextLocation;

		if (bDrawDebugTrajectory)
		{
			DrawDebugLine(
				World,
				PreviousLocation,
				NextLocation,
				FColor::Green,
				false,
				0.0f,
				0,
				2.0f);

			DrawDebugPoint(
				World,
				NextLocation,
				8.0f,
				FColor::Cyan,
				false,
				0.0f);
		}

		// 충돌하지 않고 이동 중 최대 수명 도달하여 투사체 제거
		if (Projectile.LifeTime >= Projectile.MaxLifeTime)
		{
			ActiveProjectiles.RemoveAtSwap(
				Index,
				1,
				EAllowShrinking::No);
		}
	}
}

bool UNSProjectileManagerComponent::TryApplyProjectileDamage(
	const FNSServerProjectileData& Projectile,
	const FHitResult& HitResult) const
{
	AActor* SourceActor = Projectile.SourceActor.Get();
	AActor* TargetActor = HitResult.GetActor();

	if (!IsValid(SourceActor) ||
		!IsValid(TargetActor) ||
		SourceActor == TargetActor ||
		!Projectile.DamageEffectClass)
	{
		return false;
	}

	const IGenericTeamAgentInterface* SourceTeam = Cast<IGenericTeamAgentInterface>(SourceActor);
	const IGenericTeamAgentInterface* TargetTeam = Cast<IGenericTeamAgentInterface>(TargetActor);

	// 같은 팀 피해 적용 X
	if (SourceTeam &&
		TargetTeam &&
		SourceTeam->GetGenericTeamId() == TargetTeam->GetGenericTeamId())
	{
		return false;
	}

	IAbilitySystemInterface* SourceAbilityInterface = Cast<IAbilitySystemInterface>(SourceActor);
	IAbilitySystemInterface* TargetAbilityInterface = Cast<IAbilitySystemInterface>(TargetActor);

	if (!SourceAbilityInterface || !TargetAbilityInterface)
	{
		return false;
	}

	UAbilitySystemComponent* SourceASC = SourceAbilityInterface->GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = TargetAbilityInterface->GetAbilitySystemComponent();

	if (!IsValid(SourceASC) ||
		!IsValid(TargetASC))
	{
		return false;
	}

	// 발사자, 충돌 정보 저장하는 EffectContext 저장
	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(SourceActor);
	EffectContext.AddHitResult(HitResult, true);

	// 발사자의 ASC를 Source로 해 Damage EffectSpec 생성
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
		Projectile.DamageEffectClass,
		1.0f,
		EffectContext);

	if (!SpecHandle.IsValid() ||
		!SpecHandle.Data.IsValid())
	{
		return false;
	}

	// 서버에서 Source ASC가 Target ASC에 Effect 적용
	SourceASC->ApplyGameplayEffectSpecToTarget(
		*SpecHandle.Data.Get(),
		TargetASC);

	return true;
}
