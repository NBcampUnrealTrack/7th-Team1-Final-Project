// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSHUDWidget.generated.h"

class UNSHPShieldWidget;
class UNSGoodsWidget;
class UNSCrosshairWidget;
class UNSAugmentationWidget;
class UNSPartPanelWidget;
class UNSAmmoWidget;
class UNSOutRunGoodsWidget;


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
	//조준점 표시
	UFUNCTION(BlueprintCallable,Category = "UI")
	void ShowCrosshair();
	//조준점 숨김
	UFUNCTION(BlueprintCallable,Category = "UI")
	void HideCrosshair();
	//조준점 색상 변경
	UFUNCTION(BlueprintCallable,Category = "UI")
	void SetCrosshairColor(FLinearColor NewColor);
	//증강 패널 열기
	UFUNCTION(BlueprintCallable,Category = "UI")
	void OpenAugmentationPanel();
	//증강 패널 닫기
	UFUNCTION(BlueprintCallable,Category = "UI")
	void CloseAugmentationPanel();
	//파츠 패널 열기
	UFUNCTION(BlueprintCallable,Category = "UI")
	void OpenPartPanel();
	//파츠 패널 닫기
	UFUNCTION(BlueprintCallable,Category = "UI")
	void ClosePartPanel();
	// 인런 빌드 패널 열기
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenRunBuildPanel();
	// 인런 빌드 패널 닫음
	UFUNCTION(BlueprintCallable, Category = "UI")
	void CloseRunBuildPanel();
	
	//탄약 UI 갱신
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateAmmo(int32 CurrentAmmo, int32 MaxAmmo);
	
	//리로드 상태 UI 갱신
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetReloading(bool bReloading);
	
	//인런 스킬 재화 UI 갱신
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateRunSkillGoods(int32 NewGoodsAmount);
	
	//인런 재화 UI 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowInRunGoods();

	//아웃런 재화 UI 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowOutRunGoods();
	
	void SelectAugmentCardByIndex(int32 CardIndex);

	void RequestRerollAugment();
private:
	//HP / Shield HUD 위젯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UNSHPShieldWidget> HPShieldWidget;
	//재화 HUD 위젯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UNSGoodsWidget> GoodsWidget;
	//아웃런 재화 HUD 위젯
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UNSOutRunGoodsWidget> OutRunGoodsWidget;
	//조준점 HUD 위젯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UNSCrosshairWidget> CrosshairWidget;
	//증강 선택 HUD 위젯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UNSAugmentationWidget> AugmentationWidget;
	//파츠 장착 상태 HUD 위젯
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UNSPartPanelWidget> PartPanelWidget;
	//탄약 HUD 위젯
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UNSAmmoWidget> AmmoWidget;
	
protected:
	virtual void NativeConstruct() override;
};