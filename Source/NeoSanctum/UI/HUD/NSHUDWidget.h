// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSHUDWidget.generated.h"

class UNSHPShieldWidget;

/**
 * 인게임 HUD 요소를 묶어서 관리하는 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSHUDWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//HP / Shield UI 갱신
	UFUNCTION(BlueprintCallable,Category = "UI")
	void UpdateHealthAndShield(
		float CurrentHealth,
		float MaxHealth,
		float CurrentShield,
		float MaxShield
		);
private:
	//HP / Shield HUD 위젯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UNSHPShieldWidget> HPShieldWidget;
};
