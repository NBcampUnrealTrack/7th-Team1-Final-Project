// Copyright 2026 One Team. All rights reserved.

#include "NSReadyStartWidget.h"
#include "NeoSanctum/UI/Common/NSButtonBase.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "CommonTextBlock.h"
#include "NSFriendEntryWidget.h"
#include "NSReadyPlayerEntry.h"
#include "HAL/PlatformApplicationMisc.h"
#include "InputCoreTypes.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSessionSubsystem.h"
#include "NeoSanctum/Core/GameState/NSOutGameState.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"

void UNSReadyStartWidget::NativeConstruct()
{
	Super::NativeConstruct();

	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController)
	{
		// UI Only에서는 PlayerController의 ESC 입력도 차단되므로 이 위젯이 직접 키 입력을 받는다.
		SetIsFocusable(true);
		PlayerController->SetShowMouseCursor(true);

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);

		SetFocus();
	}
	
	ANSPlayerController* NSPlayerController =
	Cast<ANSPlayerController>(GetOwningPlayer());

	const ANSPlayerState* PlayerState =
		NSPlayerController
			? NSPlayerController->GetPlayerState<ANSPlayerState>()
			: nullptr;

	bLocalReadySelected = PlayerState && PlayerState->IsReady();

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
			StartButtonText->SetVisibility(
				bIsHost
					? ESlateVisibility::SelfHitTestInvisible
					: ESlateVisibility::Collapsed);
		}
	}
	
	if (CloseButton)
	{
		CloseButton->OnClicked().AddUObject(
			this,
			&UNSReadyStartWidget::HandleCloseClicked);
	}
	
	ReadyImages = { ReadyImage0, ReadyImage1, ReadyImage2, ReadyImage3 };
	PlayerRows = { PlayerRow0, PlayerRow1, PlayerRow2, PlayerRow3 };
	
	InitializeButtonText();
	RefreshReadyButtonText();
	if (ReadyButtonText)
	{
		ReadyButtonText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	
	BindReadyStateChanged();
	RefreshReadyStatusText();
	
	// 세션 관련 기능
	UNSSessionSubsystem* Session =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<UNSSessionSubsystem>() : nullptr;

	if (CreateSessionButton)
	{
		CreateSessionButton->OnClicked().AddUObject(
			this,
			&UNSReadyStartWidget::OnClickedCreateSession);
	}
	if (CopyCodeButton)
	{
		CopyCodeButton->OnClicked().AddUObject(
			this, 
			&UNSReadyStartWidget::OnClickedCopyCode);
	}
	if (JoinByCodeButton)
	{
		JoinByCodeButton->OnClicked().AddUObject(
			this, 
			&UNSReadyStartWidget::OnClickedJoinByCode);
	}

	if (Session)
	{
		Session->OnInviteCodeReady.RemoveDynamic(
			this, 
			&UNSReadyStartWidget::HandleInviteCodeReady);
		Session->OnInviteCodeReady.AddDynamic(
			this, 
			&UNSReadyStartWidget::HandleInviteCodeReady);

		const FString ExistingCode = Session->GetCurrentInviteCode();
		if (!ExistingCode.IsEmpty())
		{
			HandleInviteCodeReady(ExistingCode);
		}

		Session->OnFriendsListUpdated.RemoveDynamic(
			this,
			&UNSReadyStartWidget::HandleFriendsListUpdated);
		Session->OnFriendsListUpdated.AddDynamic(
			this, 
			&UNSReadyStartWidget::HandleFriendsListUpdated);
		
		if (FriendSearchBox)
		{
			FriendSearchBox->OnTextChanged.RemoveAll(this);
			FriendSearchBox->OnTextChanged.AddDynamic(
				this,
				&UNSReadyStartWidget::OnFriendSearchChanged);
		}

		
		Session->RequestFriendsList();
	}
}

void UNSReadyStartWidget::NativeDestruct()
{
	UnbindReadyStateChanged();
	
	// 세션 델리게이트 해제
	if (UNSSessionSubsystem* Session =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<UNSSessionSubsystem>() : nullptr)
	{
		Session->OnInviteCodeReady.RemoveDynamic(
			this,
			&UNSReadyStartWidget::HandleInviteCodeReady);
		Session->OnFriendsListUpdated.RemoveDynamic(
			this,
			&UNSReadyStartWidget::HandleFriendsListUpdated);
	}
	
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
	
	if (CreateSessionButton)
	{
		CreateSessionButton->OnClicked().RemoveAll(this);
	}
	
	if (CopyCodeButton)
	{
		CopyCodeButton->OnClicked().RemoveAll(this);
	}
	
	if (JoinByCodeButton)
	{
		JoinByCodeButton->OnClicked().RemoveAll(this);
	}

	Super::NativeDestruct();
}

