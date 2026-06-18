	
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
	//초 단위 시간을 mm:ss 형식으로 변환
	FText FormatRunTime(float RunTimeSeconds)const;
