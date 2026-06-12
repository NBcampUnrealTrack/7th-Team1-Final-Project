// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSProjectileTypes.h"
#include "Components/ActorComponent.h"
#include "NSProjectileVisualManagerComponent.generated.h"


class ANSProjectileVisual;

/**
 * 클라이언트에서 관리 중인 시각 투사체 상태 정보
 */
struct FNSClientProjectileVisualState
{
	FNSProjectileSpawnEvent SpawnEvent;
	TWeakObjectPtr<ANSProjectileVisual> VisualActor;
};

/**
 * Proxy가 받은 네트워크 이벤트를 실제 시각 탄환으로 표현
 */
UCLASS(ClassGroup = (Projectile), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSProjectileVisualManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSProjectileVisualManagerComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** 생성 RPC를 받아 로컬 시각 투사체 생성 */
	void HandleSpawnEvent(const FNSProjectileSpawnEvent& SpawnEvent);

	/** 종료 RPC를 받아 해당 시각 투사체 제거 */
	void HandleEndEvent(const FNSProjectileEndEvent& EndEvent);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 시각 투사체
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Visual")
	TSubclassOf<ANSProjectileVisual> VisualClass;

private:
	// GameState의 동기화된 서버 시간 반환
	float GetEstimatedServerTime() const;

	// 현재 서버 시간에 해당하는 투사체 위치 계산
	FVector CalculateLocation(
		const FNSProjectileSpawnEvent& SpawnEvent,
		float ServerTime) const;

	// 특정 ID의 시각 투사체를 파괴하고 컨테이너에서 제거
	void RemoveVisual(int32 ProjectileId);

	TMap<int32, FNSClientProjectileVisualState> ActiveVisuals;
};
