void UNSRunResultWidget::SetRunResult(
	bool bCleared,
	int32 EarnedGoods,
	float RunTimeSeconds,
	int32 KillCount)
{
	if (ResultTitleText)
	{
		ResultTitleText->SetText(
			bCleared
			? NSLOCTEXT("RunResult", "ClearTitle", "Clear")
			: NSLOCTEXT("RunResult", "FailedTitle", "FAILED"));
	}
	
	if (EarnedGoodsText)
	{
		EarnedGoodsText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "EarnedGoodsFormat", "Goods : {0}"),
			FText::AsNumber(EarnedGoods)));
	}
	
	if (RunTimeText)
	{
		RunTimeText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "RunTimeFormat", "Time: {0}"),
			FormatRunTime(RunTimeSeconds)));
	}
	
	if (KillCountText)
	{
		KillCountText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "KillCountFormat", "Kills: {0}"),
			FText::AsNumber(KillCount)));
	}
	if (NextStageButton)
	{
		NextStageButton->SetVisibility(
			bCleared
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}
	if (NextVotesText)
	{
		NextVotesText->SetVisibility(
			bCleared
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}
	if (!bCleared)
	{
		SetSelectedChoice(ENSRunChoice::ReturnToHub);
	}
	SetVoteSubmitted(false);
}

FText UNSRunResultWidget::FormatRunTime(float RunTimeSeconds) const
{
	const int32 TotalSeconds = FMath::Max(FMath::FloorToInt(RunTimeSeconds), 0);
	const int32 Minutes = TotalSeconds / 60;
	const int32 Seconds = TotalSeconds % 60;

	return FText::FromString(FString::Printf(
		TEXT("%02d:%02d"),
		Minutes,
		Seconds));
}
void UNSRunResultWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	//에디터에서 위젯을 열어쓸때 기본표시 상태 확인
	SetRunResult(false,0,0.0f,0);
}
