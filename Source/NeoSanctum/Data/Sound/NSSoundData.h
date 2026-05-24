// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NSSoundData.generated.h"

UENUM(BLueprintType)
enum class ENSSoundCategory : uint8
{
	BGM,
	SFX,
	UI
};

UCLASS()
class NEOSANCTUM_API UNSSoundData : public UDataTable
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	ENSSoundCategory SoundCategory = ENSSoundCategory::BGM;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	TObjectPtr<USoundBase> Sound;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	float Volume = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	float Pitch = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	bool bLoop = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	bool bAllowMultiple = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	float FadeInTime = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	float FadeOutTime = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	TObjectPtr<USoundAttenuation> Attenuation;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	TObjectPtr<USoundConcurrency> Concurrency;
};
