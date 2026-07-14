// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSPlayerWorldStatusTypes.h"
#include "NSPlayerWorldStatusWidget.generated.h"

class UCommonTextBlock;
class UProgressBar;
class USizeBox;
class UNSPlayerWorldStatusViewModel;

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 팀원 플레이어 위에 표시되는 월드 상태 UI 위젯입니다.
 * ViewModel이 전달한 이름과 체력 비율을 표시 정책에 맞게 화면에 반영합니다.
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

	// 이름과 체력바 표시 여부를 변경하는 함수
	UFUNCTION(BlueprintCallable, Category = "NS|PlayerWorldStatus")
	void SetDisplayOptions(bool bInShowName, bool bInShowHealth);

protected:
	// 위젯 파괴 시 ViewModel 구독을 해제하는 함수
	virtual void NativeDestruct() override;

private:
	// bool 값에 따라 표시 상태를 반환하는 함수
	ESlateVisibility ResolveVisibility(bool bVisible) const;

private:
	// 이름 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerWorldStatus|Display",
		meta = (AllowPrivateAccess = "true"))
	bool bShowName = true;

	// 체력바 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerWorldStatus|Display",
		meta = (AllowPrivateAccess = "true"))
	bool bShowHealth = true;

	// 이름 영역을 숨기거나 표시하는 SizeBox 변수
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> SizeBox_Name;

	// 체력바 영역을 숨기거나 표시하는 SizeBox 변수
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> SizeBox_Health;

	// 플레이어 이름을 표시하는 CommonText 변수
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CommonText_PlayerName;

	// 플레이어 체력 진행도를 표시하는 ProgressBar 변수
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar_HP;

	// 연결된 ViewModel을 약하게 보관하는 변수
	TWeakObjectPtr<UNSPlayerWorldStatusViewModel> BoundViewModel;
};
