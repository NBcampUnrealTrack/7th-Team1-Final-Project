// Copyright 2026 One Team. All rights reserved.

#include "NSLocalSaveRepository.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Async/Async.h"

const FString UNSLocalSaveRepository::SavePath = FPaths::ProjectSavedDir() / TEXT("SaveGames/NS_Permanent.sav");

void UNSLocalSaveRepository::SaveBytesAsync(const TArray<uint8>& Data, FNSSaveBytesComplete OnComplete)
{
	TArray<uint8> DataCopy = Data;
	Async(EAsyncExecution::ThreadPool, [DataCopy = MoveTemp(DataCopy), OnComplete]()
	{
		const bool bSuccess = FFileHelper::SaveArrayToFile(DataCopy, *SavePath);
		AsyncTask(ENamedThreads::GameThread, [bSuccess, OnComplete]()
		{
			OnComplete.ExecuteIfBound(bSuccess);
		});
	});
}

void UNSLocalSaveRepository::LoadBytesAsync(FNSLoadBytesComplete OnComplete)
{
	Async(EAsyncExecution::ThreadPool, [OnComplete]()
	{
		if (!FPaths::FileExists(SavePath))
		{
			AsyncTask(ENamedThreads::GameThread, [OnComplete]()
			{
				OnComplete.ExecuteIfBound(false, TArray<uint8>{});
			});
			return;
		}

		TArray<uint8> Bytes;
		const bool bSuccess = FFileHelper::LoadFileToArray(Bytes, *SavePath);
		AsyncTask(ENamedThreads::GameThread, [bSuccess, Bytes = MoveTemp(Bytes), OnComplete]()
		{
			OnComplete.ExecuteIfBound(bSuccess, Bytes);
		});
	});
}
