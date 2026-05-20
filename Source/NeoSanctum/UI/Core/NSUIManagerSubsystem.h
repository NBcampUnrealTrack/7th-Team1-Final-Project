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
	//HUD 위젯 반환
	UNSHUDWidget* GetHUDWidget() const;

	
private:
	//생성된 HUD 보관
	UPROPERTY()
	TObjectPtr<UNSHUDWidget> HUDWidget;
	
protected:
	//HUD 위젯 블루프린트
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNSHUDWidget> HUDWidgetClass;
	
}; 