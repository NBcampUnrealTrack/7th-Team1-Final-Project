// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NSPlayerController.generated.h"

UCLASS()
class NEOSANCTUM_API ANSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ANSPlayerController();
	
	// 게임 시작 요청
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_RequestStartRun();
};
