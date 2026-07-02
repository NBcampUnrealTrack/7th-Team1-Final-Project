// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NSUIManagerSubsystem.generated.h"

class UNSHUDWidget;
class UDataTable;
class APlayerController;
class UUserWidget;
class UNSRunResultWidget;
class UNSSpectatorWidget;
class ANSRunGameState;


/**
 * 게임 전체 UI 생성을 관리하는 서브시스템
 */
UCLASS()
class NEOSANCTUM_API UNSUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	static UNSUIManagerSubsystem* Get(const UObject* WorldContext);

	// @원종
	void Initialize(FSubsystemCollectionBase& Collection) override;
	// @원종
	void Deinitialize() override;

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

	void ClearTitle();
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
	
	//(이용호 추가) PauseMenu 위젯 전용
	void OpenPauseMenu(APlayerController* OwningPlayer);
	void ClosePauseMenu();
	bool IsPauseMenuOpen() const { return bPauseMenuOpen; }
	void ClearPauseMenu();
	
	//(이용호 추가) OptionPanel 위젯 전용
	void OpenOptionPanel(APlayerController* OwningPlayer);
	void CloseOptionPanel();
	bool IsOptionPanelOpen() const { return bOptionPanelOpen; }
	void ClearOptionPanel();
	
	//(정주현 추가) Loading 위젯 전용
	void CreateLoadingScreen(APlayerController* OwningPlayer);
	void ShowLoadingScreen();
	void HideLoadingScreen();
	void ShowTravelLoadingScreen(APlayerController* OwningPlayer);
	void RestoreTravelLoadingScreen(APlayerController* OwningPlayer);
	void HideTravelLoadingScreen();
	bool IsTravelLoadingScreenActive() const { return bTravelLoadingScreenActive; }
	
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
	// 결과창에 표시할 처치 수를 증가
	void AddRunResultKillCount(int32 Amount = 1);
	// 결과창에 표시할 데이터를 위젯에 전달
	void UpdateRunEndResult(bool bCleared);
	//런 종료 투표 수를 결과창 위젯에 전달
	void UpdateRunEndVotes(int32 NextVotes, int32 HubVotes);

	int32 GetRunResultGoods() const { return RunResultGoods; }
	int32 GetRunResultKillCount() const { return RunResultKillCount; }
	float GetRunResultTimeSeconds() const;
	
	//런 종료 순간의 플레이 시간을 고정한다
	void CacheRunResultTime();
	
	//탄약 UI 갱신
	void UpdateAmmo(int32 CurrentAmmo, int32 MaxAmmo);
	
	//리로드 상태 UI 갱신
	void SetReloading(bool bReloading);
	
	//인런 스킬 재화 UI 갱신
	void UpdateRunSkillGoods(int32 NewGoodsAmount);
	
	//인런 재화 UI 표시
	void ShowInRunGoods();

	//아웃런 재화 UI 표시
	void ShowOutRunGoods();
	
	void RefreshOutRunGoods();

	int32 GetRunResultCommonGoods() const { return RunResultCommonGoods; }
	int32 GetRunResultSkillGoods() const { return RunResultSkillGoods; }
	
	//런 결과창에 표시할 영구재화 획득량 갱신
	void UpdateRunResultCommonGoods(int32 NewAmount);

	//런 결과창에 표시할 스킬재화 획득량 갱신
	void UpdateRunResultSkillGoods(int32 NewAmount);
	
	//캐릭터별 스킬 슬롯 UI 적용
	void ApplyCharacterSkillUISet(FName CharacterId);
	
	//관전자 위젯 생성
	void CreateSpectator(APlayerController* OwningPlayer);
	
	//관전자 위젯 표시
	void ShowSpectator(const FString& SpectatingPlayerName);
	
	//관전자 위젯 숨김
	void HideSpectator();
	
	//관전자 위젯 제거
	void ClearSpectator();
	
	void UpdateRunEndResultFromGameState(const ANSRunGameState* RunGameState);

	// @원종 추가
private:
	UFUNCTION()
	void HandleCommonDataReady();

	// CommonData에서 받은 DT_UIWidget을 RowName -> WidgetClass 캐시로 변환.
	void RebuildWidgetClassCache();

	// CommonDataReady 전에 실패했던 Title 생성을 UI 위젯 캐시 구축 후 재시도.
	void RetryPendingTitleCreation();

private:
	//생성된 HUD 보관
	UPROPERTY()
	TObjectPtr<UNSHUDWidget> HUDWidget;

	bool bAugmentationPanelOpen = false;
	
	UPROPERTY()
	TObjectPtr<UUserWidget> TitleWidget;


	// @원종: CommonDataReady 전에 Title 생성이 요청되면 로딩 완료 후 같은 PlayerController로 재시도하기 위해 보관.
	TWeakObjectPtr<APlayerController> PendingTitleOwningPlayer;
	bool bPendingTitleCreation = false;
	
	UPROPERTY()
	TObjectPtr<UUserWidget> RunEndWidget;
	
	// (이용호 추가) PauseMenu 위젯 전용
	UPROPERTY()
	TObjectPtr<UUserWidget> PauseMenuWidget;
	bool bPauseMenuOpen = false;
	
	// (이용호 추가) OptionWidget 위젯 전용
	UPROPERTY() TObjectPtr<UUserWidget> OptionWidget;
	bool bOptionPanelOpen = false;
	
	UPROPERTY()
	TObjectPtr<UUserWidget> LoadingScreenWidget;
	
	// 로딩스크린을 계속 Active할 것인가에 대한 판단을 위한 bool 변수
	bool bTravelLoadingScreenActive = false;
	
	//DataTable에서 RowName에 해당되는 위젯 조회
	TSubclassOf<UUserWidget> GetWidgetClassFromTable(
		FName RowName)const;

	//UI 위젯 정보 테이블
	UPROPERTY()
	TObjectPtr<UDataTable> UIWidgetDataTable;

	UPROPERTY()
	TMap<FName, TSubclassOf<UUserWidget>> WidgetClassCache;
	
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
	
	//런 결과창에 표시할 공통 영구재화 획득량
	int32 RunResultCommonGoods = 0;

	//런 결과창에 표시할 스킬재화 획득량
	int32 RunResultSkillGoods = 0;
	
	//관전자 상태 UI위젯
	UPROPERTY()
	TObjectPtr<UNSSpectatorWidget> SpectatorWidget;
protected:
	//HUD 위젯 블루프린트
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNSHUDWidget> HUDWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> TitleWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> RunEndWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> OptionWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNSSpectatorWidget> SpectatorWidgetClass;
}; 
