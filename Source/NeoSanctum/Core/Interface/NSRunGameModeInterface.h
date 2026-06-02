// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NSRunGameModeInterface.generated.h"



UINTERFACE(MinimalAPI, Blueprintable)
class UNSRunGameModeInterface : public UInterface { GENERATED_BODY() };

class NEOSANCTUM_API INSRunGameModeInterface
{
	GENERATED_BODY()

public:
	// 스테이지 클리어 처리 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameFlow")
	void NotifyStageCleared();

	// 플레이어 사망 신호 처리 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameFlow")
	void NotifyPlayerDied(AController* DeadPlayer);
	
	// 적 죽었을 때 호출 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameFlow")
	void NotifyEnemyKilled(ACharacter* DeadEnemy);
	
	// 거점 복귀 선택하면 호출될 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameFlow")
	void RequestReturnToHub();

	// 다음 스테이지 진행 선택하면 호출될 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameFlow")
	void RequestMoveToNextStage();
	
	// 몬스터 풀 반환용 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameFlow")
	void ReturnMonsterToPool(ACharacter* Monster);
};
