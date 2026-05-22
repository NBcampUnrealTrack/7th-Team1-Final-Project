// Copyright 2026 One Team. All rights reserved.

#include "NSPlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Core/Interface/NSOutGameModeInterface.h"
#include "NeoSanctum/Core/Interface/NSGameInstanceInterface.h"

ANSPlayerController::ANSPlayerController()
{
	
}

void ANSPlayerController::Server_RequestStartRun_Implementation()
{
	if (HasAuthority())
	{
		AGameModeBase* CurrentGameMode = GetWorld()->GetAuthGameMode();
		
		if (CurrentGameMode && CurrentGameMode->Implements<UNSOutGameInterface>())
		{
			INSOutGameInterface::Execute_RequestStartRun(CurrentGameMode);
		}
	}
}

void ANSPlayerController::ExitSpectatorAndRespawn()
{
	if (!HasAuthority())
	{
		return;
	}

	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (!GameMode)
	{
		return;
	}
	
	// 직접 소환
	AActor* PlayerStartSpot = GameMode->FindPlayerStart(this);
	if (!PlayerStartSpot)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerStart를 찾을 수 없음"));
		return;
	}

	APawn* NewPawn = GameMode->SpawnDefaultPawnFor(this, PlayerStartSpot);
	if (!NewPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("폰 스폰 실패"));
		return;
	}
	
	Possess(NewPawn);

	Multicast_NotifyRespawn();
}

void ANSPlayerController::Multicast_NotifyRespawn_Implementation()
{
	if (IsLocalController())
	{
		// 호스트 클라이언트 리스폰 처리용
		if (GetPawn())
		{
			SetViewTargetWithBlend(GetPawn());
		}
		
		// 로딩 UI 종료용
		if (GetGameInstance() && GetGameInstance()->Implements<UNSGameInstanceInterface>())
		{
			INSGameInstanceInterface::Execute_HideLoadingScreen(GetGameInstance());
		}
	}
	
	
}
