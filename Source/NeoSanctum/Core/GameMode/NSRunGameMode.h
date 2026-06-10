// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "NeoSanctum/Core/GameFlow/NSRunFlowType.h"
#include "NSRunGameMode.generated.h"

class UNSStageManager;
class UNSMonsterPoolManager;

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
	virtual void ReturnMonsterToPool_Implementation(ACharacter* Monster) override;
	virtual ANSEnemyCharacterBase* RequestSpawnMonster_Implementation(
		UClass* CharacterClass,
		UNSEnemyData* EnemyData,
		const FVector& Location,
		const FRotator& Rotation) override;
	virtual void SubmitRunChoice_Implementation(APlayerController* Voter, ENSRunChoice Choice) override;
	
	// 룸 생성 완료시 호출
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void RespawnAllPlayers();
	
	// 맵 로딩이 완료되고 블루프린트에서 호출될 함수
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void SetEnemyCount(int32 Count);

	UNSMonsterPoolManager* GetMonsterPoolManager() const { return NSMonsterPoolManager; }

	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;
	
	// 에디터에서 수정 가능한 EndRun UI 투표 시간 및 투표 결과 보여줄 시간
	UPROPERTY(EditAnywhere, Category="RunEnd")
	float VoteDuration = 10.0f;
	UPROPERTY(EditAnywhere, Category="RunEnd")
	float ResultDisplayDuration = 3.0f;
	
	FTimerHandle PhaseTimerHandle;

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	
	// 보스 처치 후 호출될 투표 시작용 함수
	void OpenRunEndVote(bool bInIsClear);

	// 각각의 확인 RPC 끝에서 호출될 플레이어들 투표했는지 확인용 함수
	void HandlePlayerConfirmed();
	
	// 투표 집계용 함수
	void ResolveVote();
	
	// 투표 결과 UI 표시 후 실제 트래블 진행할 함수
	void OnResultDisplayFinished();

private:

	void HandleRunOver(bool bIsClear);
	
	UPROPERTY()
	TObjectPtr<UNSStageManager> NSStageManager;
	
	UPROPERTY()
	TObjectPtr<UNSMonsterPoolManager> NSMonsterPoolManager;
};