FReply UNSReadyStartWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		CloseWidget();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
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
	if (bIsClosing)
	{
		return;
	}

	bIsClosing = true;

	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(false);

		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}

	RemoveFromParent();
	OnWidgetClosed.Broadcast(this, PlayerController);
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
}

void UNSReadyStartWidget::OnClickedCreateSession()
{
	if (UNSSessionSubsystem* Session =
		GetGameInstance()->GetSubsystem<UNSSessionSubsystem>())
	{
		Session->CreateSession();
	}
}

void UNSReadyStartWidget::HandleInviteCodeReady(const FString& InviteCode)
{
	CurrentInviteCode = InviteCode;
	if (InviteCodeText)
	{
		InviteCodeText->SetText(FText::FromString(InviteCode));
	}
}

void UNSReadyStartWidget::OnClickedCopyCode()
{
	FString InviteCode = CurrentInviteCode;

	if (UNSSessionSubsystem* Session =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<UNSSessionSubsystem>() : nullptr)
	{
		InviteCode = Session->GetCurrentInviteCode();
		HandleInviteCodeReady(InviteCode);
	}

	if (!InviteCode.IsEmpty())
	{
		FPlatformApplicationMisc::ClipboardCopy(*InviteCode);
	}
}

void UNSReadyStartWidget::OnClickedJoinByCode()
{
	if (!CodeInputBox)
	{
		return;
	}

	const FString EnteredCode = CodeInputBox->GetText().ToString().TrimStartAndEnd();
	if (EnteredCode.IsEmpty())
	{
		return;
	}

	if (UNSSessionSubsystem* Session =
		GetGameInstance()->GetSubsystem<UNSSessionSubsystem>())
	{
		Session->JoinSessionByCode(EnteredCode);
	}
}

void UNSReadyStartWidget::HandleFriendsListUpdated()
{
	RefreshFriendList();
}

void UNSReadyStartWidget::RefreshFriendList()
{
	if (!FriendListContainer || !FriendEntryClass)
	{
		return;
	}

	UNSSessionSubsystem* Session =
		GetGameInstance() ?
		GetGameInstance()->GetSubsystem<UNSSessionSubsystem>() : nullptr;
	if (!Session)
	{
		return;
	}

	// 기존 항목 비우기
	FriendListContainer->ClearChildren();

	// 캐시된 친구 목록 꺼내기
	TArray<FNSFriendInfo> Friends;
	Session->GetCachedFriends(Friends);

	for (const FNSFriendInfo& FriendInfo : Friends)
	{
		// 검색어가 있으면 닉네임에 포함 안 되는 친구는 건너뜀
		if (!CurrentFriendFilter.IsEmpty() &&
			!FriendInfo.DisplayName.Contains(CurrentFriendFilter, ESearchCase::IgnoreCase))
		{
			continue;
		}
		
		UNSFriendEntryWidget* Entry =
			CreateWidget<UNSFriendEntryWidget>(this, FriendEntryClass);
		if (Entry)
		{
			Entry->Setup(FriendInfo);
			FriendListContainer->AddChild(Entry);
		}
	}
}

void UNSReadyStartWidget::OnFriendSearchChanged(const FText& Text)
{
	CurrentFriendFilter = Text.ToString().TrimStartAndEnd();
	// 필터 반영해 목록 다시 그림
	RefreshFriendList();  
}

