	
public:
	//런 종료 결과 데이터를 받아 화면을 갱신
	UFUNCTION(BlueprintCallable, Category = "UI|RunResult")
	void SetRunResult(
		bool bCleared,
		int32 EarnedGoods,
		float RunTimeSeconds,
		int32 KillCount);
	UFUNCTION(BlueprintCallable, Category = "UI|RunResult")
    void SetVoteResult(int32 NextVotes, int32 HubVotes);
	virtual void NativePreConstruct() override;
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
