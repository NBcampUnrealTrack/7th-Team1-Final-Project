// Copyright 2026 One Team. All rights reserved.

#include "NSLocalSaveRepository.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

const FString UNSLocalSaveRepository::SavePath = FPaths::ProjectSavedDir() / TEXT("SaveGames/NS_Permanent.sav");

void UNSLocalSaveRepository::SaveBytesAsync(const TArray<uint8>& Data, FNSSaveBytesComplete OnComplete)
{
	const bool bSuccess = FFileHelper::SaveArrayToFile(Data, *SavePath);
	OnComplete.ExecuteIfBound(bSuccess);
}

void UNSLocalSaveRepository::LoadBytesAsync(FNSLoadBytesComplete OnComplete)
{
	if (!FPaths::FileExists(SavePath))
	{
		OnComplete.ExecuteIfBound(false, TArray<uint8>{});
		return;
	}

	TArray<uint8> Bytes;
	const bool bSuccess = FFileHelper::LoadFileToArray(Bytes, *SavePath);
	OnComplete.ExecuteIfBound(bSuccess, Bytes);
}
