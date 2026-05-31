// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "UObject/PrimaryAssetId.h"
#include "Engine/StreamableManager.h"
#include "NSAugmentationWidget.generated.h"

class UCanvasPanel;
class UWeapBox;
class UNSAugmentCardWidget;
class UNSAugmentSelectionComponent;
class UNSInputBinderComponent;
class APlayerController;

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

	//현재 오퍼의 증강 ID 목록
	TArray<FPrimaryAssetId> CurrentOfferIds;
	//오퍼 자산 비동기 로드 핸들 (리롤 시 이전 로드 취소용)
	TSharedPtr<FStreamableHandle> IconLoadHandle;
	//비동기 로드 완료 → 카드 채우고 필요하면 UI 표시
	void OnIconsLoaded();
	//현재 오퍼 ID들을 Definition으로 풀어 각 카드에 이름/설명/아이콘 표시
	void PopulateOfferCards();
	//바인딩한 선택 컴포넌트 캐시 (소멸 시 언바인드용)
	TWeakObjectPtr<UNSAugmentSelectionComponent> SelectionComponent;
	//오너 PlayerController의 선택 컴포넌트를 찾아 캐시
	UNSAugmentSelectionComponent* GetSelectionComponent();
	//오너 폰의 입력 바인더 컴포넌트 반환 (증강 입력 모드 전환용)
	UNSInputBinderComponent* GetOwningInputBinder(APlayerController* PC) const;
	//오퍼 제시 수신 -> 카드 생성 및 표시
	UFUNCTION()
	void HandleOfferPresented(const TArray<FPrimaryAssetId>& OfferIds, int32 RerollCost);
	//오퍼 종료 수신 -> 증강 UI 숨김
	UFUNCTION()
	void HandleOfferClosed();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNSAugmentCardWidget> AugmentCardWidgetClass;
};
