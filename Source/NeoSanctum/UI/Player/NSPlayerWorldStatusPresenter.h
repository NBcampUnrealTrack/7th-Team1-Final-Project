// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NSPlayerWorldStatusPresenter.generated.h"

class ANSPlayerCharacterBase;
class ANSPlayerState;
class UCanvasPanel;
class ULocalPlayer;
class UNSPlayerWorldStatusViewModel;
class UNSPlayerWorldStatusWidget;

USTRUCT()
struct FNSPlayerWorldStatusEntry
{
	GENERATED_BODY()

	// 상태 UI가 연결된 플레이어 식별자를 보관하는 변수
	UPROPERTY()
	int32 PlayerId = INDEX_NONE;

	// 상태 UI가 연결된 PlayerState를 약하게 보관하는 변수
	UPROPERTY()
	TWeakObjectPtr<ANSPlayerState> PlayerState;

	// 상태 UI가 따라갈 플레이어 캐릭터를 약하게 보관하는 변수
	UPROPERTY()
	TWeakObjectPtr<ANSPlayerCharacterBase> TargetCharacter;

	// 화면에 표시 중인 플레이어 월드 상태 위젯 변수
	UPROPERTY()
	TObjectPtr<UNSPlayerWorldStatusWidget> Widget;

	// Attribute를 관찰하는 ViewModel 변수
	UPROPERTY()
	TObjectPtr<UNSPlayerWorldStatusViewModel> ViewModel;

	// 마지막 Occlusion Trace 검사 시간 변수
	UPROPERTY()
	float LastOcclusionCheckTimeSeconds = -1000.0f;

	// 마지막 Occlusion Trace 결과를 보관하는 변수
	UPROPERTY()
	bool bOccluded = false;
};

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 플레이어 월드 상태 UI의 런타임 수명을 관리하는 Presenter입니다.
 * 팀원 위젯과 ViewModel을 접속 중인 플레이어 수에 맞게 관리합니다.
 * Presenter: 이벤트와 ViewModel/Widget을 연결해서 화면 표현을 관리하는 객체
 */
UCLASS()
class NEOSANCTUM_API UNSPlayerWorldStatusPresenter : public UObject
{
	GENERATED_BODY()

public:
	// 로컬 플레이어 기준 플레이어 월드 상태 Presenter를 초기화하는 함수
	void Initialize(ULocalPlayer* InLocalPlayer);

	// HUD Host를 등록하거나 해제하는 함수
	void SetHUDHost(UObject* InHUDHostObject);

	// Presenter가 보유한 런타임 상태를 해제하는 함수
	void Shutdown();

private:
	// 현재 접속 중인 팀원 목록과 활성 Entry 목록을 동기화하는 함수
	void RefreshTrackedPlayers();

	// 대상 PlayerState의 월드 상태 UI를 생성하는 함수
	void AddTrackedPlayer(ANSPlayerState* PlayerState);

	// 특정 PlayerId의 활성 Entry를 제거하는 함수
	void RemoveTrackedPlayer(int32 PlayerId);

	// 활성 플레이어 위젯들의 위치와 표시 상태를 갱신하는 함수
	void UpdateActiveEntries();

	// 플레이어 월드 상태 위젯을 풀에서 얻거나 새로 생성하는 함수
	UNSPlayerWorldStatusWidget* AcquireWidget();

	// 플레이어 월드 상태 위젯을 풀로 반환하는 함수
	void ReleaseWidget(UNSPlayerWorldStatusWidget* Widget);

	// 활성 Entry를 해제하는 함수
	void ReleaseEntryAt(int32 EntryIndex);

	// 모든 활성 Entry와 풀 위젯을 정리하는 함수
	void ReleaseAllEntries();

	// PlayerId에 해당하는 활성 Entry 인덱스를 찾는 함수
	int32 FindEntryIndex(int32 PlayerId) const;

	// 대상 PlayerState가 플레이어 월드 상태 UI에 표시 가능한지 확인하는 함수
	bool IsValidPlayerStateTarget(ANSPlayerState* PlayerState) const;

