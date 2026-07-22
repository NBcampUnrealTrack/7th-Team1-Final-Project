// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NSVFXDataTableRow.generated.h"

class UNiagaraSystem;

// ID 기반 VFX 재생에 필요한 기본 데이터
USTRUCT(BlueprintType)
struct NEOSANCTUM_API FNSVFXDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFXData")
	TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;

	// 첫 재생 시 Niagara 컴파일 비용이 크게 체감되는 VFX만 CommonDataReady 이후 미리 초기화.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFXData")
	bool bWarmupOnCommonDataReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFXData")
	float ScaleMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFXData")
	bool bAutoDestroy = true;
};
