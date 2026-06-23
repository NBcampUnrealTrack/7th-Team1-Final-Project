class UCommonButtonBase;
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> ReadyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> StartButton;
	
	UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCommonButtonBase> CloseButton;
	UFUNCTION()
	void HandleReadyClicked();

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleCloseClicked();
	void CloseWidget();
