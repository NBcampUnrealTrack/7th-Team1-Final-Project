// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NeoSanctum/Data/Progression/Currency/NSCurrencyTypes.h"
#include "NeoSanctum/Data/Progression/Drop/NSDropLaunchData.h"
#include "NSCurrencyDropSubsystem.generated.h"

class ANSCurrencyReplicationProxy;
class ANSPlayerState;

/**
 * 서버 전용 재화 드랍 레지스트리
 * 드랍 데이터만 보관하고, 비주얼은 각 플레이어 프록시 경유해서 로컬 생성
 */
UCLASS()
class NEOSANCTUM_API UNSCurrencyDropSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	void RegisterProxy(ANSCurrencyReplicationProxy* Proxy);
	void UnregisterProxy(ANSCurrencyReplicationProxy* Proxy);
	
	// 드랍 등록 -> DropId 발급 후 전 플레이어에 스폰 이벤트 전송
	int32 RegisterDrop(
		FGameplayTag CurrencyType,
		ENSCurrencyGrade Grade, 
		int64 Amount, 
		const FVector& Location,
		float Duration,
		const FNSDropLaunchData& LaunchData = FNSDropLaunchData()
	);
	
	// 픽업 시도
	bool TryCollect(int32 DropId, ANSPlayerState* Collector);
	
private:
	int32 AllocateDropId();
	void RemoveExpiredDrops();
	ANSCurrencyReplicationProxy* FindProxy(const ANSPlayerState* PlayerState) const;
	FNSCurrencySpawnEvent MakeSpawnEvent(int32 DropId, const FNSCurrencyDropEntry& Entry,
		float NowSeconds) const;
	
	bool HasServerAuthority() const;
	float GetWorldSeconds() const;
	
	int32 NextDropId = 1;
	
	// 실제 드롭 데이터
	TMap<int32, FNSCurrencyDropEntry> ActiveDrops;
	
	// 플레이어별 owner_only 프록시
	TArray<TWeakObjectPtr<ANSCurrencyReplicationProxy>> Proxies;
	
	// 임시 오버랩 위치 검증용
	static constexpr float CollectDistanceSq = 300.f * 300.f;
	
	// Companion
	static constexpr float CompanionCollectDistanceSq = 200.f * 200.f;
	
#pragma region CompanionDropSystem
	
public:
	bool FindNearestTrackableDrop(
		const ANSPlayerState* CompanionOwnerPS,
		const FVector& FromLocation,
		float MaxRadius,
		int32& OutDropId,
		FVector& OutLocation);
	
	bool TryCollectByCompanion(
		int32 DropId,
		ANSPlayerState* CompanionOwnerPS,
		const FVector& CompanionLocation);	
	
private:
	bool IsDropTrackableFor(
		const FNSCurrencyDropEntry& Entry,
		const ANSPlayerState* PlayerState,
		float Now) const;
	
#pragma endregion 
};
