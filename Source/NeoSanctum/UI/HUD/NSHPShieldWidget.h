// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSHPShieldWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 *  플레이어의 체력과 실드를 표시하는 HUD위젯
 */
UCLASS()
class NEOSANCTUM_API UNSHPShieldWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//체력 UI 갱신
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetHealth(float CurrentHealth, float MaxHealth);
	//쉴드 UI 갱신
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetShield(float CurrentShield, float MaxShield);
	//체력 수치 텍스트 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetHealthTextVisible(bool bVisible);
	//실드 수치 텍스트 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetShieldTextVisible(bool bVisible);
	//체략 실드 초기화
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ResetHealthAndShield();
	
private:
	//0 나누기 방지 후 ProgressBar 비율 계산
	float GetSafePercent(float CurrentValue, float MaxValue);
	
	//체력 바
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;
	//실드 바
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ShieldBar;
	//체력 수치 
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthText;
	//쉴드 수치 
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ShieldText;
	
protected:
	virtual void NativeConstruct()override;
};
