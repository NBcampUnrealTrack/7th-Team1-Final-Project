// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NeoSanctum/Core/GameFlow/NSRunFlowType.h"
#include "NSRunGameModeInterface.generated.h"

class ACharacter;
class AController;
class APlayerController;
class ANSEnemyCharacterBase;
class UNSEnemyData;
class ANSEnemyPawnBase; 

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
	void NotifyEnemyKilled(AActor* DeadEnemy, AController* Killer);
	
	// 거점 복귀 선택하면 호출될 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameFlow")
	void RequestReturnToHub();

	// 다음 스테이지 진행 선택하면 호출될 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameFlow")
	void RequestMoveToNextStage();
	
	// 몬스터 풀 반환용 함수 (적 캐릭터 디졸브 완료 후 호출)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameFlow")
	void ReturnMonsterToPool(ACharacter* Monster);
	
	// 스포너가 몬스터 스폰 요청
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameFlow")
	ANSEnemyCharacterBase* RequestSpawnMonster(
		UClass* CharacterClass,
		UNSEnemyData* EnemyData,
		const FVector& Location,
		const FRotator& Rotation);
	
	// 투표 확정 입력 함수 요청
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GameFlow")
	void SubmitRunChoice(APlayerController* Voter, ENSRunChoice Choice);
	// 투표 취소 입력 함수 요청
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GameFlow")
	void CancelRunChoice(APlayerController* Voter);
	// 스테이지에 있는 npc 구출했을 때 알릴 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameFlow")
	void NotifyNPCRescued(FName RescuedNPCId);
	// 보스 진입 볼륨 타이머 시간 완료 시 호출: 전원 텔레포트 + 보스 페이즈 진입
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameFlow")
	void NotifyBossGateReached();
	// 보스 스폰 요청
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameFlow")
	ANSEnemyPawnBase* RequestSpawnBoss(
		UClass* BossClass, 
		UNSEnemyData* EnemyData,
		const FVector& Location,
		const FRotator& Rotation);
};
