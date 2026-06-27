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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFXData")
	float ScaleMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFXData")
	bool bAutoDestroy = true;
};
