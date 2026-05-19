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
	virtual void SaveBytesAsync(const TArray<uint8>& Data, FNSSaveBytesComplete OnComplete) override;
	virtual void LoadBytesAsync(FNSLoadBytesComplete OnComplete) override;

private:
	static const FString SavePath;
};
