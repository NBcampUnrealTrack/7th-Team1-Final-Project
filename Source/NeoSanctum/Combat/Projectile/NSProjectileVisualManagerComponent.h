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

	
private:
	// 게임 시작 시 미리 생성해 둘 Visual Actor 수
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Visual Pool", meta = (ClampMin = "0"))
	int32 InitialPoolSize = 32;

	// 이 인스턴스에서 시각 처리를 수행할지 여부
	bool bVisualSystemEnabled = false;

	// 풀을 초기 크기만큼 미리 생성
	void PrewarmPool();

	// 새로운 풀 전용 Visual Actor를 생성
	ANSProjectileVisual* CreatePooledVisual();

	// 풀에서 Visual Actor 하나를 대여
	ANSProjectileVisual* AcquireVisual();

	// 사용이 끝난 Visual Actor를 풀에 반납
	void ReleaseVisual(ANSProjectileVisual* VisualActor);

	// 현재 사용되지 않고 풀에서 대기 중인 Visual Actor들
	TArray<TWeakObjectPtr<ANSProjectileVisual>> AvailableVisuals;

	// 현재 ProjectileId와 연결되어 사용 중인 Visual Actor들
	TMap<int32, FNSClientProjectileVisualState> ActiveVisuals;
};
