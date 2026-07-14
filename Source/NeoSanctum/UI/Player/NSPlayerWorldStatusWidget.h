// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSPlayerWorldStatusTypes.h"
#include "NSPlayerWorldStatusWidget.generated.h"

class UProgressBar;
class UNSPlayerWorldStatusViewModel;

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 팀원 플레이어 위에 표시되는 월드 상태 UI 위젯입니다.
 * ViewModel이 전달한 체력 비율만 화면에 반영합니다.
 */
UCLASS()
class NEOSANCTUM_API UNSPlayerWorldStatusWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// ViewModel을 연결하고 상태 변경 이벤트를 구독하는 함수
	void BindViewModel(UNSPlayerWorldStatusViewModel* InViewModel);

	// ViewModel 연결을 해제하는 함수
	void UnbindViewModel();

	// ViewModel 상태값을 위젯에 반영하는 함수
	void ApplyStatus(const FNSPlayerWorldStatusData& StatusData);

protected:
	// 위젯 파괴 시 ViewModel 구독을 해제하는 함수
	virtual void NativeDestruct() override;

private:
	// 플레이어 체력 진행도를 표시하는 ProgressBar 변수
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_HP;

	// 연결된 ViewModel을 약하게 보관하는 변수
	TWeakObjectPtr<UNSPlayerWorldStatusViewModel> BoundViewModel;
};
