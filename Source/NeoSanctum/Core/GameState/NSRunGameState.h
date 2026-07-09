// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NeoSanctum/Core/GameFlow/NSRunFlowType.h"
#include "NeoSanctum/Core/GameFlow/NSStageObjectiveType.h"
#include "NSRunGameState.generated.h"

class UNSLevelConfig;
class UNSRunConfig;
class ANSPlayerState;
class UNSProjectileManagerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNSOnRunEndPhaseChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNSOnRunEndVoteChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNSOnRunDataConfigChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNSOnStagePhaseChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNSOnStageObjectiveChanged);
// 보스 게이트 상태 변화 알림
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNSOnBossGateChanged);


/**
 *
 */

//런결과창에 표시할 집계 데이터
USTRUCT(BlueprintType)
struct FNSRunResultData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "RunEnd|Result")
	int32 EarnedGoods = 0;
	UPROPERTY(BlueprintReadOnly, Category = "RunEnd|Result")
	int32 CommonGoods = 0;
	UPROPERTY(BlueprintReadOnly, Category = "RunEnd|Result")
	float RunTimeSeconds =0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "RunEnd|Result")
	int32 KillCount = 0;
};

// 복제용 압축 스냅샷
USTRUCT(BlueprintType)
struct FNSStageObjectiveState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "Stage")
	ENSStageObjectiveType Type = ENSStageObjectiveType::KillCount;
	UPROPERTY(BlueprintReadOnly, Category = "Stage")
	int32 Current = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stage")
	int32 Target  = 0;
	
	//UI에 표시할 현재 목표 설명
	UPROPERTY(BlueprintReadOnly, Category = "Stage")
	FText Description;
};

