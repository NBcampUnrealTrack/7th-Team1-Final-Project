// Copyright 2026 One Team. All rights reserved.


#include "NSCurrencyDropSubsystem.h"

#include "ToolMenusEditor.h"
#include "NeoSanctum/Progression/Currency/NSCurrencyReplicationProxy.h"
#include "NeoSanctum/Progression/Currency/NSCurrencyComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Tag/NSGameplayTags_Currency.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Splines/SplineMath.h"
#include "Tests/ToolMenusTestUtilities.h"

// 프록시 등록 함수
void UNSCurrencyDropSubsystem::RegisterProxy(ANSCurrencyReplicationProxy* Proxy)
{
	if (!HasServerAuthority())
	{
		return;
	}
	if (!IsValid(Proxy))
	{
		return;
	}
	if (Proxies.Contains(Proxy))
	{
		return;
	}
	// 프록시 등록
	Proxies.Add(Proxy);
	RemoveExpiredDrops();
	const float Now = GetWorldSeconds();
	for (const TPair<int32, FNSCurrencyDropEntry>& Pair : ActiveDrops)
	{
		// 실제로 비주얼 액터 만듬 (클라)
		Proxy->SendSpawnEvent(MakeSpawnEvent(Pair.Key, Pair.Value, Now));
	}
}

void UNSCurrencyDropSubsystem::UnregisterProxy(ANSCurrencyReplicationProxy* Proxy)
{
	Proxies.Remove(Proxy);
}


int32 UNSCurrencyDropSubsystem::RegisterDrop(
	FGameplayTag CurrencyType,
	ENSCurrencyGrade Grade,
	int64 Amount,
	const FVector& Location,
	float Duration,
	const FNSDropLaunchData& LaunchData
	)
{
	if (!HasServerAuthority() || Amount <= 0 || Duration <= 0.f)
	{
		return INDEX_NONE;
	}
	RemoveExpiredDrops();
	
	const float Now = GetWorldSeconds();
	
	FNSCurrencyDropEntry Entry;
	Entry.CurrencyType = CurrencyType;
	Entry.Grade = Grade;
	Entry.Amount = Amount;
	Entry.Location = Location;
	Entry.LaunchData = LaunchData;
	Entry.ExpireTime = Now + Duration;
	
	const int32 DropId = AllocateDropId();
	ActiveDrops.Add(DropId, Entry);
	
	Proxies.RemoveAll([](const TWeakObjectPtr<ANSCurrencyReplicationProxy>& P)
	{
		return !P.IsValid();
	});
	
	const FNSCurrencySpawnEvent Event = MakeSpawnEvent(DropId, Entry, Now);
	for (const TWeakObjectPtr<ANSCurrencyReplicationProxy>& Proxy : Proxies)
	{
		Proxy->SendSpawnEvent(Event);
	}
	return DropId;
}

bool UNSCurrencyDropSubsystem::TryCollect(int32 DropId, ANSPlayerState* Collector)
{
	if (!HasServerAuthority() || !IsValid(Collector))
	{
		return false;
	}
	
	FNSCurrencyDropEntry* Entry = ActiveDrops.Find(DropId);
	if (!Entry)
	{
		return false;
	}
	
	const float Now = GetWorldSeconds();
	if (Now >= Entry->ExpireTime)
	{
		ActiveDrops.Remove(DropId);
		return false;
	}
	
	if (Entry->LaunchData.IsValid())
	{
		const float LaunchEndTime = Entry->LaunchData.StartServerTime + Entry->LaunchData.FlightDuration;
		
		if (Now < LaunchEndTime)
		{
			return false;
		}
	}
	
	if (Entry->CollectedPlayer.Contains(Collector))
	{
		return false;
	}
	const APawn* Pawn = Collector->GetPawn();
	if (!Pawn || FVector::DistSquared(Pawn->GetActorLocation(), Entry->Location) > CollectDistanceSq)
	{
		return false;
	}
	
	UNSCurrencyComponent* Currency = Collector->GetCurrencyComponent();
	if (!Currency)
	{
		return false;
	}
	
	if (Entry->CurrencyType == NSGameplayTags::Currency_Temp)
	{
		// 임시 재화면 지갑에
		Currency->AddTemp(static_cast<int32>(Entry->Grade), Entry->Amount);
	}
	else
	{
		// 영구재화면 버킷에
		Currency->AddRunPermanent(Entry->CurrencyType, Entry->Amount);
	}
	
	// 주운 플레이어에 등록
	Entry->CollectedPlayer.Add(Collector);
	// 플레이어 화면에서 삭제
	if (ANSCurrencyReplicationProxy* Proxy = FindProxy(Collector))
	{
		Proxy->SendRemoveEvent(DropId);
	}
	
	return true;
}

int32 UNSCurrencyDropSubsystem::AllocateDropId()
{
	const int32 Allocated = NextDropId++;
	if (NextDropId <= 0)
	{
		NextDropId = 1;
	}
	return Allocated;
}

void UNSCurrencyDropSubsystem::RemoveExpiredDrops()
{
	const float Now = GetWorldSeconds();
	
	TArray<int32> Expired;
	for (const TPair<int32, FNSCurrencyDropEntry>& Pair : ActiveDrops)
	{
		if (Now >= Pair.Value.ExpireTime)
		{
			Expired.Add(Pair.Key);
		}
	}
	
	for (int32 DropId : Expired)
	{
		ActiveDrops.Remove(DropId);
	}
}

