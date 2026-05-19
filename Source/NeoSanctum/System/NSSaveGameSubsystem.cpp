// Copyright 2026 One Team. All rights reserved.

#include "NSSaveGameSubsystem.h"
#include "NSLocalSaveRepository.h"
#include "NeoSanctum/Progression/Save/NSPermanentSaveGame.h"
#include "Kismet/GameplayStatics.h"

void UNSSaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 현재는 로컬로 추후에 다른 Repo로 변경가능한 구조
	UNSLocalSaveRepository* LocalRepo = NewObject<UNSLocalSaveRepository>(this);
	Repository.SetObject(LocalRepo);
	Repository.SetInterface(Cast<INSSaveRepository>(LocalRepo));

	LoadPermanent(FNSLoadPermanentComplete::CreateLambda([this](bool, UNSPermanentSaveGame* Data)
	{
		OnPermanentDataLoaded.Broadcast(Data);
	}));
}

void UNSSaveGameSubsystem::SavePermanent(UNSPermanentSaveGame* Data, FNSSaveComplete OnComplete)
{
	if (!Data)
	{
		OnComplete.ExecuteIfBound(false);
		return;
	}

	TArray<uint8> Bytes;
	if (!UGameplayStatics::SaveGameToMemory(Data, Bytes))
	{
		OnComplete.ExecuteIfBound(false);
		return;
	}

	INSSaveRepository* Repo = Repository.GetInterface();
	if (!Repo)
	{
		OnComplete.ExecuteIfBound(false);
		return;
	}

	Repo->SaveBytesAsync(Bytes,
		FNSSaveBytesComplete::CreateLambda([OnComplete](bool bSuccess)
		{
			OnComplete.ExecuteIfBound(bSuccess);
		}));
}

void UNSSaveGameSubsystem::LoadPermanent(FNSLoadPermanentComplete OnComplete)
{
	INSSaveRepository* Repo = Repository.GetInterface();
	if (!Repo)
	{
		OnComplete.ExecuteIfBound(false, nullptr);
		return;
	}

	Repo->LoadBytesAsync(
		FNSLoadBytesComplete::CreateLambda([this, OnComplete](bool bSuccess, const TArray<uint8>& Bytes)
		{
			if (!bSuccess || Bytes.IsEmpty())
			{
				// 세이브 파일 없음 → 빈 데이터로 신규 시작
				CachedData = NewObject<UNSPermanentSaveGame>(this);
				OnComplete.ExecuteIfBound(true, CachedData);
				return;
			}

			USaveGame* Loaded = UGameplayStatics::LoadGameFromMemory(Bytes);
			CachedData = Cast<UNSPermanentSaveGame>(Loaded);
			if (!CachedData)
			{
			CachedData = NewObject<UNSPermanentSaveGame>(this);
			}
			OnComplete.ExecuteIfBound(CachedData != nullptr, CachedData);
		}));
}
