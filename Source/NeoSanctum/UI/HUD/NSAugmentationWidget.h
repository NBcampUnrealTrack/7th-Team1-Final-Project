// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "UObject/PrimaryAssetId.h"
#include "Engine/StreamableManager.h"
#include "NeoSanctum/Progression/Augment/NSAugmentSelectionComponent.h"
#include "NeoSanctum/Type/NSAugmentDisplayTypes.h"
#include "TimerManager.h"
#include "NSAugmentationWidget.generated.h"

class UButton;
class UCanvasPanel;
class USizeBox;
class UTextBlock;
class UWidget;
class UWrapBox;
class UNSAugmentCardWidget;
class UNSAugmentInventoryComponent;
class UNSCurrencyComponent;

/**
 * 인게임 중 증강 통합 패널입니다.
 * 보유 증강 목록, 대기 중인 증강 수, 선택 카드, 리롤 UI를 관리합니다.
 */
UCLASS()
class NEOSANCTUM_API UNSAugmentationWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// 증강 패널 열기
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenPanel();

	// 증강 패널 닫기
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ClosePanel();

	// 패널 열림 상태
	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsPanelOpen() const { return bPanelOpen; }

	// 선택지 개수만큼 증강 선택 카드 생성
	UFUNCTION(BlueprintCallable, Category = "UI")
	void CreateChoiceCard(int32 NewChoiceCount);

	// 숫자 입력으로 카드 선택
	void SelectCardByIndex(int32 CardIndex);

	// 선택한 증강 적용 요청
	void ConfirmAugmentSelection(int32 CardIndex);

	// 증강 선택지 리롤 요청
	UFUNCTION()
	void RequestRerollAugment();

	// 현재 리롤 비용을 감당할 수 있는지
	bool CanAffordReroll();

	// 리롤 요청 중이 아니면서 재화가 충분한지
	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsRerollAvailable();

	// 현재 보유 증강 목록 갱신
	void RefreshOwnedAugmentList();

	void SetOwnedAugmentListVisible(bool bVisible);

	void OpenSelectionPanel();

	// 1/2/3/4 카드 선택 키 입력이 실제로 수락됐을 때 선택 사운드를 재생
	void PlayAugmentSelectSound() const;

	// T 리롤 성공 응답을 받았을 때 성공 사운드를 재생
	void PlayAugmentRerollSuccessSound() const;

	// T 리롤 실패 응답을 받았을 때 실패 사운드를 재생
	void PlayAugmentRerollFailSound() const;

	// Tab 증강 패널 토글 키 입력이 실제로 수락됐을 때 탭 사운드를 재생
	void PlayAugmentTabSound() const;