void UNSReadyStartWidget::RefreshReadyStatusText()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		return;
	}

	int32 RowIndex = 0;

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const ANSPlayerState* NSPlayerState = Cast<ANSPlayerState>(PlayerState);
		if (!NSPlayerState)
		{
			continue;
		}
		
		// 슬롯(4개) 초과 방지
		if (!PlayerRows.IsValidIndex(RowIndex))
		{
			break;
		}  

		const bool bIsLocalPlayer =
			GetOwningPlayer() && NSPlayerState == GetOwningPlayer()->PlayerState;
		const bool bIsReady =
			bIsLocalPlayer ? bLocalReadySelected : NSPlayerState->IsReady();

		// CharacterData의 HUDPortraitTexture (소프트 참조라 로드 필요)
		UTexture2D* ClassIcon = nullptr;
		FString ClassName = TEXT("Unknown"); 
		if (const UNSCharacterData* CharacterData = NSPlayerState->GetCurrentCharacterData())
		{
			if (!CharacterData->HUDPortraitTexture.IsNull())
			{
				ClassIcon = CharacterData->HUDPortraitTexture.LoadSynchronous();
			}
			
			if (CharacterData->CharacterTag.IsValid())
			{
				FString TagString = CharacterData->CharacterTag.GetTagName().ToString();
				FString LeftString;
				FString RightString;

				if (TagString.Split(
					TEXT("."),
					&LeftString,
					&RightString,
					ESearchCase::IgnoreCase,
					ESearchDir::FromEnd))
				{
					// 마지막 마디만 추출
					ClassName = RightString;   
				}
				else
				{
					ClassName = TagString;
				}
			}
		}

		if (UNSReadyPlayerEntry* Row = PlayerRows[RowIndex])
		{
			Row->SetPlayer(ClassIcon, NSPlayerState->GetPlayerName(), ClassName, bIsReady);
		}

		++RowIndex;
	}

	// 남는 슬롯 비우기
	for (int32 i = RowIndex; i < PlayerRows.Num(); ++i)
	{
		if (PlayerRows[i]) { PlayerRows[i]->ClearSlot(); }
	}
	
	RefreshReadySummary();
}

void UNSReadyStartWidget::BindReadyStateChanged()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ANSOutGameState* OutGameState =
		World->GetGameState<ANSOutGameState>();

	if (!OutGameState)
	{
		return;
	}

	// 중복 바인딩을 방지한 뒤 Ready 상태 변경 알림을 구독한다.
	OutGameState->OnReadyStateChanged.RemoveDynamic(
		this,
		&UNSReadyStartWidget::RefreshReadyStatusText);

	OutGameState->OnReadyStateChanged.AddDynamic(
		this,
		&UNSReadyStartWidget::RefreshReadyStatusText);
}

void UNSReadyStartWidget::UnbindReadyStateChanged()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ANSOutGameState* OutGameState =
		World->GetGameState<ANSOutGameState>();

	if (!OutGameState)
	{
		return;
	}

	OutGameState->OnReadyStateChanged.RemoveDynamic(
		this,
		&UNSReadyStartWidget::RefreshReadyStatusText);
}

void UNSReadyStartWidget::RefreshReadySummary()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		return;
	}

	// 분자: 레디 인원 / 분모: 전체 인원
	int32 ReadyCount = 0;
	const int32 TotalCount = GameState->PlayerArray.Num();

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const ANSPlayerState* NSPlayerState = Cast<ANSPlayerState>(PlayerState);
		if (!NSPlayerState)
		{
			continue;
		}

		const bool bIsLocalPlayer =
			GetOwningPlayer() &&
			NSPlayerState == GetOwningPlayer()->PlayerState;
		
		const bool bIsReady =
			bIsLocalPlayer ? bLocalReadySelected : NSPlayerState->IsReady();

		if (bIsReady)
		{
			++ReadyCount;
		}
	}

	// 텍스트 "2 / 4"
	if (ReadyCountText)
	{
		ReadyCountText->SetText(FText::Format(
			NSLOCTEXT("ReadyStartWidget", "ReadyCountFormat", "분대 준비 {0} / {1}"),
			FText::AsNumber(ReadyCount),
			FText::AsNumber(TotalCount)));
	}
	
	for (int32 i = 0; i < ReadyImages.Num(); ++i)
	{
		if (!ReadyImages[i]) { continue; }
		
		if (i >= TotalCount)
		{
			ReadyImages[i]->SetVisibility(ESlateVisibility::Hidden);
			continue;
		}

		ReadyImages[i]->SetVisibility(ESlateVisibility::HitTestInvisible);

		UObject* Sprite =
			(i < ReadyCount) ? ReadyActiveSprite : ReadyDefaultSprite;
		if (Sprite)
		{
			ReadyImages[i]->SetBrushResourceObject(Sprite);
		}
	}
}
