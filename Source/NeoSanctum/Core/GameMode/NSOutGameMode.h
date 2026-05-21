// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Core/Interface/NSOutGameModeInterface.h"
#include "NSOutGameMode.generated.h"



UCLASS()
class NEOSANCTUM_API ANSOutGameMode : public AGameModeBase, public INSOutGameInterface
{
	GENERATED_BODY()
	
public:
	ANSOutGameMode();
	
	// 런 레벨 이동용 함수
	virtual void RequestStartRun_Implementation() override;

protected:
	virtual void PostLogin(APlayerController* NewPlayer) override;
};
