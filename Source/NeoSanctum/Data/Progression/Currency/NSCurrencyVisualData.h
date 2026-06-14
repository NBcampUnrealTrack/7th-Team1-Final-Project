// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Data/Progression/Currency/NSCurrencyTypes.h"
#include "NSCurrencyVisualData.generated.h"

class UStaticMesh;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct FNSCurrencyVisualRow
{	
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag CurrencyType;
	
	// 임시 재화만 등급 구분, 공통/스킬은 None
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ENSCurrencyGrade Grade = ENSCurrencyGrade::None;
	
	// 액터가 띄울 메시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UStaticMesh> Mesh;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Scale = 1.f;
};

UCLASS()
class NEOSANCTUM_API UNSCurrencyVisualData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	const FNSCurrencyVisualRow* FindVisual(FGameplayTag CurrencyType, ENSCurrencyGrade Grade) const;
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Currency", meta=(AllowPrivateAccess="true"))
	TArray<FNSCurrencyVisualRow> Visuals;
};
