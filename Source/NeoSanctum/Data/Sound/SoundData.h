#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SoundData.generated.h"


UENUM(BlueprintType)
enum class ENSSoundCategory : uint8
{
	BGM,
	SFX,
	UI
};

USTRUCT(BlueprintType)
struct NEOSANCTUM_API FNSSoundDataTableRow : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	ENSSoundCategory Category = ENSSoundCategory::BGM;
	
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
	float StartTime = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	TObjectPtr<USoundAttenuation> Attenuation;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	TObjectPtr<USoundConcurrency> Concurrency;
};
