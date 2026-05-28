// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NSUIManagerSubsystem.generated.h"

class UNSHUDWidget;

/**
 * 게임 전체 UI 생성을 관리하는 서브시스템
 */
UCLASS()
class NEOSANCTUM_API UNSUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	//HUD 위젯 생성
	void CreateHUD(APlayerController* OwningPlayer);
	//HUD 화면 표시
	void ShowHUD();
	//HUD 화면 숨김
	void HideHUD();
	//런 인 재화 UI갱신
	void UpdateRunInGoods(int32 NewGoodsAmount);
	//런 아웃 재화 UI 갱신
	void UpdateRunOutGoods(int32 NewGoodsAmount);
	//런 인 재화 초기화
	void ResetRunInGoods();
	//조준점 표시
	void ShowCrosshair();
	//조준점 숨김
	void HideCrosshair();
	//조준점 색상 변경
	void SetCrosshairColor(FLinearColor NewColor);
	//HP/Shield UI 갱신
	void UpdateHealthAndShield(
		float CurrentHealth,
		float MaxHealth,
		float CurrentShield,
		float MaxShield);
	//증강 UI 표시
	void ShowAugmentation();
	//증강 UI 숨김
	void HideAugmentation();
	
	void ClearHUD();
	
	//HUD 위젯 반환
	UNSHUDWidget* GetHUDWidget() const;
	
	//(이용호 추가) Title 위젯 전용
	void CreateTitle(APlayerController* OwningPlayer);
	void ShowTitle();
	void HideTitle();
	
	//(이용호 추가) RunEnd 위젯 전용
	void CreateRunEnd(APlayerController* OwningPlayer);
	void ShowRunEnd();
	void HideRunEnd();

	
private:
	//생성된 HUD 보관
	UPROPERTY()
	TObjectPtr<UNSHUDWidget> HUDWidget;
	
	UPROPERTY()
	TObjectPtr<UUserWidget> TitleWidget;
	
	UPROPERTY()
	TObjectPtr<UUserWidget> RunEndWidget;
	
protected:
	//HUD 위젯 블루프린트
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNSHUDWidget> HUDWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> TitleWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> RunEndWidgetClass;
	
}; 