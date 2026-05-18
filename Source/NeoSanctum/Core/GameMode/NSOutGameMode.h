// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NSOutGameMode.generated.h"



UCLASS()
class NEOSANCTUM_API ANSOutGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ANSOutGameMode();

protected:
	virtual void PostLogin(APlayerController* NewPlayer) override;
};
