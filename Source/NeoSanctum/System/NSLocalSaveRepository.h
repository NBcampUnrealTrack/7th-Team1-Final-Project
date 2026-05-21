// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NSSaveRepository.h"
#include "NSLocalSaveRepository.generated.h"

UCLASS()
class NEOSANCTUM_API UNSLocalSaveRepository : public UObject, public INSSaveRepository
{
	GENERATED_BODY()

public:
	virtual void SaveBytesAsync(const FString& SlotName, const TArray<uint8>& Data, FNSSaveBytesComplete OnComplete) override;
	virtual void LoadBytesAsync(const FString& SlotName, FNSLoadBytesComplete OnComplete) override;

private:
	static FString GetSavePath(const FString& SlotName);
};
