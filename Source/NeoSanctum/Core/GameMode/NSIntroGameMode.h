// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NSIntroGameMode.generated.h"


UCLASS()
class NEOSANCTUM_API ANSIntroGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ANSIntroGameMode();
	
protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	
public:
	bool bEnteredViaConnectionClosed = false;
};
