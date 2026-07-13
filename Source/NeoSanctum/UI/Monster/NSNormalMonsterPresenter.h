// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
#include "NSNormalMonsterPresenter.generated.h"

class ULocalPlayer;
class UNSMonsterStatusViewModel;
class UNSNormalMonsterStatusWidget;

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 일반 몬스터 상태 UI의 런타임 수명을 관리하는 Presenter입니다.
 * 이후 피격 표시, 위젯 풀, ViewModel 소유, 위치 갱신을 담당합니다.
 * Presenter: 이벤트와 ViewModel/Widget을 연결해서 화면 표현을 관리하는 객체
 */
USTRUCT()
struct FNSNormalMonsterUIEntry
{
	GENERATED_BODY()

	// 상태 UI가 연결된 몬스터 Actor를 약하게 보관하는 변수
	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	// 화면에 표시 중인 일반 몬스터 상태 위젯 변수
	UPROPERTY()
	TObjectPtr<UNSNormalMonsterStatusWidget> Widget;

	// Attribute를 관찰하는 ViewModel 변수
	UPROPERTY()
	TObjectPtr<UNSMonsterStatusViewModel> ViewModel;

	// UI를 숨길 월드 시간 변수
	UPROPERTY()
	float ExpireTimeSeconds = 0.0f;

	// 마지막 Occlusion Trace 검사 시간 변수
	UPROPERTY()
	float LastOcclusionCheckTimeSeconds = -1000.0f;

	// 마지막 Occlusion Trace 결과를 보관하는 변수
	UPROPERTY()
	bool bOccluded = false;
};

UCLASS()
class NEOSANCTUM_API UNSNormalMonsterPresenter : public UObject
{
	GENERATED_BODY()

public:
	// 로컬 플레이어 기준 일반 몬스터 Presenter를 초기화하는 함수
	void Initialize(ULocalPlayer* InLocalPlayer);

	// HUD Host를 등록하거나 해제하는 함수
	void SetHUDHost(UObject* InHUDHostObject);

	// 일반 몬스터 Presenter가 보유한 런타임 상태를 해제하는 함수
	void Shutdown();

private:
	// 일반 몬스터 표시 메시지를 수신하는 함수
	void HandleRevealMessage(FGameplayTag ChannelTag, const FNSNormalMonsterRevealMessage& Message);

	// 대상 몬스터의 상태 UI를 표시하거나 표시 시간을 갱신하는 함수
	void RevealTarget(AActor* TargetActor);

	// 활성 일반 몬스터 위젯들의 위치와 표시 상태를 갱신하는 함수
	void UpdateActiveEntries();

	// 일반 몬스터 상태 위젯을 풀에서 얻거나 새로 생성하는 함수
	UNSNormalMonsterStatusWidget* AcquireWidget();

	// 일반 몬스터 상태 위젯을 풀로 반환하는 함수
	void ReleaseWidget(UNSNormalMonsterStatusWidget* Widget);

	// 활성 Entry를 해제하는 함수
	void ReleaseEntryAt(int32 EntryIndex);

	// 모든 활성 Entry와 풀 위젯을 정리하는 함수
	void ReleaseAllEntries();

	// TargetActor에 해당하는 활성 Entry 인덱스를 찾는 함수
	int32 FindEntryIndex(AActor* TargetActor) const;

	// 대상이 일반 몬스터 UI에 표시 가능한지 확인하는 함수
	bool IsValidNormalMonsterTarget(AActor* TargetActor) const;

	// 대상 몬스터의 UI 기준 월드 위치를 계산하는 함수
	FVector ResolveAnchorLocation(AActor* TargetActor) const;

	// 대상 몬스터가 벽 뒤에 가려졌는지 검사하는 함수
	bool IsTargetOccluded(AActor* TargetActor, const FVector& AnchorLocation) const;

	// 위치 갱신 Timer를 시작하는 함수
	void StartPositionUpdateTimer();

	// 위치 갱신 Timer를 정지하는 함수
	void StopPositionUpdateTimer();

	// Reveal 메시지 리스너를 등록하는 함수
	void RegisterRevealMessageListener();

	// Reveal 메시지 리스너를 해제하는 함수
	void UnregisterRevealMessageListener();

	// Presenter가 사용할 PlayerController를 반환하는 함수
	APlayerController* GetOwningPlayerController() const;

	// Presenter가 사용할 World를 반환하는 함수
	UWorld* GetPresenterWorld() const;

	// HUD Host에서 일반 몬스터 Canvas Layer를 반환하는 함수
	class UCanvasPanel* GetNormalMonsterCanvas() const;

	// 일반 몬스터 위젯 클래스를 UIManager 캐시에서 찾는 함수
	bool ResolveWidgetClass();

private:
	// Presenter가 속한 로컬 플레이어를 약하게 보관하는 변수
	TWeakObjectPtr<ULocalPlayer> OwningLocalPlayer;

	// HUD Host UObject를 약하게 보관하는 변수
	TWeakObjectPtr<UObject> HUDHostObject;

	// 활성 일반 몬스터 UI Entry 목록 변수
	UPROPERTY()
	TArray<FNSNormalMonsterUIEntry> ActiveEntries;

	// 재사용 대기 중인 일반 몬스터 위젯 풀 변수
	UPROPERTY()
	TArray<TObjectPtr<UNSNormalMonsterStatusWidget>> PooledWidgets;

	// 일반 몬스터 상태 위젯 클래스 변수
	UPROPERTY()
	TSubclassOf<UNSNormalMonsterStatusWidget> NormalMonsterWidgetClass;

	// 위치 갱신 Timer Handle 변수
	FTimerHandle PositionUpdateTimerHandle;

	// Reveal 메시지 리스너 Handle 변수
	FGameplayMessageListenerHandle RevealMessageListenerHandle;

	// 일반 몬스터 상태 UI 표시 유지 시간 변수
	UPROPERTY(EditDefaultsOnly, Category = "MonsterUI|Normal")
	float RevealDurationSeconds = 3.0f;

	// 일반 몬스터 상태 UI 위치 갱신 간격 변수
	UPROPERTY(EditDefaultsOnly, Category = "MonsterUI|Normal")
	float PositionUpdateIntervalSeconds = 1.0f / 30.0f;

	// 일반 몬스터 상태 UI 최대 표시 거리 변수
	UPROPERTY(EditDefaultsOnly, Category = "MonsterUI|Normal")
	float MaxDisplayDistance = 3500.0f;

	// 몬스터 기준 위치에서 UI를 위로 올릴 높이 변수
	UPROPERTY(EditDefaultsOnly, Category = "MonsterUI|Normal")
	float AnchorVerticalOffset = 35.0f;

	// Occlusion Trace 사용 여부 변수
	UPROPERTY(EditDefaultsOnly, Category = "MonsterUI|Normal")
	bool bUseOcclusionTrace = true;

	// Occlusion Trace 검사 간격 변수
	UPROPERTY(EditDefaultsOnly, Category = "MonsterUI|Normal")
	float OcclusionTraceIntervalSeconds = 0.15f;

	// 동시에 표시할 일반 몬스터 UI 최대 개수 변수
	UPROPERTY(EditDefaultsOnly, Category = "MonsterUI|Normal")
	int32 MaxVisibleWidgets = 15;
};
