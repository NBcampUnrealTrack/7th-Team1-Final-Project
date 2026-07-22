// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSGuideChecklistItemWidget.generated.h"

class UCommonTextBlock;
class UWidgetAnimation;

class UNSGuideChecklistItemWidget;
/** 완료 애니메이션이 끝났을 때 컨테이너에 알리는 델리게이트 (해당 아이템 전달) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGuideChecklistItemComplete, UNSGuideChecklistItemWidget*, Item);

/**
 * 가이드 체크리스트 한 줄
 * 완료 시 CompleteAnimation(체크+취소선→위로 날아가 사라짐)을 재생하고,
 * 애니메이션이 끝나면 OnItemCompleteFinished로 컨테이너에 알린다 (컨테이너가 이 줄을 제거)
 */
UCLASS()
class NEOSANCTUM_API UNSGuideChecklistItemWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// 표시할 식별자 + 문구 설정
	void Setup(FName InItemId, const FText& InText);

	// 완료 애니메이션 재생 (없으면 즉시 완료 통보)
	void PlayCompleteAnimation();

	FName GetItemId() const { return ItemId; }

	// 완료 애니메이션 종료 알림 (컨테이너가 구독)
	UPROPERTY()
	FOnGuideChecklistItemComplete OnItemCompleteFinished;

protected:
	// 줄 문구 (필수 바인딩 — 없으면 표시 불가)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> ItemText;

	// 체크+취소선→위로 사라짐 애니메이션 (WBP에서 제작)
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> CompleteAnimation;

private:
	// CompleteAnimation 종료 시 호출 — OnItemCompleteFinished 브로드캐스트
	UFUNCTION()
	void HandleCompleteAnimationFinished();

	// 이 줄의 식별자 (코드 상수와 일치)
	FName ItemId;
};
