// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "UObject/PrimaryAssetId.h"
#include "Engine/StreamableManager.h"
#include "NeoSanctum/Progression/Augment/NSAugmentSelectionComponent.h"
#include "NeoSanctum/Type/NSAugmentDisplayTypes.h"
#include "NSAugmentationWidget.generated.h"

class UButton;
class UCanvasPanel;
class USizeBox;
class UTextBlock;
class UWrapBox;
class UWidget;
class UWeapBox;
class UNSAugmentCardWidget;
class UNSAugmentInventoryComponent;

/**
 *  인게임 중 증강 통합 패널 (보유 증강 아이콘 상시 + 대기 시 선택 카드)
 */
UCLASS()
class NEOSANCTUM_API UNSAugmentationWidget : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	//증강 패널 열기 (보유 아이콘 갱신 + 서버에 대기 오퍼 표시 요청)
	UFUNCTION(BlueprintCallable,Category = "UI")
	void OpenPanel();
	//증강 패널 닫기
	UFUNCTION(BlueprintCallable,Category = "UI")
	void ClosePanel();
	//패널 열림 상태
	UFUNCTION(BlueprintPure,Category = "UI")
	bool IsPanelOpen() const { return bPanelOpen; }
	//선택지의 개수만큼 증강 선택지 생성
	UFUNCTION(BlueprintCallable,Category = "UI")
	void CreateChoiceCard(int32 NewChoiceCount);
	//키 입력으로 카드 선택
	void SelectCardByIndex(int32 CardIndex);
	//선택한 증강 적용 요청
	void ConfirmAugmentSelection(int32 CardIndex);
	//증강 선택지 리롤 요청
	UFUNCTION()
	void RequestRerollAugment();
	//현재 보유 증강 목록 갱신
	void RefreshOwnedAugmentList();
	void SetOwnedAugmentListVisible(bool bVisible);


private:
	//카드 영역 표시 (대기 > 0)
	void ShowCardSection();
	//카드 영역 숨김 (대기 = 0 / 선택 완료 후, 패널 자체는 유지)
	void HideCardSection();

	//카드 영역 전체 컨테이너 (대기 유무에 따라 표시/숨김) — BP 미배치 가능성 대비 Optional
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CardSectionRoot;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CardDimBackground;
	//증강 선택 카드들이 들어가는 박스
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> ChoiceRootCanvas;
	//보유 증강 아이콘 컨테이너 (패널 열린 동안 항상 표시)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> OwnedAugmentWrapBox;
	//대기 카운트 뱃지 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PendingCountText;
	//생성된 증강 카드 목록
	UPROPERTY()
	TArray<TObjectPtr<UNSAugmentCardWidget>> AugmentCardWidgets;
	// 리롤 버튼 (없어도 T키로는 계속 리롤 가능)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RerollButton;
	// "[T]" 같은 단축키 안내 위젯 (리롤 가능할 때만 버튼과 같이 보임)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> RerollShortcutHintWidget;
	// 현재 리롤 비용 표시
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RerollCostText;
	// 리롤 진행 상태와 실패 문구 표시
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RerollStatusText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CenterControlRoot;
	//남은 증강 개수 표시
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RemainingAugmentCountText;
	//기본 증강 선택지 개수
	int32 ChoiceCount = 3;
	//현재 하이라이트된 카드 인덱스
	int32 HighlightedCardIndex = INDEX_NONE;
	//패널 열림 여부
	bool bPanelOpen = false;
	//선택한 카드만 하이라이트 처리
	void HighLightCard(int32 CardIndex);

	void RefreshAugmentPanelState();
	
	//현재 오퍼의 증강 ID 목록
	TArray<FNSAugmentSelectionCard> CurrentOfferCards;
	// 마지막으로 받은 오퍼 번호 (Server_Choose/Server_RerollCard 보낼 때 같이 실어 보냄)
	int32 CurrentOfferRevision = 0;
	// 리롤 요청을 보내고 서버 응답을 기다리는 중이면 true. 이 동안 카드 선택, 추가 리롤을 막음.
	bool bRerollRequestPending = false;
	// 서버가 마지막으로 알려준 리롤 비용.
	int64 CurrentRerollCost = 0;
	// 지금 오퍼에서 리롤이 가능한지 (규칙 없는 트리거이거나 부분 오퍼면 false)
	bool bCanRerollCurrentOffer = false;
	// Bridge가 생성한 현재 오퍼 카드 표시 데이터
	UPROPERTY(Transient)
	TArray<FNSAugmentCardViewData> CurrentOfferViewData;
	//오퍼 카드 아이콘 비동기 로드 핸들 (리롤 시 이전 로드 취소용)
	TSharedPtr<FStreamableHandle> IconLoadHandle;
	//보유 아이콘 비동기 로드 핸들
	TSharedPtr<FStreamableHandle> OwnedIconLoadHandle;
	//오퍼 아이콘 로드 완료 → 카드 채우기
	void OnIconsLoaded();
	//보유 아이콘 로드 완료 → 아이콘 위젯 생성
	void OnOwnedIconsLoaded();
	//현재 오퍼 ID들을 Definition으로 풀어 각 카드에 이름/설명/아이콘 표시
	void PopulateOfferCards();
	//바인딩한 선택 컴포넌트 캐시 (소멸 시 언바인드용)ㄹ
	TWeakObjectPtr<UNSAugmentSelectionComponent> SelectionComponent;
	//바인딩한 인벤토리 컴포넌트 캐시 (소멸 시 언바인드용)
	TWeakObjectPtr<UNSAugmentInventoryComponent> InventoryComponent;
	//오너 PlayerController의 선택 컴포넌트를 찾아 캐시
	UNSAugmentSelectionComponent* GetSelectionComponent();
	//오너 PlayerState의 인벤토리 컴포넌트를 찾아 캐시
	UNSAugmentInventoryComponent* GetInventoryComponent();
	//오퍼 제시 수신 -> 카드 생성 및 표시
	UFUNCTION()
	void HandleOfferPresented(
		const TArray<FNSAugmentSelectionCard>& Cards,
		int64 RerollCost,
		bool bCanReroll,
		int32 OfferRevision
	);
	// 리롤 실패 결과 수신 -> 잠금 해제 + 문구 갱신
	UFUNCTION()
	void HandleRerollResult(
		ENSAugmentRerollResult Result,
		int64 RequiredCost,
		int64 HaveCurrency,
		int32 RequestRevision,
		int32 ServerOfferRevision
	);
	// 리롤 버튼/힌트/비용 텍스트를 지금 상태에 맞게 갱신
	void RefreshRerollControls();
	// 상태 또는 실패 문구를 보여주거나(비어있으면) 숨김
	void SetRerollStatusMessage(const FText& Message);
	//오퍼 종료 수신 -> 카드 영역 숨김 (패널은 유지)
	UFUNCTION()
	void HandleOfferClosed();
	//대기 카운트 변경 수신 -> 뱃지 갱신
	UFUNCTION()
	void HandlePendingCountChanged(int32 NewCount);
	//보유 증강 변경 수신 -> 아이콘 목록 갱신
	UFUNCTION()
	void HandleInventoryChanged();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNSAugmentCardWidget> AugmentCardWidgetClass;

	// 보유 증강 아이콘 크기 (에디터에서 BP별 조정 가능)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FVector2D OwnedIconSize = FVector2D(48.f, 48.f);
};
