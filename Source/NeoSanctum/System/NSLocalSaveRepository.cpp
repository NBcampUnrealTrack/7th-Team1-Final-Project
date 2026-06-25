// Copyright 2026 One Team. All rights reserved.

#include "NSLocalSaveRepository.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Async/Async.h"

FString UNSLocalSaveRepository::GetSavePath(const FString& SlotName)
{
	return FPaths::ProjectSavedDir() / TEXT("SaveGames") / FString::Printf(TEXT("NS_%s.sav"), *SlotName);
}

void UNSLocalSaveRepository::SaveBytesAsync(const FString& SlotName, const TArray<uint8>& Data, FNSSaveBytesComplete OnComplete)
{
	TArray<uint8> DataCopy = Data;
	FString Path = GetSavePath(SlotName);
	Async(EAsyncExecution::ThreadPool, [DataCopy = MoveTemp(DataCopy), Path = MoveTemp(Path), OnComplete]()
	{
		const bool bSuccess = FFileHelper::SaveArrayToFile(DataCopy, *Path);
		AsyncTask(ENamedThreads::GameThread, [bSuccess, OnComplete]()
		{
			OnComplete.ExecuteIfBound(bSuccess);
		});
	});
}

void UNSLocalSaveRepository::LoadBytesAsync(const FString& SlotName, FNSLoadBytesComplete OnComplete)
{
	FString Path = GetSavePath(SlotName);
	Async(EAsyncExecution::ThreadPool, [Path = MoveTemp(Path), OnComplete]()
	{
		if (!FPaths::FileExists(Path))
		{
			AsyncTask(ENamedThreads::GameThread, [OnComplete]()
			{
				OnComplete.ExecuteIfBound(false, TArray<uint8>{});
			});
			return;
		}

		TArray<uint8> Bytes;
		const bool bSuccess = FFileHelper::LoadFileToArray(Bytes, *Path);
		AsyncTask(ENamedThreads::GameThread, [bSuccess, Bytes = MoveTemp(Bytes), OnComplete]()
		{
			OnComplete.ExecuteIfBound(bSuccess, Bytes);
		});
	});
}
