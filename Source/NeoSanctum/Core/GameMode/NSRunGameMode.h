// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "NSRunGameMode.generated.h"

class UNSStageManager;

UCLASS()
class NEOSANCTUM_API ANSRunGameMode :
public AGameModeBase,
public INSRunGameModeInterface
{
	GENERATED_BODY()
	
public:
	ANSRunGameMode();

	virtual void BeginPlay() override;

	// 인터페이스 구현부 오버라이드
	virtual void NotifyStageCleared_Implementation() override;
	virtual void NotifyPlayerDied_Implementation(AController* DeadPlayer) override;
	virtual void NotifyEnemyKilled_Implementation(ACharacter* DeadEnemy) override;
	virtual void RequestReturnToHub_Implementation() override;
	virtual void RequestMoveToNextStage_Implementation() override;

	// 룸 생성 완료시 호출
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void RespawnAllPlayers();
	
	// 맵 로딩이 완료되고 블루프린트에서 호출될 함수
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void SetEnemyCount(int32 Count);

	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

private:

	void HandleRunOver(bool bIsClear);
	
	UPROPERTY()
	TObjectPtr<UNSStageManager> NSStageManager;
};
