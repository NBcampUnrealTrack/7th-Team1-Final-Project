// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSNoticePopupWidget.generated.h"

class UTextBlock;

// 팝업이 화면에 유지되는 방식
UENUM()
enum class ENSNoticeMode : uint8
{
	// Dismiss()가 호출될 때까지 유지 — "저장하는 중..." 같은 진행 표시용
	Blocking,
	// 지정 시간 뒤 자동으로 사라지고, 팝업 클릭으로도 즉시 닫힘 — "업그레이드 성공", "재화 부족" 같은 토스트용
	Toast,
};

/**
 * 짧은 안내 문구를 띄우는 범용 팝업 위젯.
 * 레이아웃/배경/딤 처리는 WBP에서 구성하고, C++은 문구 세팅과 수명(자동 소멸 타이머)만 관리한다.
 * 사용처: 아웃런 파츠 저장 진행 표시, 인런 파츠 업그레이드/리롤/구매 결과 토스트.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NEOSANCTUM_API UNSNoticePopupWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// Dismiss() 호출 전까지 계속 표시. 이미 떠 있으면 문구만 교체한다.
	UFUNCTION(BlueprintCallable, Category = "UI|Notice")
	void ShowBlocking(const FText& Message);

	// Duration초 뒤 자동으로 닫히는 토스트 표시. 다시 호출하면 문구 교체 + 타이머 리셋.
	UFUNCTION(BlueprintCallable, Category = "UI|Notice")
	void ShowToast(const FText& Message, float Duration = 1.5f);

	// 팝업을 화면에서 제거 (인스턴스는 재사용을 위해 파괴하지 않음)
	UFUNCTION(BlueprintCallable, Category = "UI|Notice")
	void Dismiss();

protected:
	// Toast 모드에서 팝업 클릭 시 즉시 닫기 (Blocking 모드는 무시)
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 안내 문구 표시용 (WBP에서 같은 이름으로 배치)
	UPROPERTY(BlueprintReadOnly, Category = "UI|Notice", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NoticeText;

private:
	// 문구 세팅 + 뷰포트 표시 공통 처리
	void ShowInternal(const FText& Message, ENSNoticeMode InMode);

	ENSNoticeMode Mode = ENSNoticeMode::Toast;
	FTimerHandle DismissTimerHandle;
};
