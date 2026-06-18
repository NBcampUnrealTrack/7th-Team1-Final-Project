// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NSUIManagerSubsystem.generated.h"

class UNSHUDWidget;
class UDataTable;
class APlayerController;
class UUserWidget;


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
	//증강 패널 열기 (Tab 토글 / 자동 오픈)
	void OpenAugmentationPanel();
	//증강 패널 닫기 (Tab 토글)
	void CloseAugmentationPanel();
	//증강 패널 열림 여부 (InputBinder 게이팅용)
	bool IsAugmentationPanelOpen() const { return bAugmentationPanelOpen; }

	void ClearHUD();
	
	void SelectAugmentCardByIndex(int32 CardIndex);

	void RequestRerollAugment();
	
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
	void ClearRunEnd();
	
	//파츠 패널 열기
	void OpenPartPanel();
	//파츠 패널 닫기
	void ClosePartPanel();
	//파츠 패널 열림 여부
	bool IsPartPanelOpen() const
	{
		return bPartPanelOpen;
	}
	//인런 빌드 패널을 열거나 닫음
	void ToggleRunBuildPanel();
	//인런 빌드 패널을 연다
	void OpenRunBuildPanel();
	//인런 빌드 패널을 닫음
	void CloseRunBuildPanel();
	//인런 빌드 패널 알림
	bool IsRunBuildPanelOpen() const
	{
		return bRunBuildPanelOpen;
	}
	
	//런 시작 시 결과 표시용 집계값을 초기화
	void ResetRunResultStats();
	// 결과창에 표시할 데이터를 위젯에 전달
	void UpdateRunEndResult(bool bCleared);

	float GetRunResultTimeSeconds() const;
	//런 종료 순간의 플레이 시간을 고정한다
	void CacheRunResultTime();
	UNSUIManagerSubsystem();
private:
	//생성된 HUD 보관
	UPROPERTY()
	TObjectPtr<UNSHUDWidget> HUDWidget;

	bool bAugmentationPanelOpen = false;
	
	UPROPERTY()
	TObjectPtr<UUserWidget> TitleWidget;
	
	UPROPERTY()
	TObjectPtr<UUserWidget> RunEndWidget;
	
	//DataTable에서 RowName에 해당되는 위젯 조회
	TSubclassOf<UUserWidget> GetWidgetClassFromTable(
		FName RowName)const;
	//UI 위젯 정보 테이블
	UPROPERTY()
	TObjectPtr<UDataTable> UIWidgetDataTable;
	
	bool bPartPanelOpen = false;
	
	bool bRunBuildPanelOpen = false;
	
	//런 결과창에 표시할 획득 재화
	int32 RunResultGoods = 0;

	//런 결과창에 표시할 처치 수
	int32 RunResultKillCount = 0;

	//런 시작 시점의 월드 시간
	float RunStartWorldTimeSeconds = -1.0f;
	
	//런 종료 순간에 고정된 플레이 시간
	float CachedRunResultTimeSeconds = 0.0f;

	//플레이 시간을 이미 고정했는지 여부
	bool bRunResultTimeCached = false;
protected:
	//HUD 위젯 블루프린트
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNSHUDWidget> HUDWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> TitleWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> RunEndWidgetClass;
	
}; 