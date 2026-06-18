// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NeoSanctum/Core/GameFlow/NSRunFlowType.h"
#include "NSRunGameState.generated.h"

class ANSPlayerState;
class UNSProjectileManagerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNSOnRunEndPhaseChanged);

/**
 *
 */
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
	
	// UI가 바인딩해서 열고/닫고 전환
	UPROPERTY(BlueprintAssignable, Category="RunEnd")
	FNSOnRunEndPhaseChanged OnRunEndPhaseChanged;

	UPROPERTY(ReplicatedUsing=OnRep_RunEndPhase, BlueprintReadOnly, Category="RunEnd")
	ENSRunEndPhase RunEndPhase = ENSRunEndPhase::None;  

	// true면 선택지 1개(귀환만), false면 2개
	UPROPERTY(Replicated, BlueprintReadOnly, Category="RunEnd")
	bool bIsClear = false;                               

	// 현재 페이즈 종료 서버시각
	UPROPERTY(Replicated, BlueprintReadOnly, Category="RunEnd")
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
	
	// 호스트 UI 연동용 헬퍼 함수
	void SetRunEndPhase(ENSRunEndPhase NewPhase);
	
private:
	// 런 전체에서 서버 투사체를 관리하는 Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNSProjectileManagerComponent> ProjectileManagerComponent;
};
