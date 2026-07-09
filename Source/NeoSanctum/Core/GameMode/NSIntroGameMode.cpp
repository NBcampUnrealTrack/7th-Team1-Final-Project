// Copyright 2026 One Team. All rights reserved.


#include "NSIntroGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "NeoSanctum/Core/PlayerController/NSIntroPlayerController.h"


ANSIntroGameMode::ANSIntroGameMode()
{
	PlayerControllerClass = ANSIntroPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	bUseSeamlessTravel = false;
}

void ANSIntroGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	UE_LOG(LogTemp, Warning, TEXT("[Intro] InitGame Options=%s"), *Options);   // 검증용
	// ?closed 로 진입했는지 판정 (UGameplayStatics::HasOption 사용)
	bEnteredViaConnectionClosed = UGameplayStatics::HasOption(Options, TEXT("closed"));
}
