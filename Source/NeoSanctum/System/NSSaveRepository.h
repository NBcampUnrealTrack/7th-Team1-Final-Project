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
	/**
	 * 저장/로드 비동기 순수가상함수
	 * @param SlotName: 저장 단위 식별자. 동일 슬롯명은 덮어쓰기, 다른 슬롯명은 별개 파일로 취급
	 */
	virtual void SaveBytesAsync(const FString& SlotName, const TArray<uint8>& Data, FNSSaveBytesComplete OnComplete) = 0;
	virtual void LoadBytesAsync(const FString& SlotName, FNSLoadBytesComplete OnComplete) = 0;
};
