// Copyright 2026 One Team. All rights reserved.


#include "NSHealDropSubsystem.h"
#include "NeoSanctum/Progression/Heal/NSHealReplicationProxy.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "AbilitySystemComponent.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

void UNSHealDropSubsystem::RegisterProxy(ANSHealReplicationProxy* Proxy)
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
	Proxies.Add(Proxy);
	RemoveExpiredDrops();
	const float Now = GetWorldSeconds();
	for (const TPair<int32, FNSHealDropEntry>& Pair : ActiveDrops)
	{
		Proxy->SendSpawnEvent(MakeSpawnEvent(Pair.Key, Pair.Value, Now));
	}
}

void UNSHealDropSubsystem::UnregisterProxy(ANSHealReplicationProxy* Proxy)
{
	Proxies.Remove(Proxy);
}

int32 UNSHealDropSubsystem::RegisterDrop(FGameplayTag PotionTag, const FVector& Location, float Duration,
	const FNSDropLaunchData& LaunchData)
{
	if (!HasServerAuthority() || !PotionTag.IsValid() || Duration <= 0.f)
	{
		return INDEX_NONE;
	}
	RemoveExpiredDrops();

	const float Now = GetWorldSeconds();
	FNSHealDropEntry Entry;
	Entry.PotionTag = PotionTag;
	Entry.Location = Location;
	Entry.LaunchData = LaunchData;
	Entry.ExpireTime = Now + Duration;
	
	const int32 DropId = AllocateDropId();
	ActiveDrops.Add(DropId, Entry);
	
	Proxies.RemoveAll([](const TWeakObjectPtr<ANSHealReplicationProxy>& Proxy)
	{
		return !Proxy.IsValid();
	});
	
	const FNSHealSpawnEvent Event = MakeSpawnEvent(DropId, Entry,  Now);
	for (const TWeakObjectPtr<ANSHealReplicationProxy>& Proxy : Proxies)
	{
		Proxy->SendSpawnEvent(Event);
	}
	return DropId;
}

bool UNSHealDropSubsystem::TryCollect(int32 DropId, ANSPlayerState* Collector)
{
	if (!HasServerAuthority() || !IsValid(Collector))
	{
		return false;
	}
	
	FNSHealDropEntry* Entry = ActiveDrops.Find(DropId);
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
	
	if (!ApplyHealEffect(Collector, Entry->PotionTag))
	{
		// 풀피라 회복이 적용되지 않은 경우(또는 GE/DT 조회 실패) — 소모되지 않았으므로
		// CollectedPlayer에 기록하지 않고, 클라가 낙관적으로 숨긴 비주얼을 다시 보이게 복원.
		// 이 플레이어는 이후 체력이 줄면 같은 드랍을 다시 시도할 수 있다.
		if (ANSHealReplicationProxy* Proxy = FindProxy(Collector))
		{
			Proxy->SendRestoreEvent(DropId);
		}
		return false;
	}

	// 주운 플레이어에 등록
	Entry->CollectedPlayer.Add(Collector);
	// 플레이어 화면에서 삭제
	if (ANSHealReplicationProxy* Proxy = FindProxy(Collector))
	{
		Proxy->SendRemoveEvent(DropId);
	}
	return true;
}

int32 UNSHealDropSubsystem::AllocateDropId()
{
	const int32 Allocated = NextDropId++;
	if (NextDropId <= 0)
	{
		NextDropId = 1;
	}
	return Allocated;
}

void UNSHealDropSubsystem::RemoveExpiredDrops()
{
	const float Now = GetWorldSeconds();
	
	TArray<int32> Expired;
	for (const TPair<int32, FNSHealDropEntry>& Pair : ActiveDrops)
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

ANSHealReplicationProxy* UNSHealDropSubsystem::FindProxy(const ANSPlayerState* PlayerState) const
{
	for (const TWeakObjectPtr<ANSHealReplicationProxy>& Proxy : Proxies)
	{
		ANSHealReplicationProxy* Pro = Proxy.Get();
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

FNSHealSpawnEvent UNSHealDropSubsystem::MakeSpawnEvent(int32 DropId, const FNSHealDropEntry& Entry,
	float NowSeconds) const
{
	FNSHealSpawnEvent Event;
	Event.DropId = DropId;
	Event.PotionTag = Entry.PotionTag;
	Event.Location = Entry.Location;
	Event.LaunchData = Entry.LaunchData;
	Event.Duration = FMath::Max(0.f, Entry.ExpireTime-NowSeconds);
	return Event;
}

bool UNSHealDropSubsystem::ApplyHealEffect(ANSPlayerState* Collector, FGameplayTag PotionTag) const
{
	UAbilitySystemComponent* ASC = Collector->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	// 풀피 상태면 소모하지 않고 거부 — 최대체력 증강으로 MaxHealth가 달라도
	// 항상 그 사람 기준 Health/MaxHealth 비교이므로 정상 동작.
	const float CurrentHealth = ASC->GetNumericAttribute(UNSBaseAttributeSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(UNSBaseAttributeSet::GetMaxHealthAttribute());
	if (CurrentHealth >= MaxHealth)
	{
		return false;
	}

	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem)
	{
		return false;
	}

	const TSubclassOf<UGameplayEffect> HealEffectClass = DataSubsystem->GetInstantHealEffectClass();
	if (!HealEffectClass)
	{
		return false;
	}

	// PotionTag(정체성)만으로는 회복%를 알 수 없으므로, DT_HealPotion에서 RowName=태그이름으로 조회.
	const UDataTable* HealPotionTable = DataSubsystem->GetHealPotionTable();
	if (!HealPotionTable)
	{
		return false;
	}

	static const FString Context(TEXT("UNSHealDropSubsystem::ApplyHealEffect"));
	const FNSHealPotionRow* Row = HealPotionTable->FindRow<FNSHealPotionRow>(PotionTag.GetTagName(), Context, false);
	if (!Row)
	{
		return false;
	}

	const float HealAmount = MaxHealth * Row->HealPercent / 100.f;

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(HealEffectClass, 1.0f, EffectContext);
	if (!SpecHandle.IsValid())
	{
		return false;
	}
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Heal, HealAmount);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	return true;
}

bool UNSHealDropSubsystem::HasServerAuthority() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Client;
}

float UNSHealDropSubsystem::GetWorldSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.f;
}