UCLASS()
class NEOSANCTUM_API ANSRunGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ANSRunGameState();
	
	// GameMode, GA 등에서 서버 투사체 Manager에 접근하기 위한 Getter
	UNSProjectileManagerComponent* GetProjectileManagerComponent() const
	{
		return ProjectileManagerComponent;
	}
	
	void GetAlivePlayerStates(TArray<ANSPlayerState*>& AlivePlayerStates, const ANSPlayerState* ExcludedPlayerState = nullptr) const;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// 인런 월드 진입 후 클라이언트가 로드해야 할 데이터 구성이 복제됐음을 알림.
	UPROPERTY(BlueprintAssignable, Category = "Run|Data")
	FNSOnRunDataConfigChanged OnRunDataConfigChanged;
	
	// 서버가 선택한 인런 공통 데이터. 클라이언트는 이 값을 복제된 뒤 로드 시작.
	UPROPERTY(ReplicatedUsing = OnRep_RunDataConfig, BlueprintReadOnly, Category = "Run|Data")
	TSoftObjectPtr<UNSRunConfig> CurrentRunConfig;
	
	// 서버가 선택한 현재 스테이지 데이터. 클라이언트는 이 값이 복제된 뒤 로드 시작.
	UPROPERTY(ReplicatedUsing = OnRep_RunDataConfig, BlueprintReadOnly, Category = "Run|Data")
	TSoftObjectPtr<UNSLevelConfig> CurrentLevelConfig;
	
	void SetRunDataConfig(TSoftObjectPtr<UNSRunConfig> InRunConfig, TSoftObjectPtr<UNSLevelConfig> InLevelConfig);
	bool HasRunDataConfig() const;
	
	UFUNCTION()
	void OnRep_RunDataConfig();
	
	// UI가 바인딩해서 열고/닫고 전환
	UPROPERTY(BlueprintAssignable, Category="RunEnd")
	FNSOnRunEndPhaseChanged OnRunEndPhaseChanged;

	UPROPERTY(ReplicatedUsing=OnRep_RunEndPhase, BlueprintReadOnly, Category="RunEnd")
	ENSRunEndPhase RunEndPhase = ENSRunEndPhase::None;  

	// true면 선택지 1개(귀환만), false면 2개
	UPROPERTY(Replicated, BlueprintReadOnly, Category="RunEnd")
	bool bIsClear = false;           

	// 현재 페이즈 종료 서버시각
	UPROPERTY(ReplicatedUsing=OnRep_PhaseEndServerTime, BlueprintReadOnly, Category="RunEnd")
	float PhaseEndServerTime = 0.0f;        

	// Result 단계에서 표시
	UPROPERTY(Replicated, BlueprintReadOnly, Category="RunEnd")
	ENSRunChoice WinningChoice = ENSRunChoice::ReturnToHub;

	UPROPERTY(ReplicatedUsing=OnRep_RunEndVotes, BlueprintReadOnly, Category="RunEnd")
	int32 NextVotes = 0;

	UPROPERTY(ReplicatedUsing=OnRep_RunEndVotes, BlueprintReadOnly, Category="RunEnd")
	int32 HubVotes = 0;

	UFUNCTION()
	void OnRep_RunEndVotes();

	// UI 카운트다운용 남은 초 계산 (Voting의 10초, Result의 3초)
	UFUNCTION(BlueprintPure, Category="RunEnd")
	float GetPhaseTimeRemaining() const;
	
	UFUNCTION()
	void OnRep_RunEndPhase();
	
	UFUNCTION()
	void OnRep_PhaseEndServerTime();
	
	// 호스트 UI 연동용 헬퍼 함수
	void SetRunEndPhase(ENSRunEndPhase NewPhase);
	
	UPROPERTY(BlueprintAssignable, Category = "RunEnd")
	FNSOnRunEndVoteChanged OnRunEndVoteChanged;
	
	void NotifyRunVoteChanged();
	
	UPROPERTY(ReplicatedUsing = OnRep_RunResultData, BlueprintReadOnly, Category = "RunEnd|Result")
	FNSRunResultData RunResultData;
	
	// UI(목표/보스게이트 표시) 바인딩용
	UPROPERTY(BlueprintAssignable, Category = "Stage")
	FNSOnStagePhaseChanged OnStagePhaseChanged;

	UPROPERTY(ReplicatedUsing = OnRep_StagePhase, BlueprintReadOnly, Category = "Stage")
	ENSStagePhase StagePhase = ENSStagePhase::Objective;
	
	UPROPERTY(BlueprintAssignable, Category = "Stage")
	FNSOnStageObjectiveChanged OnStageObjectiveChanged;

	UPROPERTY(ReplicatedUsing = OnRep_ObjectiveState, BlueprintReadOnly, Category = "Stage")
	FNSStageObjectiveState ObjectiveState;
	
	// dwell 남은 시간/전원 집결 상태 변화
	UPROPERTY(BlueprintAssignable, Category = "Stage")
	FNSOnBossGateChanged OnBossGateChanged;

	// dwell 카운트다운 종료 서버시각 (0이면 미진행)
	UPROPERTY(ReplicatedUsing = OnRep_BossGate, BlueprintReadOnly, Category = "Stage")
	float BossGateEndServerTime = 0.0f;

	// 볼륨 위에 살아있는 전원이 모였는지
	UPROPERTY(ReplicatedUsing = OnRep_BossGate, BlueprintReadOnly, Category = "Stage")
	bool bBossGateAllPresent = false;

	UFUNCTION()
	void OnRep_BossGate();

	// UI 카운트다운용 남은 초 (미진행이면 0)
	UFUNCTION(BlueprintPure, Category = "Stage")
	float GetBossGateTimeRemaining() const;

	// 서버 전용: 볼륨이 dwell 상태를 밀어넣을 때 호출
	void SetBossGateState(float InEndServerTime, bool bInAllPresent);

	UFUNCTION()
	void OnRep_ObjectiveState();
	
	void SetObjectiveState(const FNSStageObjectiveState& NewState);

	UFUNCTION()
	void OnRep_StagePhase();

	// 서버 전용: 페이즈 전환 (호스트 OnRep 수동 호출 포함)
	void SetStagePhase(ENSStagePhase NewPhase);
	
	UFUNCTION()
	void OnRep_RunResultData();

	void SetRunResultData(const FNSRunResultData& NewRunResultData);

	void AddRunResultKillCount(int32 Amount = 1);
	
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetDifficultyElapsedSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetDifficultyProgressPercent(float Interval) const;

	UFUNCTION(BlueprintPure, Category = "Difficulty")
	int32 GetDifficultyLevel(float Interval) const;

	UFUNCTION(BlueprintPure, Category = "Difficulty")
	bool ShouldShowDifficultyTimer() const;

	UFUNCTION(BlueprintPure, Category = "Difficulty")
	int32 GetDifficultyStageNumber() const { return DifficultyStageNumber; }

	void SetDifficultyTimerState(
		bool bInRunning,
		float InBaseElapsedSeconds,
		int32 InStageNumber);
	
	UPROPERTY(ReplicatedUsing = OnRep_DifficultyTimerState, BlueprintReadOnly, Category = "Difficulty", meta = (AllowPrivateAccess = "true"))
	bool bDifficultyTimerRunning = false;

	UPROPERTY(ReplicatedUsing = OnRep_DifficultyTimerState, BlueprintReadOnly, Category = "Difficulty", meta = (AllowPrivateAccess = "true"))
	float DifficultyBaseElapsedSeconds = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_DifficultyTimerState, BlueprintReadOnly, Category = "Difficulty", meta = (AllowPrivateAccess = "true"))
	float DifficultyStartServerTime = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_DifficultyTimerState, BlueprintReadOnly, Category = "Difficulty", meta = (AllowPrivateAccess = "true"))
	int32 DifficultyStageNumber = 0;

	UFUNCTION()
	void OnRep_DifficultyTimerState();
private:
	// 런 전체에서 서버 투사체를 관리하는 Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNSProjectileManagerComponent> ProjectileManagerComponent;
};
