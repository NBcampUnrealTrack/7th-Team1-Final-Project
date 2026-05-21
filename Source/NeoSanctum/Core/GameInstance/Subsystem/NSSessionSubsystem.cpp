// Copyright 2026 One Team. All rights reserved.


#include "NSSessionSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"


void UNSSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (OnlineSubsystem)
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();
	}
}

void UNSSessionSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UNSSessionSubsystem::CreateSession()
{
	if (!SessionInterface.IsValid())
	{
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}

	LastSessionSettings = MakeShared<FOnlineSessionSettings>();
	LastSessionSettings->bIsLANMatch = true;
	LastSessionSettings->NumPublicConnections = 4;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bUsesPresence = false;

	CreateSessionDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UNSSessionSubsystem::OnCreateSessionCompleted)
	);

	const ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
	SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings);
}

void UNSSessionSubsystem::JoinSessionByAddress(const FString& Address)
{
	APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
	if (PlayerController)
	{
		// 입력받은 주소로 직접 맵 이동
		PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
		OnJoinSessionComplete.Broadcast(true);
	}
	else
	{
		OnJoinSessionComplete.Broadcast(false);
	}
}

void UNSSessionSubsystem::DestroySession()
{
	if (!SessionInterface.IsValid())
	{
		OnDestroySessionComplete.Broadcast(false);
		return;
	}

	DestroySessionDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UNSSessionSubsystem::OnDestroySessionCompleted)
	);

	SessionInterface->DestroySession(NAME_GameSession);
}

void UNSSessionSubsystem::OnCreateSessionCompleted(FName SessionName, bool bWasSuccessful)
{
	SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);

	if (bWasSuccessful)
	{
		// 후에 실제 맵 경로로 교체 필요
		GetWorld()->ServerTravel("/Game/TestSpace/TestInGame?listen");
	}

	OnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UNSSessionSubsystem::OnDestroySessionCompleted(FName SessionName, bool bWasSuccessful)
{
	SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
	OnDestroySessionComplete.Broadcast(bWasSuccessful);
}
