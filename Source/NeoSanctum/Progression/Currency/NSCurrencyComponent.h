// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "NSCurrencyComponent.generated.h"

USTRUCT()
struct FNSCurrencyEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	UPROPERTY()
	FGameplayTag CurrencyType;
	
	UPROPERTY()
	int64 Amount = 0;
};

USTRUCT()
struct FNSCurrencyWallet : public FFastArraySerializer
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FNSCurrencyEntry> Entries;
	
	UPROPERTY()
	TObjectPtr<UNSCurrencyComponent> OwnerComponent = nullptr;
	
	// 새로운게 추가되었을때
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	// 기존게 변경되었을때
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
	
	// 직렬화 진입 점
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FNSCurrencyEntry, FNSCurrencyWallet>(Entries, DeltaParams, *this);
	}
};

DECLARE_MULTICAST_DELEGATE_OneParam(FNSOnTempChanged, int64 /* Amount */ );
DECLARE_MULTICAST_DELEGATE_TwoParams(FNSOnPermanentChanged, FGameplayTag /* Type */, int64 /* Amount */);

/**
 * 어떤 재화를 얼마나 보유하고 있는지 담고있는 컴포넌트
 * 용어 정리
 * 버킷 : 런에서 재화를 습득했을때 들고있는 곳, 임시해좌는 런에서 사용시 차감, 공통/스킬재화는 인런에서 사용불가
 * 지갑 : 아웃런까지 통용되는 재화, 런 종료시 버킷 -> 지갑으로 공통/스킬재화가 반영됨
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSCurrencyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSCurrencyComponent();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	// ================================================================
	// 서버 전용 API
	// ================================================================
	
	// 임시 재화 습득
	void AddTemp(int32 Grade, int64 Amount);
	// 임시 재화 사용
	bool TrySpendTemp(int64 Amount);
	// 영구재화 획득 -> 버킷 
	void AddRunPermanent(FGameplayTag Type, int64 Amount);
	// 스테이지 보상 -> 버킷
	void AddPermanentDirect(FGameplayTag Type, int64 Amount);
	// 런 종료시 영구 재화 반영 (클리어 1배율, 전멸 0.5배율)
	void CommitRunPermanent(float Multiplier);
	
	
	int64 GetTemp() const;
	int GetPermanent(FGameplayTag Type) const;
	int64 GetAmount(FGameplayTag Type) const;
	
	FNSOnTempChanged OnTempChanged;
	FNSOnPermanentChanged OnPermanenetChanged;
	
	// FastArray 콜백이 부르는 전파 함수
	void NotifyWalletEntryChanged(const FNSCurrencyEntry& Entry);
private:
	// 지갑에 추가
	void AddToWallet(FGameplayTag Type, int64 Amount);
	
	UPROPERTY(Replicated)
	FNSCurrencyWallet Wallet;
	
	// 서버 전용 런 누적
	TMap<FGameplayTag, int64> PendingPermanent;
};
