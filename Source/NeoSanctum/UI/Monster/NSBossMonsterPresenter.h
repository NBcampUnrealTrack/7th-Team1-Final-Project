// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NSMonsterUITypes.h"
#include "NSBossMonsterPresenter.generated.h"

struct FNSMonsterUIData;
class ANSRunGameState;
class ULocalPlayer;
class UNSBossMonsterStatusWidget;
class UNSMonsterStatusViewModel;

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 보스 몬스터 상태 UI의 런타임 수명을 관리하는 Presenter입니다.
 * 이후 다중 보스 위젯과 ViewModel을 보스 수에 맞게 관리합니다.
 * Presenter: 이벤트와 ViewModel/Widget을 연결해서 화면 표현을 관리하는 객체
 */
USTRUCT()
struct FNSBossMonsterUIEntry
{
	GENERATED_BODY()

	// 상태 UI가 연결된 보스 Actor를 약하게 보관하는 변수
	UPROPERTY()
	TWeakObjectPtr<AActor> BossActor;

	// 화면에 표시 중인 보스 상태 위젯 변수
	UPROPERTY()
	TObjectPtr<UNSBossMonsterStatusWidget> Widget;

	// Attribute를 관찰하는 ViewModel 변수
	UPROPERTY()
	TObjectPtr<UNSMonsterStatusViewModel> ViewModel;
};

UCLASS()
class NEOSANCTUM_API UNSBossMonsterPresenter : public UObject
{
	GENERATED_BODY()

public:
	// 로컬 플레이어 기준 보스 Presenter를 초기화하는 함수
	void Initialize(ULocalPlayer* InLocalPlayer);

	// HUD Host를 등록하거나 해제하는 함수
	void SetHUDHost(UObject* InHUDHostObject);

	// 보스 Presenter가 보유한 런타임 상태를 해제하는 함수
	void Shutdown();

private:
	// StagePhase 변경 시 보스 UI 상태를 갱신하는 함수
	UFUNCTION()
	void HandleStagePhaseChanged();

	// RunGameState에 StagePhase 변경 이벤트를 바인딩하는 함수
	void BindRunGameState();

	// RunGameState StagePhase 변경 이벤트 바인딩을 해제하는 함수
	void UnbindRunGameState();

	// BossFight 상태에 맞춰 보스 탐색 Timer를 시작하는 함수
	void StartBossDiscovery();

	// 보스 탐색 Timer를 정지하는 함수
	void StopBossDiscovery();

	// 현재 World에서 표시 대상 보스를 갱신하는 함수
	void RefreshBosses();

	// 보스 Actor를 HUD에 추가하는 함수
	void AddBoss(AActor* BossActor);

	// 보스 Entry를 제거하는 함수
	void RemoveBossAt(int32 EntryIndex);

	// 모든 보스 Entry를 제거하는 함수
	void ClearBosses();

	// 이미 등록된 보스 Entry 인덱스를 찾는 함수
	int32 FindBossEntryIndex(AActor* BossActor) const;

	// Actor가 보스 UI 표시 대상인지 확인하는 함수
	bool IsValidBossTarget(AActor* BossActor) const;

	// 보스 상태 위젯 클래스를 UIManager 캐시에서 찾는 함수
	bool ResolveWidgetClass();

	// Presenter가 사용할 PlayerController를 반환하는 함수
	APlayerController* GetOwningPlayerController() const;

	// Presenter가 사용할 World를 반환하는 함수
	UWorld* GetPresenterWorld() const;

	// HUD Host에서 보스 HorizontalBox Layer를 반환하는 함수
	class UHorizontalBox* GetBossMonsterBox() const;
	
	// 대상 보스의 UI 프로필 Row를 찾는 함수
	const FNSMonsterUIData* FindMonsterUIData(AActor* BossActor) const;

	// 보스 몬스터용 UI 표시 정책을 만드는 함수
	FNSMonsterUIDisplayPolicy BuildBossDisplayPolicy(const FNSMonsterUIData* ProfileRow) const;

private:
	// Presenter가 속한 로컬 플레이어를 약하게 보관하는 변수
	TWeakObjectPtr<ULocalPlayer> OwningLocalPlayer;

	// HUD Host UObject를 약하게 보관하는 변수
	TWeakObjectPtr<UObject> HUDHostObject;

	// StagePhase Delegate를 바인딩한 RunGameState를 약하게 보관하는 변수
	TWeakObjectPtr<ANSRunGameState> BoundRunGameState;

	// 활성 보스 UI Entry 목록 변수
	UPROPERTY()
	TArray<FNSBossMonsterUIEntry> ActiveBossEntries;

	// 보스 상태 위젯 클래스 변수
	UPROPERTY()
	TSubclassOf<UNSBossMonsterStatusWidget> BossMonsterWidgetClass;

	// 보스 탐색 Timer Handle 변수
	FTimerHandle BossDiscoveryTimerHandle;

	// 보스 탐색 갱신 간격 변수
	UPROPERTY(EditDefaultsOnly, Category = "MonsterUI|Boss")
	float BossDiscoveryIntervalSeconds = 0.25f;
};
