// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Data/UI/NSCharacterSelectData.h"
#include "CommonUserWidget.h"
#include "NSCharacterSlotWidget.generated.h"

class UTextBlock;

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSCharacterSlotWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//캐릭터 데이터
	UFUNCTION(BlueprintCallable, Category = "CharacterSelect")
	void SetCharacterData(const FNSCharacterSelectData& InData);
	
protected:
	virtual void NativeConstruct() override;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UTextBlock> SlotNameText;
	
	UPROPERTY(BlueprintReadOnly, Category="CharacterSelect")
	FNSCharacterSelectData CharacterData;
};
