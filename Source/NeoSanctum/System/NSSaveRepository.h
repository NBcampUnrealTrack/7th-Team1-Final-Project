// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NSSaveRepository.generated.h"

/**
 * 바이트 배열 저장 완료 콜백
 * @param bSuccess 저장 성공 여부
 */
DECLARE_DELEGATE_OneParam(FNSSaveBytesComplete, bool /** bSuccess */);

/**
 * 바이트 배열 로드 완료 콜백
 * @param bSuccess 저장 성공 여부
 * @param Bytes 로드된 바이너리 데이터
 */
DECLARE_DELEGATE_TwoParams(FNSLoadBytesComplete, bool /** bSuccess */, const TArray<uint8>& /** Bytes */);

UINTERFACE(MinimalAPI)
class UNSSaveRepository : public UInterface { GENERATED_BODY() };

class NEOSANCTUM_API INSSaveRepository
{
	GENERATED_BODY()

public:
	virtual void SaveBytesAsync(const TArray<uint8>& Data, FNSSaveBytesComplete OnComplete) = 0;
	virtual void LoadBytesAsync(FNSLoadBytesComplete OnComplete) = 0;
};
