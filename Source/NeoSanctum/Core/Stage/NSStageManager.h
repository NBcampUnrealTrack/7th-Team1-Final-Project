// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSStageManager.generated.h"

UCLASS()
class NEOSANCTUM_API ANSStageManager : public AActor
{
	GENERATED_BODY()

public:	
	ANSStageManager();

protected:
	virtual void BeginPlay() override;

private:
	// 스테이지 클리어시 호출
	void CheckStageClearCondition();
	
};
