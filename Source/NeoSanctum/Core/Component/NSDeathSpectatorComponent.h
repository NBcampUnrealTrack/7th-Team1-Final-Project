// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSDeathSpectatorComponent.generated.h"

class ANSDeathSpectatorPawn;
class ANSPlayerCharacterBase;
class ANSPlayerController;
class ANSPlayerState;

UCLASS(ClassGroup=(NeoSanctum), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSDeathSpectatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSDeathSpectatorComponent();

	// 사망 관전자 상태로 진입 요청 : 캐릭터의 사망 로직에서 요청하도록 되어있음
	void RequestEnterDeathSpectatorMode();
	// 이전 생존 플레이어를 관전 대상으로 요청
	void SpectatePreviousPlayer();
	// 다음 생존 플레이어를 관전 대상으로 요청
	void SpectateNextPlayer();
	// 서버가 확정한 관전 대상을 로컬 ViewTarget에 적용
	void ApplyConfirmedSpectatorTarget(ANSPlayerCharacterBase* TargetCharacter);
	// ClientRestart 이후 관전자 UI와 ViewTarget 복구
	bool HandleClientRestart(APawn* NewPawn);
	// 관전자 상태 캐시 초기화
	void ClearSpectatorState();
	// 서버 복제 기준 위치 확인
	bool GetSpectatorReplicationViewPoint(FVector& Location, FRotator& Rotation) const;

private:
	ANSPlayerController* GetOwnerPlayerController() const;

	// 실제로 사망 관전자 상태로 진입
	void EnterDeathSpectatorMode();
	// Spectator Pawn을 스폰하고 Possess하는 헬퍼
	void SpawnAndPossessDeathSpectatorPawn();
	// 진입 타이머 초기화 헬퍼
	void ClearDeathSpectatorModeTimer();

	// 관전자 스위칭
	void SwitchSpectatorTarget(int32 Direction);
	// 서버 권한에서 실제 관전 대상을 확정
	void ApplyServerSpectatorTargetChange(int32 Direction);
	// PlayerState에서 서버 기준 관전 대상 Pawn 확인
	ANSPlayerCharacterBase* ResolveServerSpectatorTargetPawn(const ANSPlayerState* TargetPlayerState) const;

	// 재화와 물약 표시 기준을 관전 대상에 맞춰 함께 바꿔줌.
	void SetDropViewPlayerState(ANSPlayerController* ViewerController, ANSPlayerState* ViewPlayerState);

	// Spectator Pawn을 스폰하고 Posses를 서버 권한에서 해야하기 때문에 서버 RPC로 처리
	UFUNCTION(Server, Reliable)
	void Server_EnterDeathSpectatorMode();

	// 클라이언트의 관전 대상 전환 입력을 서버로 전달
	UFUNCTION(Server, Reliable)
	void Server_RequestSpectatorTargetChange(int32 Direction);

private:
	// 사망 후 몇 초 뒤에 Death Spectator 모드로 진입할지 결정
	UPROPERTY(EditDefaultsOnly, Category = "Spectator", meta = (ClampMin = "0.0"))
	float DeathSpectatorModeDelay = 2.0f;

	// 사망 시 Spawn / Possess될 Spectator Pawn
	UPROPERTY(EditDefaultsOnly, Category = "Spectator")
	TSubclassOf<ANSDeathSpectatorPawn> DeathSpectatorPawnClass;

	// 관전 대상 전환 시 ViewTarget 보간 시간 : 값이 클수록 느리게 전환됨
	UPROPERTY(EditDefaultsOnly, Category = "Spectator", meta = (ClampMin = "0.0"))
	float DeathSpectatorViewBlendTime = 0.35f;

	// 관전 대상 PlayerState 캐싱
	UPROPERTY(Transient)
	TObjectPtr<ANSPlayerState> SpectatingPlayerState;

	FTimerHandle DeathSpectatorModeTimerHandle;
};
