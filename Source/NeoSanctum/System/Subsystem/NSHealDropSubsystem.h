// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NeoSanctum/Data/Progression/Drop/NSHealDropTypes.h"
#include "NSHealDropSubsystem.generated.h"

class APlayerController;
class ANSHealReplicationProxy;
class ANSPlayerState;

/**
 * 서버 전용 회복 드랍 레지스트리
 * 드랍 데이터만 보관하고, 비주얼은 각 플레이어 프록시 경유해서 로컬 생성
 */
UCLASS()
class NEOSANCTUM_API UNSHealDropSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	void RegisterProxy(ANSHealReplicationProxy* Proxy);
	void UnregisterProxy(ANSHealReplicationProxy* Proxy);

	void SetProxyViewPlayerState(APlayerController* ViewerController, ANSPlayerState* ViewPlayerState);

	int32 RegisterDrop(
		FGameplayTag PotionTag,
		const FVector& Location,
		float Duration,
		const FNSDropLaunchData& LaunchData = FNSDropLaunchData()
	);

	bool TryCollect(int32 DropId, ANSPlayerState* Collector);

private:
	int32 AllocateDropId();
	void RemoveExpiredDrops();
	ANSHealReplicationProxy* FindProxy(const ANSPlayerState* PlayerState) const;
	FNSHealSpawnEvent MakeSpawnEvent(int32 DropId, const FNSHealDropEntry& Entry, float NowSeconds) const;

	ANSPlayerState* ResolveProxyViewPlayerState(ANSHealReplicationProxy* Proxy) const;
	void SyncProxyHealVisuals(ANSHealReplicationProxy* Proxy);
	void NotifyHealCollected(int32 DropId, ANSPlayerState* Collector);

	// PotionTag로 DT_HealPotion을 조회해 회복%를 얻고, MaxHealth와 곱해 실제 회복 GE를 적용
	bool ApplyHealEffect(ANSPlayerState* Collector, FGameplayTag PotionTag) const;
	
	bool HasServerAuthority() const;
	float GetWorldSeconds() const;
                                                                                                                     
	int32 NextDropId = 1;
	
	// 실제 드롭 데이터                                                                                                                     
	TMap<int32, FNSHealDropEntry> ActiveDrops;
	
	// 플레이어별 owner-only 프록시                                                                                                         
	TArray<TWeakObjectPtr<ANSHealReplicationProxy>> Proxies;

	// 관전자 프록시가 누구의 물약 획득 상태를 보여주는지 기억.
	TMap<TWeakObjectPtr<ANSHealReplicationProxy>, TWeakObjectPtr<ANSPlayerState>> ProxyViewPlayerState;
	
	// 임시 오버랩 위치 검증용                                                                                                              
	static constexpr float CollectDistanceSq = 300.f * 300.f;
};