ANSCurrencyReplicationProxy* UNSCurrencyDropSubsystem::FindProxy(const ANSPlayerState* PlayerState) const
{
	for (const TWeakObjectPtr<ANSCurrencyReplicationProxy>& Proxy : Proxies)
	{
		ANSCurrencyReplicationProxy* Pro = Proxy.Get();
		if (!Pro)
		{
			continue;
		}
		const APlayerController* PC = Cast<APlayerController>(Pro->GetOwner());
		if (PC && PC->PlayerState == PlayerState)
		{
			return Pro;
		}
	}
	return nullptr;
}

FNSCurrencySpawnEvent UNSCurrencyDropSubsystem::MakeSpawnEvent(int32 DropId, const FNSCurrencyDropEntry& Entry,
	float NowSeconds) const
{
	FNSCurrencySpawnEvent Event;
	Event.DropId = DropId;
	Event.CurrencyType = Entry.CurrencyType;
	Event.Grade = Entry.Grade;
	Event.Amount = Entry.Amount;
	Event.Location = Entry.Location;
	Event.LaunchData = Entry.LaunchData;
	Event.Duration = FMath::Max(0.f, Entry.ExpireTime - NowSeconds);
	return Event;
}

bool UNSCurrencyDropSubsystem::HasServerAuthority() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Client;
}

float UNSCurrencyDropSubsystem::GetWorldSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.f;
}

bool UNSCurrencyDropSubsystem::FindNearestTrackableDrop(const ANSPlayerState* CompanionOwnerPS, const FVector& FromLocation,
	float MaxRadius, int32& OutDropId, FVector& OutLocation)
{
	// 서버 판정 및 오너 유효
	if (!HasServerAuthority() || !CompanionOwnerPS) return false;
	
	// 드랍 시간 확보
	const float Now = GetWorldSeconds();
	
	// 제곱 거리 캐싱
	float BestDistSq = MaxRadius * MaxRadius;
	int32 BestDropId = INDEX_NONE;
	FVector BestLocation = FVector::ZeroVector;
	
	// 드롭물 순회
	for (const TPair<int32, FNSCurrencyDropEntry>& Pair : ActiveDrops)
	{
		if (!IsDropTrackableFor(Pair.Value, CompanionOwnerPS, Now)) continue;
		
		// 드롭물과의 거리 계산
		const float DistSq = FVector::DistSquared(FromLocation, Pair.Value.Location);
		if (DistSq <= BestDistSq)
		{
			// 반경 안이라면 갱신
			BestDistSq = DistSq;
			BestDropId = Pair.Key;
			BestLocation = Pair.Value.Location;
		}
	}
	
	if (BestDropId == INDEX_NONE) return false;
	
	OutDropId = BestDropId;
	OutLocation = BestLocation;
	return true;
}

bool UNSCurrencyDropSubsystem::TryCollectByCompanion(int32 DropId, ANSPlayerState* CompanionOwnerPS,
	const FVector& CompanionLocation)
{
	// 권한 체크
	if (!HasServerAuthority() || !CompanionOwnerPS) return false;
	
	// 드랍 재화 정보 만들기
	FNSCurrencyDropEntry* Entry = ActiveDrops.Find(DropId);
	if (!Entry) return false;
	
	// 현재 시간 받아와서 만료 시간과 비교
	const float Now = GetWorldSeconds();
	if (Now >= Entry->ExpireTime)
	{
		ActiveDrops.Remove(DropId);
		return false;
	}
	
	if (!IsDropTrackableFor(*Entry, CompanionOwnerPS, Now)) return false;
	
	const FVector Delta = CompanionLocation - Entry->Location;
	if(FVector(Delta.X,Delta.Y,0.f).SizeSquared() > CompanionCollectDistanceSq) return false;
	
	UNSCurrencyComponent* CurrencyComp = CompanionOwnerPS->GetCurrencyComponent();
	if (!CurrencyComp) return false;
	
	if (Entry->CurrencyType == NSGameplayTags::Currency_Temp)
	{
		CurrencyComp->AddTemp(static_cast<int32>(Entry->Grade), Entry->Amount);
	}
	else
	{
		CurrencyComp->AddRunPermanent(Entry->CurrencyType, Entry->Amount);
	}
	
	Entry->CollectedPlayer.Add(CompanionOwnerPS);
	
	if (ANSCurrencyReplicationProxy* Proxy = FindProxy(CompanionOwnerPS))
	{
		Proxy->SendRemoveEvent(DropId);
	}
	
	return true;
}

bool UNSCurrencyDropSubsystem::IsDropTrackableFor(const FNSCurrencyDropEntry& Entry, const ANSPlayerState* PlayerState,
	float Now) const
{
	// 만료 시간 이후면 추적 대상 아님
	if (Now >= Entry.ExpireTime) return false;
	// 포물선 정보가 있을 경우에 비행 종료 여부 검사
	if (Entry.LaunchData.IsValid())
	{
		// 아직 공중에 떠 있는지 확인
		float FinishedFlyingTime = Entry.LaunchData.StartServerTime + Entry.LaunchData.FlightDuration;
		if (Now < FinishedFlyingTime) return false;
	}
	
	// 조건 4. 이미 수집을 한 Player인지 판단
	for (const TWeakObjectPtr<ANSPlayerState>& EntryPS : Entry.CollectedPlayer)
	{
		if (PlayerState == EntryPS) return false;
	}
	
	return true;
}
