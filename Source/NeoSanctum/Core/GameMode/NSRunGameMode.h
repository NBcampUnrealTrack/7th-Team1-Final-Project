// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "NSRunGameMode.generated.h"

enum class ENSRunChoice : uint8;
class UNSStageManager;
class UNSMonsterPoolManager;
class ANSEnemyCharacterBase;
class UNSEnemyData;
class UNSProjectileManagerComponent;
class ANSProjectileReplicationProxy;
class ANSCurrencyReplicationProxy;
class APlayerController;

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

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void HandleSeamlessTravelPlayer(AController*& Controller) override;

	virtual void SubmitRunChoice_Implementation(APlayerController* Voter, ENSRunChoice Choice) override;
	virtual void CancelRunChoice_Implementation(APlayerController* PlayerController) override;
	
	// 룸 생성 완료시 호출
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void RespawnAllPlayers();
	
	// 맵 로딩이 완료되고 블루프린트에서 호출될 함수
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void SetEnemyCount(int32 Count);

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

private:
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
	
	UPROPERTY(EditDefaultsOnly, Category = "Currency")
	float ClearMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Currency")
	float FailMultiplier = 0.5f;
	
	UPROPERTY(Transient)
	TMap<TObjectPtr<APlayerController>, TObjectPtr<ANSCurrencyReplicationProxy>> CurrencyProxies;
};
