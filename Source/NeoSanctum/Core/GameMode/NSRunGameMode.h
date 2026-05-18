// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NSRunGameMode.generated.h"



UCLASS()
class NEOSANCTUM_API ANSRunGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ANSRunGameMode();

	virtual void BeginPlay() override;

	void StartNextStage();
	void OnStageCleared();

	// 플레이어 사망 시 전멸 여부 판정용
	void NotifyPlayerDied(AController* DeadPlayer);

private:

	void HandleRunOver(bool bIsClear);
};
