// Copyright 2026 One Team. All rights reserved.


#include "NSCurrencyComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "NeoSanctum/Tag/NSGameplayTags_Currency.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"


// ================================================================
// FNSCurrencyWallet 콜백
// ================================================================

void FNSCurrencyWallet::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	if (!OwnerComponent)
	{
		return;
	}
	for (int32 Index : AddedIndices)
	{
		OwnerComponent->NotifyWalletEntryChanged(Entries[Index]);
	}
}

void FNSCurrencyWallet::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	if (!OwnerComponent)
	{
		return;
	}
	for (int32 Index : ChangedIndices)
	{
		OwnerComponent->NotifyWalletEntryChanged(Entries[Index]);
	}
}

UNSCurrencyComponent::UNSCurrencyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Wallet.OwnerComponent = this;
}

void UNSCurrencyComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UNSCurrencyComponent, Wallet, COND_OwnerOnly);
}

void UNSCurrencyComponent::AddTemp(int32 Grade, int64 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	AddToWallet(NSGameplayTags::Currency_Temp, Amount);
	// 테스트용 임시 로그 (드롭 테이블 연동 후 삭제)
	UE_LOG(LogTemp, Log, TEXT("[Currency] 임시재화 +%lld 획득 → 현재 보유 %lld"), Amount, GetTemp());
}

bool UNSCurrencyComponent::TrySpendTemp(int64 Amount)
{
	if (Amount <= 0)
	{
		return false;
	}
	if (GetTemp() < Amount)
	{
		return false;
	}

	AddToWallet(NSGameplayTags::Currency_Temp, -Amount);
	return true;
}

void UNSCurrencyComponent::AddRunPermanent(FGameplayTag Type, int64 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	const int64 Pending = (PendingPermanent.FindOrAdd(Type) += Amount);
	// 테스트용 임시 로그 (드롭 테이블 연동 후 삭제)
	UE_LOG(LogTemp, Log, TEXT("[Currency] 영구재화(%s) +%lld 획득 → 대기 누적 %lld (커밋 전)"),
		*Type.ToString(), Amount, Pending);
}

void UNSCurrencyComponent::AddPermanentDirect(FGameplayTag Type, int64 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	PendingPermanent.FindOrAdd(Type) += Amount;
}

void UNSCurrencyComponent::CommitRunPermanent(float Multiplier)
{
	ANSPlayerState* PS = GetOwner<ANSPlayerState>();
	UNSPlayerProgressComponent* Progress = PS ? PS->GetProgressComponent() : nullptr;
	if (!Progress)
	{
		return;
	}
	// 테스트용 임시 로그 (드롭 테이블 연동 후 삭제)
	UE_LOG(LogTemp, Log, TEXT("[Currency] 런 종료 영구재화 커밋 시작 (배율 %.2f)"), Multiplier);
	for (const TPair<FGameplayTag, int64>& Pair : PendingPermanent)
	{
		const int64 Committed = static_cast<int64>(Pair.Value * Multiplier);
		if (Committed <= 0)
		{
			continue;
		}
		
		if (Pair.Key == NSGameplayTags::Currency_Common)
		{
			Progress->AddCommonCurrency(Committed);
		}
		else if (Pair.Key == NSGameplayTags::Currency_Skill)
		{
			Progress->AddJobCurrency(Committed);
		}
	}
	PendingPermanent.Empty();
}

int64 UNSCurrencyComponent::GetTemp() const
{
	return GetAmount(NSGameplayTags::Currency_Temp);
}

int UNSCurrencyComponent::GetPermanent(FGameplayTag Type) const
{
	return GetAmount(Type);
}

int64 UNSCurrencyComponent::GetAmount(FGameplayTag Type) const
{
	for (const FNSCurrencyEntry& Entry : Wallet.Entries)
	{
		if (Entry.CurrencyType == Type)
		{
			return Entry.Amount;
		}
	}
	return 0;
}

void UNSCurrencyComponent::AddToWallet(FGameplayTag Type, int64 Amount)
{
	FNSCurrencyEntry* Found = Wallet.Entries.FindByPredicate([Type](const FNSCurrencyEntry& Entry)
	{
		return Entry.CurrencyType == Type;
	});
	
	if (!Found)
	{
		FNSCurrencyEntry NewEntry;
		NewEntry.CurrencyType = Type;
		NewEntry.Amount = Amount;
		const int32 Index = Wallet.Entries.Add(NewEntry);
		Found = &Wallet.Entries[Index];
	} 
	else
	{
		Found->Amount += Amount;
	}
	
	Wallet.MarkItemDirty(*Found);
	NotifyWalletEntryChanged(*Found);
}

void UNSCurrencyComponent::NotifyWalletEntryChanged(const FNSCurrencyEntry& Entry)
{
	if (Entry.CurrencyType == NSGameplayTags::Currency_Temp)
	{
		OnTempChanged.Broadcast(Entry.Amount);
		return;
	}
	OnPermanenetChanged.Broadcast(Entry.CurrencyType, Entry.Amount);
}


