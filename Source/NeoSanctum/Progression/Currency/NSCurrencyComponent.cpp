// Copyright 2026 One Team. All rights reserved.


#include "NSCurrencyComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "NeoSanctum/Tag/NSGameplayTags_Currency.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/CommonUpgrade/NSCommonUpgradeUtilityHelper.h"


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

	const int64 FinalAmount = ApplyCurrencyGainBoost(NSCommonUpgradeUtility::NodeId_TempCurrencyGainRate, Amount);

	AddToWallet(NSGameplayTags::Currency_Temp, FinalAmount);
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

	const int64 FinalAmount = ApplyCurrencyGainBoost(NSCommonUpgradeUtility::NodeId_CommonCurrencyGainRate, Amount);

	PendingPermanent.FindOrAdd(Type) += FinalAmount;
	AddToWallet(Type, FinalAmount);
}

void UNSCurrencyComponent::AddPermanentDirect(FGameplayTag Type, int64 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	const int64 FinalAmount = ApplyCurrencyGainBoost(NSCommonUpgradeUtility::NodeId_CommonCurrencyGainRate, Amount);

	PendingPermanent.FindOrAdd(Type) += FinalAmount;
	AddToWallet(Type, FinalAmount);
}

void UNSCurrencyComponent::CommitRunPermanent(float Multiplier)
{
	ANSPlayerState* PS = GetOwner<ANSPlayerState>();
	UNSPlayerProgressComponent* Progress = PS ? PS->GetProgressComponent() : nullptr;
	if (!Progress)
	{
		return;
	}

	// PendingPermanent는 AddRunPermanent()/AddPermanentDirect()에서 이미 공용 업그레이드
	// 재화 획득량 보너스가 적용된 값이므로 여기서는 클리어/전멸 배율만 곱해야 하며,
	// 유틸 보너스를 다시 적용하면 중복 적용.
	UE_LOG(LogTemp, Log, TEXT("[Currency] 런 종료 영구재화 커밋 시작 (배율 %.2f)"), Multiplier);
	for (const TPair<FGameplayTag, int64>& Pair : PendingPermanent)
	{
		const int64 Committed = static_cast<int64>(Pair.Value * Multiplier);
		UE_LOG(LogTemp, Log, TEXT("[Currency] 커밋 대상 Type=%s Pending=%lld Committed=%lld"),
		*Pair.Key.ToString(),
		Pair.Value,
		Committed);

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
	FNSProgressPayload Payload;
	Progress->BuildPayload(Payload);

	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->SetCachedProgressPayload(Payload);
	}
	PendingPermanent.Empty();
}

void UNSCurrencyComponent::ClearWallet()
{
	Wallet.Entries.Reset();
	Wallet.MarkArrayDirty();
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

int64 UNSCurrencyComponent::ApplyCurrencyGainBoost(FName UtilityNodeId, int64 BaseAmount) const
{
	const ANSPlayerState* PS = GetOwner<ANSPlayerState>();
	const UNSPlayerProgressComponent* ProgressComp = PS ? PS->GetProgressComponent() : nullptr;
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);

	const double Percent = NSCommonUpgradeUtility::GetPercent(DataSubsystem, ProgressComp, UtilityNodeId);
	if (Percent == 0.0)
	{
		return BaseAmount;
	}

	// 일단 내림으로 처리. 보상이 적으면 보너스가 안 붙은 것처럼 보일 수도 있는데,
	// 그 정도는 감안하기로 함(소수점까지 딱 맞게 챙기는 건 지금은 안 함).
	return FMath::FloorToInt64(static_cast<double>(BaseAmount) * (1.0 + Percent * 0.01));
}

void UNSCurrencyComponent::NotifyWalletEntryChanged(const FNSCurrencyEntry& Entry)
{
	if (Entry.CurrencyType == NSGameplayTags::Currency_Temp)
	{
		OnTempChanged.Broadcast(Entry.Amount);
		return;
	}
	OnPermanentChanged.Broadcast(Entry.CurrencyType, Entry.Amount);
}

void UNSCurrencyComponent::CopyRunStateFrom(const UNSCurrencyComponent* Source)
{
	if (!Source)
	{
		return;
	}
	
	// 서버 전용 런 누적 버킷
	PendingPermanent = Source->PendingPermanent;
	
	// 지갑(임시재화 표시) — FastArray 정석대로 엔트리 재구성
	for (const FNSCurrencyEntry& Entry : Source->Wallet.Entries)
	{
		AddToWallet(Entry.CurrencyType, Entry.Amount);
	}
}


