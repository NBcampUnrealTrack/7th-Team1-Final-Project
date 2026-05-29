// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSAugmentationWidget.generated.h"

class UCanvasPanel;
class UWeapBox;
class UNSAugmentCardWidget;

/**
 *  인게임 중 증강 선택 화면을 표시하는 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSAugmentationWidget : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	//증강 선택 UI 표시
	UFUNCTION(BlueprintCallable,Category = "UI")
	void ShowAugmentation();
	//증강 선택 UI 숨김
	UFUNCTION(BlueprintCallable,Category = "UI")
	void HideAugmentation();
	//선택지의 개수만큼 증강 선택지 생성
	UFUNCTION(BlueprintCallable,Category = "UI")
	void CreateChoiceCard(int32 NewChoiceCount);
	//키 입력으로 카드 선택
	void SelectCardByIndex(int32 CardIndex);
	//선택한 증강 적용 요청
	void ConfirmAugmentSelection(int32 CardIndex);
	//증강 선택지 리롤 요청
	void RequestRerollAugment();
	//현재 보유 증강 목록 갱신
	void RefreshOwnedAugmentList();
	
private:
	//증강 선택 카드들이 들어가는 박스
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> ChoiceRootCanvas;
	//생성된 증강 카드 목록
	UPROPERTY()
	TArray<TObjectPtr<UNSAugmentCardWidget>> AugmentCardWidgets;
	//기본 증강 선택지 개수
	int32 ChoiceCount = 3;
	//현재 하이라이트된 카드 인덱스
	int32 HighlightedCardIndex = INDEX_NONE;
	//선택한 카드만 하이라이트 처리
	void HighLightCard(int32 CardIndex);
	
protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(
		const FGeometry& InGeometry,
		const FKeyEvent& InKeyEvent) override;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNSAugmentCardWidget> AugmentCardWidgetClass;
};