	// 대상 PlayerState에서 따라갈 캐릭터 Pawn을 찾는 함수
	ANSPlayerCharacterBase* ResolveTargetCharacter(ANSPlayerState* PlayerState) const;

	// 대상 캐릭터의 UI 기준 월드 위치를 계산하는 함수
	FVector ResolveAnchorLocation(const ANSPlayerCharacterBase* TargetCharacter) const;

	// 대상 캐릭터가 벽 뒤에 가려졌는지 검사하는 함수
	bool IsTargetOccluded(const ANSPlayerCharacterBase* TargetCharacter, const FVector& AnchorLocation) const;

	// 위치 갱신 Timer를 시작하는 함수
	void StartPositionUpdateTimer();

	// 위치 갱신 Timer를 정지하는 함수
	void StopPositionUpdateTimer();

	// 팀원 목록 갱신 Timer를 시작하는 함수
	void StartRosterRefreshTimer();

	// 팀원 목록 갱신 Timer를 정지하는 함수
	void StopRosterRefreshTimer();

	// Presenter가 사용할 PlayerController를 반환하는 함수
	APlayerController* GetOwningPlayerController() const;

	// Presenter가 사용할 World를 반환하는 함수
	UWorld* GetPresenterWorld() const;

	// HUD Host에서 플레이어 월드 상태 Canvas Layer를 반환하는 함수
	UCanvasPanel* GetPlayerWorldStatusCanvas() const;

	// 플레이어 월드 상태 위젯 클래스를 UIManager 캐시에서 찾는 함수
	bool ResolveWidgetClass();

private:
	// Presenter가 속한 로컬 플레이어를 약하게 보관하는 변수
	TWeakObjectPtr<ULocalPlayer> OwningLocalPlayer;

	// HUD Host UObject를 약하게 보관하는 변수
	TWeakObjectPtr<UObject> HUDHostObject;

	// 활성 플레이어 월드 상태 UI Entry 목록 변수
	UPROPERTY()
	TArray<FNSPlayerWorldStatusEntry> ActiveEntries;

	// 재사용 대기 중인 플레이어 월드 상태 위젯 풀 변수
	UPROPERTY()
	TArray<TObjectPtr<UNSPlayerWorldStatusWidget>> PooledWidgets;

	// 플레이어 월드 상태 위젯 클래스 변수
	UPROPERTY()
	TSubclassOf<UNSPlayerWorldStatusWidget> PlayerWorldStatusWidgetClass;

	// 위치 갱신 Timer Handle 변수
	FTimerHandle PositionUpdateTimerHandle;

	// 팀원 목록 갱신 Timer Handle 변수
	FTimerHandle RosterRefreshTimerHandle;

	// 플레이어 월드 상태 UI 위치 갱신 간격 변수
	UPROPERTY(EditDefaultsOnly, Category = "PlayerWorldStatus")
	float PositionUpdateIntervalSeconds = 1.0f / 30.0f;

	// 팀원 목록 갱신 간격 변수
	UPROPERTY(EditDefaultsOnly, Category = "PlayerWorldStatus")
	float RosterRefreshIntervalSeconds = 0.5f;

	// 플레이어 월드 상태 UI 최대 표시 거리 변수
	UPROPERTY(EditDefaultsOnly, Category = "PlayerWorldStatus")
	float MaxDisplayDistance = 5000.0f;

	// 플레이어 기준 위치에서 UI를 위로 올릴 높이 변수
	UPROPERTY(EditDefaultsOnly, Category = "PlayerWorldStatus")
	float AnchorVerticalOffset = 35.0f;

	// 플레이어 머리 기준 Socket 이름 변수
	UPROPERTY(EditDefaultsOnly, Category = "PlayerWorldStatus")
	FName PlayerStatusSocketName = TEXT("head");

	// Occlusion Trace 사용 여부 변수
	UPROPERTY(EditDefaultsOnly, Category = "PlayerWorldStatus")
	bool bUseOcclusionTrace = false;

	// Occlusion Trace 검사 간격 변수
	UPROPERTY(EditDefaultsOnly, Category = "PlayerWorldStatus")
	float OcclusionTraceIntervalSeconds = 0.15f;
};
