// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NeoSanctum/Core/GameFlow/NSRunFlowType.h"
#include "NSRunResultWidget.generated.h"

class UCommonTextBlock;
class UTextBlock;
class UCommonButtonBase;
class UCommonButtonStyle;
class UNSVoterEntry;
class UWidget;
class UImage;


/**
 * 인런 종료 결과를 표시하는 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSRunResultWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//런 종료 결과 데이터를 받아 화면을 갱신
	UFUNCTION(BlueprintCallable, Category = "UI|RunResult")
	void SetRunResult(
		bool bCleared,
		int32 EarnedGoods,
		int32 CommonGoods,
		float RunTimeSeconds,
		int32 KillCount);
	UFUNCTION(BlueprintCallable, Category = "UI|RunResult")
    void SetVoteResult(int32 NextVotes, int32 HubVotes);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run Result|Button")
	TSubclassOf<UCommonButtonStyle> RunEndButtonStyle;
    
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime) override;
	virtual void NativeOnInitialized() override;
	
private:
	//클리어 / 실패 결과 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> ResultTitleText;
	
	//획득 재화 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> EarnedGoodsText;
	
	//런 진행 시간 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> RunTimeText;
	
	//처치 수 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> KillCountText;
	
	//다음 스테이지 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> NextStageButton;
	
	//거점 복귀 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> ReturnToHubButton;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> NextVotesText;
	
	//방금 끝난 스테이지 번호 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> StageInfoText;
	
	UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> HubVotesText;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> TimerText;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CommonGoodsText;
	
	// 투표 진행 인원수
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> VoteCountText;
	
	// 다음 스테이지 슬롯 4개
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSVoterEntry> NextRow0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSVoterEntry> NextRow1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSVoterEntry> NextRow2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSVoterEntry> NextRow3;

	// 거점 복귀 슬롯 4개
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSVoterEntry> HubRow0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSVoterEntry> HubRow1;
	UPROPERTY(meta = (BindWidgetOptional)) 
	TObjectPtr<UNSVoterEntry> HubRow2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSVoterEntry> HubRow3;
	
	// 투표 완료 점 아이콘 4개 
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> VoteImage0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> VoteImage1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> VoteImage2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> VoteImage3;

	// 다음 스테이지 패널 전체(실패 시 숨김용)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> NextVotePanel;

	// 순회 편의용 배열
	UPROPERTY()
	TArray<TObjectPtr<UNSVoterEntry>> NextRows;
	UPROPERTY()
	TArray<TObjectPtr<UNSVoterEntry>> HubRows;
	
	UPROPERTY()
	TArray<TObjectPtr<UImage>> VoteImages;
	
	UFUNCTION()
	void HandleNextStageClicked();

	UFUNCTION()
	void HandleReturnToHubClicked();

	void SetSelectedChoice(ENSRunChoice NewChoice);

	ENSRunChoice SelectedChoice = ENSRunChoice::ReturnToHub;
	bool bHasSelectedChoice = false;
	
	//초 단위 시간을 mm:ss 형식으로 변환
	FText FormatRunTime(float RunTimeSeconds)const;
	
	void UpdatePhaseTimerText();
	
	void RefreshVoteVoters();
	
	UFUNCTION()
	void RefreshVoteInfo();
	void RefreshVoteSummary();
	void BindRunEndVoteChanged();
	void UnbindRunEndVoteChanged();
	
	//선택한 투표를 서버에 즉시 전달한다.
	void SubmitVote(ENSRunChoice NewChoice);

	//현재 선택에 맞춰 두 투표 버튼의 활성화 상태를 갱신한다.
	void UpdateVoteButtonState();

	//로컬 PlayerState에 복제된 투표 상태를 UI에 반영한다.
	void RefreshLocalVoteSelection();
	
	bool bLastRunCleared = false;
};
