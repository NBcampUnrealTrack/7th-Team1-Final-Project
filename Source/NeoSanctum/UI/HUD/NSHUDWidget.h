// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NeoSanctum/UI/Monster/NSMonsterUIHost.h"
#include "NSHUDWidget.generated.h"

class UNSHPShieldWidget;
class UNSGoodsWidget;
class UNSCrosshairWidget;
class UNSHitTakenFeedbackWidget;
class UNSAugmentationWidget;
class UNSPartPanelWidget;
class UNSOutRunGoodsWidget;
class UNSSkillSlotWidget;
class UNSDifficultyTimerWidget;
class UWidget;
class UNSCharacterStatsWidget;
class UNSMinimapWidget;
class UNSDashStackWidget;
class UHorizontalBox;
class UPanelWidget;


/**
 * 인게임 HUD 요소를 묶어서 관리하는 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSHUDWidget : public UCommonUserWidget, 
                                    public INSMonsterUIHost
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
	
	//인런 재화 UI 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowInRunGoods();

	//아웃런 재화 UI 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowOutRunGoods();
	
	//캐릭터별 스킬 UI 세트 적용
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ApplyCharacterSkillUISet(FName CharacterId);
	
	//본인 경험치 UI 갱신
	void UpdateExperience(
		float CurrentExperience,
		float RequiredExperience);
	
	void SelectAugmentCardByIndex(int32 CardIndex);

	void RequestRerollAugment();
	
	void UpdateDashStack(int32 CurrentDashCount, int32 MaxDashCount);
	
	// 일반 몬스터 상태 위젯을 배치할 패널을 반환하는 함수
	virtual UPanelWidget* GetNormalMonsterLayer() const override;

	// 보스 상태 위젯을 1:1 비율로 배치할 가로 패널을 반환하는 함수
	virtual UHorizontalBox* GetBossMonsterLayer() const override;
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
	// 플레이어 피격 반응 HUD 위젯
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UNSHitTakenFeedbackWidget> HitTakenFeedbackWidget;
	//증강 선택 HUD 위젯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UNSAugmentationWidget> AugmentationWidget;
	//파츠 장착 상태 HUD 위젯
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UNSPartPanelWidget> PartPanelWidget;
	// Tab 증강 선택 / C 파츠 인벤토리 패널이 열렸을 때 사용하는 공용 어두운 배경
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> HudDimBackground;
	//1번 스킬 슬롯
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSSkillSlotWidget> SkillSlot1Widget;
	//2번 스킬 슬롯
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSSkillSlotWidget> SkillSlot2Widget;
	//3번 스킬 슬롯
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSSkillSlotWidget> SkillSlot3Widget;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSDifficultyTimerWidget> DifficultyTimerWidget;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSCharacterStatsWidget> CharacterStatsWidget;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSMinimapWidget> MinimapWidget;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSDashStackWidget> DashStackWidget;
	
	// 일반 몬스터 상태 위젯을 배치하는 패널 변수
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> NormalMonsterLayer;

	// 보스 상태 위젯을 1:1 비율로 배치하는 가로 패널 변수
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> BossMonsterLayer;

	void RefreshHudDimBackground();
protected:
	virtual void NativeConstruct() override;
};
