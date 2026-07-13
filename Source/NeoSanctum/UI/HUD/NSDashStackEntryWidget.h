// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSDashStackEntryWidget.generated.h"

class UImage;
class UHorizontalBox;
class UNSDashStackEntryWidget;

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSDashStackEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	void SetActive(bool bActive);
	
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ActiveImage;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> InactiveImage;
};
