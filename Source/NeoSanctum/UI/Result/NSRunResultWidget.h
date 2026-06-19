// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NeoSanctum/Core/GameFlow/NSRunFlowType.h"
#include "NSRunResultWidget.generated.h"

class UCommonTextBlock;
class UTextBlock;
class UCommonButtonBase;


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
		int32 SkillGoods,
		float RunTimeSeconds,
		int32 KillCount);
	UFUNCTION(BlueprintCallable, Category = "UI|RunResult")
    void SetVoteResult(int32 NextVotes, int32 HubVotes);
    
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime) override;
	
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
	
	//확인 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> ConfirmButton;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> NextVotesText;
	
	UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> HubVotesText;
	
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> ConfirmButtonText;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> TimerText;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CommonGoodsText;

	//스킬재화 획득량 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> SkillGoodsText;
	
	UFUNCTION()
	void HandleNextStageClicked();

	UFUNCTION()
	void HandleReturnToHubClicked();

	UFUNCTION()
	void HandleConfirmClicked();

	void SetSelectedChoice(ENSRunChoice NewChoice);

	ENSRunChoice SelectedChoice = ENSRunChoice::ReturnToHub;
	bool bHasSelectedChoice = false;
	
	//초 단위 시간을 mm:ss 형식으로 변환
	FText FormatRunTime(float RunTimeSeconds)const;
	
	// 투표 제출 여부에 따라 확인 버튼을 확인/취소 상태로 전환한다.
    void SetVoteSubmitted(bool bSubmitted);
	
	void UpdatePhaseTimerText();
	
	bool bVoteSubmitted = false;
};
