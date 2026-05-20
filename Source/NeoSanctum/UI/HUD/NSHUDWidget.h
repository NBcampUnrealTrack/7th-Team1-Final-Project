// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSHUDWidget.generated.h"

class UNSHPShieldWidget;
class UNSGoodsWidget;

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
	//런 인 재화 UI 갱신
	UFUNCTION(BlueprintCallable,Category = "UI")
	void UpdateRunInGoods(int32 NewGoodsAmount);
	//런 아웃 재화 UI 갱신
	UFUNCTION(BlueprintCallable,Category = "UI")
	void UpdateRunOutGoods(int32 NewGoodsAmount);
	//런 인 재화 초기화
	UFUNCTION(BlueprintCallable,Category = "UI")
	void ResetRunInGoods();
private:
	//HP / Shield HUD 위젯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UNSHPShieldWidget> HPShieldWidget;
	//재화 HUD 위젯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UNSGoodsWidget> GoodsWidget;
};