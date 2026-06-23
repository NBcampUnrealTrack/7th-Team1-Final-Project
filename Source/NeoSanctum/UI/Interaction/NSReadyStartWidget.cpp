
	if (ReadyButton)
	{
		ReadyButton->OnClicked().AddUObject(
			this,
			&UNSReadyStartWidget::HandleReadyClicked);
	}

	if (StartButton)
	{
		StartButton->OnClicked().AddUObject(
			this,
			&UNSReadyStartWidget::HandleStartClicked);

		const bool bIsHost =
			NSPlayerController && NSPlayerController->HasAuthority();

		const ESlateVisibility StartVisibility =
			bIsHost
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed;

		StartButton->SetVisibility(StartVisibility);

		if (StartButtonText)
		{
			StartButtonText->SetVisibility(StartVisibility);
		}
	}
	
	if (CloseButton)
	{
		CloseButton->OnClicked().AddUObject(
			this,
			&UNSReadyStartWidget::HandleCloseClicked);
void UNSReadyStartWidget::NativeDestruct()
{
	UnbindReadyStateChanged();
	
	if (ReadyButton)
	{
		ReadyButton->OnClicked().RemoveAll(this);
	}

	if (StartButton)
	{
		StartButton->OnClicked().RemoveAll(this);
	}
	
	if (CloseButton)
	{
		CloseButton->OnClicked().RemoveAll(this);
	}

	Super::NativeDestruct();
}
void UNSReadyStartWidget::HandleReadyClicked()
{
	ANSPlayerController* NSPlayerController =
		Cast<ANSPlayerController>(GetOwningPlayer());

	if (!NSPlayerController)
	{
		return;
	}

	// RequestReady는 서버 RPC를 통해 실제 Ready 상태를 토글한다.
	// 버튼 텍스트는 입력 직후 반응하도록 로컬 예상값을 먼저 갱신한다.
	bLocalReadySelected = !bLocalReadySelected;

	NSPlayerController->RequestReady();

	RefreshReadyButtonText();
	RefreshReadyStatusText();
}
void UNSReadyStartWidget::HandleStartClicked()
{
	ANSPlayerController* NSPlayerController =
		Cast<ANSPlayerController>(GetOwningPlayer());

	if (!NSPlayerController)
	{
		return;
	}

	NSPlayerController->Server_RequestStartRun();
}

void UNSReadyStartWidget::HandleCloseClicked()
{
	CloseWidget();
}

void UNSReadyStartWidget::CloseWidget()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(false);

		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}

	RemoveFromParent();
}

void UNSReadyStartWidget::RefreshReadyButtonText()
{
		if (!ReadyButtonText)
		{
			return;
		}

		ReadyButtonText->SetText(
			bLocalReadySelected
				? NSLOCTEXT("ReadyStartWidget", "CancelReady", "취소")
				: NSLOCTEXT("ReadyStartWidget", "Ready", "준비"));
}

void UNSReadyStartWidget::InitializeButtonText()
{
	if (StartButtonText)
	{
		StartButtonText->SetText(
			NSLOCTEXT("ReadyStartWidget", "Start", "시작"));
	}
