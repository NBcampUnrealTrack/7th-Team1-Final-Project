// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSBossEntryVolume.generated.h"

class UBoxComponent;
class USceneComponent;
class ANSRunGameState;
class UNiagaraComponent;      
class UNiagaraSystem;   

// 보스룸에 배치되는 진입 트리거
// 목표 달성 후 활성화되며 플레이어가 일정 시간 연속 체류하면 GameMode에 전원 텔레포트를 요청한다.
UCLASS()
class NEOSANCTUM_API ANSBossEntryVolume : public AActor
{
	GENERATED_BODY()

public:
	ANSBossEntryVolume();

	// 목표 달성 시 GameMode가 호출: 오버랩 감지 ON + dwell 판정 시작
	void Activate();
	// 트리거 소진(텔레포트 완료 등): 오버랩 감지 OFF + 타이머 정리
	void Deactivate();
	
	// 메시와 오버랩 반경 크기 맞출 헬퍼
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;
	void ApplyBoundaryDimensions();

	// 플레이어 폰 진입: 후보 집합에 추가, 0→1이면 카운트다운 시작
	UFUNCTION()
	void OnBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& Sweep);

	// 플레이어 폰 이탈: 집합에서 제거, 전원 이탈 시 카운트다운 리셋
	UFUNCTION()
	void OnEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	// dwell 완료 시: 볼륨 끄고 GameMode에 게이트 도달 통지
	void OnDwellCompleted();
	
	// StagePhase 변화 구독 콜백
	UFUNCTION()
	void HandleStagePhaseChanged();
	
	// 무효, 사망한 폰을 볼륨 위 폰 집합에서 제거하고 인원 변화에 따른 타이머 상태 변경 함수
	void RevalidateOccupants();

private:
	// RunGameState 구독 시도용 함수, 클라에서 아직 미복제면 다음 틱 재시도.
	void BindToRunGameState();
	
	// 오버랩 액터가 플레이어(플레이어컨트롤러 소유 폰)인지 판별
	bool IsPlayerPawn(const AActor* OtherActor) const;
	// 볼륨 위 인원이 현재 생존 플레이어 전원인지 판정
	bool AreAllPlayersPresent() const;
	// 볼륨 위에 머무르는 시간 타이머 시작(이미 돌고 있으면 유지)
	void StartDwellTimer();
	// 현재 dwell 상태(종료시각/전원여부)를 GameState에 넣을 함수
	void PushBossGateStateToGameState(float DurationFromNow);
	// dwell 비활성 상태를 GameState에 통지
	void ClearBossGateState();
	
	// GameState 상태에 맞춰 메시 표시/색 갱신
	void UpdateVisual();

	// bBossGateAllPresent 변화 구독 콜백
	UFUNCTION()
	void HandleBossGateChanged();

	// 진입 감지용 트리거 박스 (크기는 레벨에서 조정)
	UPROPERTY(VisibleAnywhere, Category = "BossEntry")
	TObjectPtr<UBoxComponent> TriggerBox;
	
	// 활성화 시 표시할 연출 루트 (에디터에서 메시/이펙트/데칼을 이 아래에 부착)
	UPROPERTY(VisibleAnywhere, Category = "BossEntry")
	TObjectPtr<USceneComponent> VisualRoot;
	
	// 범위 표시용 나이아가라 이펙트
	UPROPERTY(VisibleAnywhere, Category = "BossEntry|Visual")
	TObjectPtr<UNiagaraComponent> BoundaryNiagara;

	// 대기 상태 색(파랑)
	UPROPERTY(EditAnywhere, Category = "BossEntry|Visual")
	FLinearColor WaitingColor = FLinearColor(0.f, 0.4f, 1.f);

	// 전원 집결 색(노랑)
	UPROPERTY(EditAnywhere, Category = "BossEntry|Visual")
	FLinearColor AllPresentColor = FLinearColor(1.f, 0.85f, 0.f);

	// 색깔용 이름
	UPROPERTY(EditAnywhere, Category = "BossEntry|Visual")
	FName ColorParameterName = TEXT("Color");
	
	// 크기용 이름
	UPROPERTY(EditAnywhere, Category = "BossEntry|Visual")
	FName DimensionsParameterName = TEXT("Dimensions");

	// 전원 텔레포트까지 필요한 연속 체류 시간(초)
	UPROPERTY(EditAnywhere, Category = "BossEntry", meta = (ClampMin = "1"))
	float DwellDuration = 20.0f;
	
	// 전원 집결 시 텔레포트까지의 짧은 유예 시간(초)
	UPROPERTY(EditAnywhere, Category = "BossEntry", meta = (ClampMin = "0"))
	float AllPresentDelay = 2.0f;

	// 현재 볼륨 위에 있는 플레이어 폰 집합
	UPROPERTY(Transient)
	TSet<TObjectPtr<AActor>> OverlappingPlayers;
	
	// 구독한 RunGameState 캐시 (전원 판정/페이즈 조회용)
	UPROPERTY(Transient)
	TObjectPtr<ANSRunGameState> CachedRunGameState;
	
	// BP에서 지정할 나이아가라 시스템
	UPROPERTY(EditAnywhere, Category = "BossEntry|Visual")
	TObjectPtr<UNiagaraSystem> BoundaryEffect;

	// 볼륨 체류  카운트다운 타이머 핸들
	FTimerHandle DwellTimerHandle;
	
	// dwell 진행 중 주기적으로 집합을 청소하는 타이머
	FTimerHandle RevalidateTimerHandle;

	// 목표 달성으로 활성화됐는지 확인용 (중복 트리거 방지)
	bool bActivated = false;
	
	// 전원 집결 단축 타이머가 이미 걸렸는지 (중복 방지용)
	bool bAllPresentScheduled = false;
};
