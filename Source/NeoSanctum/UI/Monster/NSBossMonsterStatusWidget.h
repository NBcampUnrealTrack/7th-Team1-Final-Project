// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSMonsterUITypes.h"
#include "NSBossMonsterStatusWidget.generated.h"

class UCommonTextBlock;
class UProgressBar;
class USizeBox;
class UNSMonsterStatusViewModel;

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 보스 몬스터 상태를 HUD 상단에 표시하는 위젯입니다.
 * ViewModel이 전달한 이름, Percent, 표시 정책만 화면에 반영합니다.
 */
UCLASS()
class NEOSANCTUM_API UNSBossMonsterStatusWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// ViewModel을 연결하고 상태 변경 이벤트를 구독하는 함수
	void BindViewModel(UNSMonsterStatusViewModel* InViewModel);

	// ViewModel 연결을 해제하는 함수
	void UnbindViewModel();

	// ViewModel 상태값을 위젯에 반영하는 함수
	void ApplyStatus(const FNSMonsterUIStatus& Status);

protected:
	// 위젯 파괴 시 ViewModel 구독을 해제하는 함수
	virtual void NativeDestruct() override;

private:
	// bool 값에 따라 표시 상태를 반환하는 함수
	ESlateVisibility ResolveVisibility(bool bVisible) const;

private:
	// 보스 이름을 표시하는 CommonText 변수
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CommonText_BossName;

	// 체력 수치를 표시하는 CommonText 변수
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CommonText_HPValue;

	// 실드 수치를 표시하는 CommonText 변수
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CommonText_ShieldValue;

	// 피격 게이지 수치를 표시하는 CommonText 변수
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CommonText_HitGaugeValue;

	// 실드 영역을 숨기거나 표시하는 SizeBox 변수
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> SizeBox_Shield;

	// 체력 진행도를 표시하는 ProgressBar 변수
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_HP;

	// 실드 진행도를 표시하는 ProgressBar 변수
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar_Shield;

	// 피격 게이지 진행도를 표시하는 ProgressBar 변수
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_HitGauge;

	// 연결된 ViewModel을 약하게 보관하는 변수
	TWeakObjectPtr<UNSMonsterStatusViewModel> BoundViewModel;
};