private:
	// 카드 영역 표시
	void ShowCardSection();

	// 카드 영역 숨김
	void HideCardSection();

	// 현재 선택지 개수와 패널 상태에 맞춰 3장/4장 입력 가이드 표시를 갱신
	void RefreshChoiceGuideVisibility();

	// WBP에 배치된 입력 아이콘 박스 위치를 기준으로 카드 위치를 갱신
	void RefreshChoiceCardPositions();

	// 카드 인덱스에 대응하는 카드 위치를 계산
	FVector2D GetChoiceCardPosition(int32 Index) const;

	// Canvas Panel 좌표계에서 위젯의 TopLeft/Size를 조회
	bool TryGetWidgetCanvasRect(
		const UWidget* Widget,
		FVector2D& OutTopLeft,
		FVector2D& OutSize) const;

	// 현재 ChoiceCount와 카드 인덱스에 맞는 입력 아이콘 박스를 반환
	USizeBox* GetChoiceGuideInputIconBox(int32 CardIndex) const;

	// 현재 오퍼에서 실제 선택 가능한 카드 수를 반환합니다.
	int32 GetSelectableChoiceCount() const;

	// 입력 슬롯 인덱스에 맞는 WBP 입력 아이콘 박스를 반환합니다.
	USizeBox* GetChoiceGuideInputIconBoxBySlotIndex(int32 SlotIndex) const;

	// 입력 슬롯 인덱스 기준으로 카드 위치를 계산합니다.
	FVector2D GetChoiceCardPositionBySlotIndex(int32 SlotIndex) const;

	// 생성된 실제 선택 카드 위젯을 모두 제거합니다.
	void ClearChoiceCardWidgets();

	// 카드 인덱스를 현재 ChoiceGuide의 입력 슬롯 인덱스로 변환
	int32 GetChoiceGuideSlotIndexForCardIndex(int32 CardIndex) const;

	// 전체 남은 카드 수를 고정된 전체 슬롯에 균등 분배합니다.
	// 분배 결과가 0인 슬롯은 계산 이후 비활성화합니다.
	TArray<int32> CalculateCardsPerSlot(
		int32 TotalRemainingCardCount) const;

	// 논리적인 카드 스택과 실제 오퍼 카드가 모두 존재하는 슬롯인지 확인합니다.
	bool IsChoiceGuideSlotActive(
		int32 SlotIndex) const;

	void QueueChoiceCardPositionRefresh();
	void HandleDeferredChoiceCardPositionRefresh();

	// 카드 영역 전체 컨테이너
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CardSectionRoot;

	// 증강 선택 카드가 들어가는 Canvas Panel
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> ChoiceRootCanvas;

	// 3장 선택지 입력 가이드 루트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ChoiceGuide3Root;

	// 4장 선택지 입력 가이드 루트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ChoiceGuide4Root;

	// 3장 선택지의 1번 입력 아이콘 박스
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> Choice3InputIcon1Box;

	// 3장 선택지의 2번 입력 아이콘 박스
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> Choice3InputIcon2Box;

	// 3장 선택지의 3번 입력 아이콘 박스
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> Choice3InputIcon3Box;

	// 4장 선택지의 1번 입력 아이콘 박스
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> Choice4InputIcon1Box;

	// 4장 선택지의 2번 입력 아이콘 박스
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> Choice4InputIcon2Box;

	// 4장 선택지의 3번 입력 아이콘 박스
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> Choice4InputIcon3Box;

	// 4장 선택지의 4번 입력 아이콘 박스
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> Choice4InputIcon4Box;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> OwnedAugmentPanelRoot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> OwnedAugmentListRoot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> CommonAugmentSectionRoot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> RareAugmentSectionRoot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> EpicAugmentSectionRoot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> LegendaryAugmentSectionRoot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> CommonAugmentWrapBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> RareAugmentWrapBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> EpicAugmentWrapBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> LegendaryAugmentWrapBox;

	// 대기 카운트 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PendingCountText;

	// 생성된 증강 카드 위젯 목록
	UPROPERTY()
	TArray<TObjectPtr<UNSAugmentCardWidget>> AugmentCardWidgets;

	// 리롤 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RerollButton;

	// 리롤 T 입력 아이콘
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> RerollInputIcon;

	// 현재 리롤 비용 표시
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RerollCostText;

	// 리롤 진행 상태/실패 메시지 표시. WBP에 없어도 동작합니다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RerollStatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CenterControlRoot;

	// 남은 증강 개수 표시
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RemainingAugmentCountText;

	// Tab 입력 안내 아이콘/버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> TabButton;

	// 기본 증강 선택지 개수
	int32 ChoiceCount = 3;

	// 서버가 확정한 최대 선택 슬롯 수
	int32 ChoiceGuideSlotCount = 3;

	// 현재 하이라이트된 카드 인덱스
	int32 HighlightedCardIndex = INDEX_NONE;

	// 패널 열림 여부
	bool bPanelOpen = false;

	// C 입력으로 보유 증강 목록을 요청한 상태인지
	bool bOwnedListRequested = false;

	// 선택한 카드만 하이라이트 처리
	void HighLightCard(int32 CardIndex);

	void RefreshAugmentPanelState();

	// 현재 오퍼의 증강 ID 목록
	TArray<FNSAugmentSelectionCard> CurrentOfferCards;

	// 서버가 전달한 현재 전체 선택 가능 후보 수
	int32 CurrentAvailableCardCount = 0;

	// 마지막으로 받은 오퍼 번호
	int32 CurrentOfferRevision = 0;

	// 리롤 요청을 보내고 서버 응답을 기다리는 중인지
	bool bRerollRequestPending = false;

	// 서버가 마지막으로 알려준 리롤 비용
	int64 CurrentRerollCost = 0;

	// 지금 오퍼에서 리롤이 가능한지
	bool bCanRerollCurrentOffer = false;

	// Bridge가 생성한 현재 오퍼 카드 표시 데이터
	UPROPERTY(Transient)
	TArray<FNSAugmentCardViewData> CurrentOfferViewData;

	// 오퍼 카드 아이콘 비동기 로드 핸들
	TSharedPtr<FStreamableHandle> IconLoadHandle;

	// 보유 아이콘 비동기 로드 핸들
	TSharedPtr<FStreamableHandle> OwnedIconLoadHandle;

	//오퍼 아이콘 로드 완료 → 카드 채우기
	void OnIconsLoaded();

	//보유 아이콘 로드 완료 → 아이콘 위젯 생성
	void OnOwnedIconsLoaded();

	//현재 오퍼 ID들을 Definition으로 풀어 각 카드에 이름/설명/아이콘 표시
	void PopulateOfferCards();

	//바인딩한 선택 컴포넌트 캐시 (소멸 시 언바인드용)
	TWeakObjectPtr<UNSAugmentSelectionComponent> SelectionComponent;

	//바인딩한 인벤토리 컴포넌트 캐시 (소멸 시 언바인드용)
	TWeakObjectPtr<UNSAugmentInventoryComponent> InventoryComponent;

	// 바인딩한 재화 컴포넌트 캐시 (소멸 시 언바인드용)
	TWeakObjectPtr<UNSCurrencyComponent> CurrencyComponent;

	//오너 PlayerController의 선택 컴포넌트를 찾아 캐시
	UNSAugmentSelectionComponent* GetSelectionComponent();

	//오너 PlayerState의 인벤토리 컴포넌트를 찾아 캐시
	UNSAugmentInventoryComponent* GetInventoryComponent();

	// 오너 PlayerState의 재화 컴포넌트를 찾아 캐시
	UNSCurrencyComponent* GetCurrencyComponent();

	//오퍼 제시 수신 -> 카드 생성 및 표시
	UFUNCTION()
	void HandleOfferPresented(
		const TArray<FNSAugmentSelectionCard>& Cards,
		int64 RerollCost,
		bool bCanReroll,
		int32 OfferRevision,
		int32 MaxChoiceCount,
		int32 AvailableCardCount);

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

	bool AreOwnedAugmentListReady() const;

	void ClearOwnedAugmentLists();

	void RefreshOwnedAugmentSectionVisibility();

	UWrapBox* GetOwnedAugmentWrapBox(ENSAugmentRarity Rarity) const;

	// 선택 카드의 시각 연출을 시작
	void BeginCardSelection(int32 CardIndex);

	// 선택 애니메이션이 끝난 뒤 실제 선택 요청
	void FinishPendingCardSelection();

	// 모든 선택 카드의 선택 연출 상태를 초기화
	void ResetChoiceCardSelectionVisuals();

	// 지연 선택 타이머와 선택 대기 상태를 초기화
	void ClearSelectionAnimationTimer();

	// 전달된 DT_SoundDataTable RowName으로 증강 UI 2D 사운드를 재생
	void PlayAugmentSound(FName SoundID) const;

	// 최대 선택 슬롯 수를 WBP에서 지원하는 3/4 슬롯 값으로 정규화
	int32 NormalizeChoiceGuideSlotCount(int32 MaxChoiceCount) const;

	// 실제 카드가 없는 키 슬롯을 레이아웃 공간 유지 상태로 숨김
	void RefreshChoiceGuideSlotVisibility();

	// 개별 키 슬롯을 표시하거나 공간만 유지한 채 숨김
	void SetChoiceGuideSlotVisible(UWidget* SlotWidget, bool bVisible) const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNSAugmentCardWidget> AugmentCardWidgetClass;

	// 1/2/3/4 카드 선택 키 입력에 사용할 DT_SoundDataTable RowName
	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment|Sound")
	FName AugmentSelectSoundID = NAME_None;

	// Tab 증강 패널 토글 키 입력에 사용할 DT_SoundDataTable RowName
	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment|Sound")
	FName AugmentTabSoundID = NAME_None;

	// T 리롤 성공 시 사용할 DT_SoundDataTable RowName
	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment|Sound")
	FName AugmentRerollSuccessSoundID = NAME_None;

	// T 리롤 실패 시 사용할 DT_SoundDataTable RowName
	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment|Sound")
	FName AugmentRerollFailSoundID = NAME_None;

	// 보유 증강 아이콘 크기
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FVector2D OwnedIconSize = FVector2D(48.f, 48.f);

	// 입력 가이드 아이콘과 카드 사이 간격
	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment")
	float ChoiceCardGuideGap = 25.f;

	// 선택 카드의 실제 표시 크기
	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment")
	FVector2D ChoiceCardSize = FVector2D(330.f, 118.f);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Choice3Slot1GuideRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Choice3Slot2GuideRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Choice3Slot3GuideRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Choice4Slot1GuideRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Choice4Slot2GuideRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Choice4Slot3GuideRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Choice4Slot4GuideRoot;

	bool bChoiceCardPositionRefreshQueued = false;

	// 선택 애니메이션 종료 후 서버 선택 요청을 보내기 위한 타이머
	FTimerHandle SelectionAnimationTimerHandle;

	// 애니메이션 종료 후 선택 요청을 보낼 카드 인덱스
	int32 PendingSelectedCardIndex = INDEX_NONE;

	// 선택 애니메이션이 재생 중인지 나타냄
	bool bSelectionAnimationPlaying = false;

	// 선택 애니메이션 후 실제 선택 요청까지 기다릴 시간
	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment|Selection")
	float SelectionAnimationDelay = 0.5f;

	// 선택되지 않은 카드에 적용할 투명도
	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment|Selection")
	float DeselectedChoiceCardOpacity = 0.35f;

	// 현재 사용 가능한 전체 카드를 활성 슬롯별로 균등 분배한 결과.
	// 실제 스택 위젯을 생성하지 않고 논리 상태로만 유지합니다.
	UPROPERTY(Transient)
	TArray<int32> CurrentCardsPerSlot;
};
