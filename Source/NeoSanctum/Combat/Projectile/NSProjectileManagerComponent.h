// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSProjectileTypes.h"
#include "NSProjectileManagerComponent.generated.h"

class ANSProjectileReplicationProxy;

/**
 * 서버의 모든 Enemy 투사체를 구조체 배열로 관리하는 Component
 */
UCLASS(ClassGroup=(Combat))
class NEOSANCTUM_API UNSProjectileManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSProjectileManagerComponent();

	/**
	 * 서버 투사체를 생성하고 발급된 ProjectileId를 반환한다.
	 * 
	 * @param Request 발사 위치, 방향, 속도, 수명, 충돌 정보
	 * @return 서버 배열 등록 성공 여부
	 */
	int32 FireProjectile(const FNSProjectileFireRequest& Request);

	void RegisterReplicationProxy(ANSProjectileReplicationProxy* Proxy);

	void UnregisterReplicationProxy(ANSProjectileReplicationProxy* Proxy);

protected:
	/**
	 * Component가 월드에 생성된 뒤 호출된다.
	 * 
	 * 서버가 아니면 Tick을 비활성화한다.
	 */
	virtual void BeginPlay() override;

public:
	/**
	 * 서버에서 활성 투사체를 이동시키고 충돌을 검사한다.
	 * 수명이 끝난 투사체를 제거한다.
	 */
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	/**
	 * 투사체 충돌 대상을 검사하고 GAS Damage Effect를 적용한다.
	 * 
	 * @param Projectile 충돌한 투사체의 서버 데이터
	 * @param HitResult Sweep 충돌 결과
	 * @return 대상에게 GameplayEffect를 적용했는지 여부
	 */
	bool TryApplyProjectileDamage(
		const FNSServerProjectileData& Projectile,
		const FHitResult& HitResult) const;

	int32 AllocateProjectileId();

	FNSProjectileSpawnEvent MakeSpawnEvent(const FNSServerProjectileData& Projectile) const;

	void BroadcastSpawnEvent(const FNSProjectileSpawnEvent& SpawnEvent);

	void BroadcastEndEvent(int32 ProjectileId);

	int32 NextProjectileId = 1;

	TArray<TWeakObjectPtr<ANSProjectileReplicationProxy>> ReplicationProxies;

private:
	// 서버가 동시에 보관할 수 있는 투사체의 최대 개수
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Server", meta = (ClampMin = "1"))
	int32 MaxActiveProjectiles = 1000;

	// 서버 이동 결과와 충돌 위치를 Debug로 표시할지 결정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Debug")
	bool bDrawDebugTrajectory = false;

	// 서버에서 현재 사용하고 있는 모든 투사체 데이터
	// 각 투사체 Actor를 Spawn하지 않고 이 배열에만 갱신한다.
	TArray<FNSServerProjectileData> ActiveProjectiles;
};
