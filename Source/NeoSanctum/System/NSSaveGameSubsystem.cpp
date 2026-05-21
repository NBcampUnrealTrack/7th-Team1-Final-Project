// Copyright 2026 One Team. All rights reserved.

#include "NSSaveGameSubsystem.h"
#include "NSLocalSaveRepository.h"
#include "NeoSanctum/Progression/Save/NSPermanentSaveGame.h"
#include "Kismet/GameplayStatics.h"

// TODO : 영구 데이터 저장 슬롯명. 향후 캐릭터별 파일로 분리 시 슬롯 키를 캐릭터 ID로 바꿀 수 있음
const FString UNSSaveGameSubsystem::PermanentSlotName = TEXT("Permanent");

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

	// 플레이한 캐릭터의 정보만 저장하고, 다른 캐릭터 정보는 보존을 위해 캐시에 머지
	UNSPermanentSaveGame* ToSave = Data;
	if (CachedData && CachedData != Data)
	{
		for (const TPair<FName, FNSCharacterSaveData>& Pair : Data->Characters)
		{
			CachedData->Characters.Add(Pair.Key, Pair.Value);
		}
		if (!Data->LastSelectedCharacterId.IsNone())
		{
			CachedData->LastSelectedCharacterId = Data->LastSelectedCharacterId;
		}
		ToSave = CachedData;
	}

	TArray<uint8> Bytes;
	if (!UGameplayStatics::SaveGameToMemory(ToSave, Bytes))
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

	Repo->SaveBytesAsync(PermanentSlotName, Bytes,
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

	Repo->LoadBytesAsync(PermanentSlotName,
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
