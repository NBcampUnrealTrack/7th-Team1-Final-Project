// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "NSRunGameMode.generated.h"

class ANSDroppedPart;
enum class ENSRunChoice : uint8;
class UNSStageManager;
class UNSMonsterPoolManager;
class ANSEnemyCharacterBase;
class UNSEnemyData;
class UNSProjectileManagerComponent;
class ANSProjectileReplicationProxy;
class ANSCurrencyReplicationProxy;
class APlayerController;
class ANSHealReplicationProxy;
class ANSEnemyPawnBase;

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
	virtual void NotifyEnemyKilled_Implementation(AActor* DeadEnemy) override;
	virtual void RequestReturnToHub_Implementation() override;
	virtual void RequestMoveToNextStage_Implementation() override;
	virtual void ReturnMonsterToPool_Implementation(ACharacter* Monster) override;
	virtual ANSEnemyCharacterBase* RequestSpawnMonster_Implementation(
		UClass* CharacterClass,
		UNSEnemyData* EnemyData,
		const FVector& Location,
		const FRotator& Rotation) override;
	virtual void NotifyNPCRescued_Implementation(FName RescuedNPCId) override;
	// 보스 게이트 도달 처리
	virtual void NotifyBossGateReached_Implementation() override;
	// 보스 스폰용
	virtual ANSEnemyPawnBase* RequestSpawnBoss_Implementation(
	UClass* BossClass,
	UNSEnemyData* EnemyData,
	const FVector& Location,
	const FRotator& Rotation) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void HandleSeamlessTravelPlayer(AController*& Controller) override;

	virtual void SubmitRunChoice_Implementation(APlayerController* Voter, ENSRunChoice Choice) override;
	virtual void CancelRunChoice_Implementation(APlayerController* PlayerController) override;
	
	// InitializeStage에서 호출
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void RespawnAllPlayers();
	
	// 맵 로딩이 완료되고 블루프린트에서 호출될 함수
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void SetEnemyCount(int32 Count);
	
	// 스테이지 진입할 때 호출될 함수: 목표 랜덤 선택, 초기화
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void InitializeStage();

	UNSMonsterPoolManager* GetMonsterPoolManager() const { return NSMonsterPoolManager; }

	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;
	
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
	// 인런 진행도 저장용 함수
	void SaveAllPlayersProgress();
	
	// 목표 달성 시 호출용, 보스 진입 페이즈로 전환
	void HandleObjectiveComplete();
	
	// 인런 보상 재화용 변수
	UPROPERTY(EditAnywhere, Category="RunEnd|Reward")
	int64 StageClearCommonReward = 0;
	
private:
	// 인런 월드가 열린 뒤, GameFlow가 보관환 데이터 구성을 RunGameState에 복제.
	void SyncRunDataConfigToGameState();
	
	// 현재 목표 진행 상태를 GameState에 복제(UI 표시용)
	void PushObjectiveStateToGameState();
	
	// 목표 풀에서 랜덤 선택 후 StageManager/GameState 초기화
	void InitializeObjectiveInternal();
	
	// 인런 데이터 준비 완료 후 목표를 초기화하기 위한 콜백
	UFUNCTION()
	void HandleRunDataReadyForObjective();
	
	// 보스룸의 보스 전용 스포너를 활성화
	void ActivateBossSpawners();
	
	void StartDifficultyTimerForReadyStage();
	
	// 죽은 적이 보스 랭크인지 판정
	bool IsBossEnemy(const AActor* DeadEnemy) const;
	
	UPROPERTY()
	TObjectPtr<UNSStageManager> NSStageManager;
	
	UPROPERTY()
	TObjectPtr<UNSMonsterPoolManager> NSMonsterPoolManager;

protected:
	/**
	 * 플레이어별로 생성할 Owner-only Proxy 클래스입니다.
	 * 에디터에서 BP Proxy 클래스를 지정합니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<ANSProjectileReplicationProxy> ProjectileReplicationProxyClass;

	/**
	 * 플레이어별로 생성할 재화 Owner-only Proxy 클래스입니다.
	 * PickupClass/VisualData 지정을 위해 BP 서브클래스를 지정합니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Currency")
	TSubclassOf<ANSCurrencyReplicationProxy> CurrencyReplicationProxyClass;

	
	// PickupClass/HealPotionTable 지정을 위해 BP 서브클래스 
	UPROPERTY(EditDefaultsOnly, Category = "Heal")
	TSubclassOf<ANSHealReplicationProxy> HealReplicationProxyClass;
private:
	// 프록시 등록 함수
	void EnsureProjectileProxy(APlayerController* PlayerController);
	
	// 프록시 제거 함수
	void DestroyProjectileProxy(APlayerController* PlayerController);

	UNSProjectileManagerComponent* GetProjectileManager() const;

	UPROPERTY(Transient)
	TMap<TObjectPtr<APlayerController>, TObjectPtr<ANSProjectileReplicationProxy>> ProjectileProxies;

	// 재화 프록시 등록 함수
	void EnsureCurrencyProxy(APlayerController* PlayerController);
	// 재화 프록시 제거 함수
	void DestroyCurrencyProxy(APlayerController* PlayerController);
	// 런 종료시 재화쪽 저장 및 클리어
	void CommitAndClearAllWallets(float Multiplier);
	
	// 회복 프록시 등록 함수
	void EnsureHealProxy(APlayerController* PlayerController);
	// 회복 프록시 제거 함수
	void DestroyHealProxy(APlayerController* PlayerController);
	
	

	// 거점 귀환 시 모든 플레이어의 인런 증강 Clear
	void ClearAllAugments();

	// 거점 귀환 시 모든 플레이어의 인런 파츠 Clear (세이브의 단일 파츠 A만 복원되도록)
	void ClearAllParts();

	// 거점 귀환 시 아직 소비되지 않은 증강 선택 큐 초기화.
	void ResetAugmentSelectionQueues();
	
	// 몬스터 사망시 보상 처리
	void HandleEnemyReward(AActor* DeadEnemy);
	bool TryGetRewardTriggerTagFromEnemy(const AActor* DeadEnemy, FGameplayTag& OutTriggerTag) const;
	void HandleEnemyExperience(AActor* DeadEnemy);
	
	// PlayerArray 내 위치로 이 플레이어의 고정 슬롯 인덱스 결정해주는 헬퍼 함수
	int32 GetPlayerSlotIndex(AController* Player) const;
	// 전원을 PlayerBossStart%d로 이동
	void TeleportAllPlayersToBossRoom();  
	// 목표방 잔존 적 풀 반환
	void ReturnStrayEnemiesToPool();          
	
	UPROPERTY(EditDefaultsOnly, Category = "Currency")
	float ClearMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Currency")
	float FailMultiplier = 0.5f;
	
	UPROPERTY(Transient)
	TMap<TObjectPtr<APlayerController>, TObjectPtr<ANSCurrencyReplicationProxy>> CurrencyProxies;
	
	UPROPERTY(Transient)
	TMap<TObjectPtr<APlayerController>, TObjectPtr<ANSHealReplicationProxy>> HealProxies;
	
	// 몬스터 보상 처리 관련 변수
	UPROPERTY(EditDefaultsOnly, Category = "NS|Reward")
	TSubclassOf<ANSDroppedPart> RewardDroppedPartClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "NS|Reward", meta = (ClampMin = "0.0"))
	float RewardCurrencyDropDuration = 15.0f;
	
	// 보상 드랍 판정이 매번 같은 결과로 고정되지 않도록 GameMode에서 유지하는 서버 전용 랜덤 스트림
	UPROPERTY(Transient)
	FRandomStream RewardRandomStream;
};
