// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NSGuideSubsystemBase.generated.h"

class UNSPermanentSaveGame;
struct FNSGuideChecklistEntry;

/**
 * 가이드(조작법/목표 안내) 서브시스템 공용 베이스
 * 세이브/DataTable 준비 대기, HUD 체크리스트 브리지, 단계 전환 골격을 담는다.
 * 월드별 단계 정의(아웃런/인런)는 파생 클래스가 가상 함수로 채운다.
 * 완료 경로는 두 가지: empty-driven(마지막 줄 완료로 컨테이너가 비면 재평가) +
 * 명시적 완료(CompleteItemAndHide — 의미 있는 단일 이벤트의 애니메이션이 끝나면
 * 다른 미완료 항목과 무관하게 즉시 종료).
 * Abstract이므로 이 클래스 자체는 서브시스템으로 생성되지 않는다.
 */
UCLASS(Abstract)
class NEOSANCTUM_API UNSGuideSubsystemBase : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 월드 진입 시 PlayerController가 호출 → 세이브/DT 준비되면 현재 단계 표시
	void StartGuide();

	// 체크리스트가 비워졌을 때(위젯 콜백) → 다음 단계 표시 또는 전체 숨김
	void NotifyChecklistEmptied();

	// HUD 재생성 시 현재 단계 상태를 새 위젯에 강제 재적용
	void RefreshGuideForHUD();

	// 이 월드에서 안내가 시작됐는지 (위젯이 활성 가이드를 고를 때 사용)
	bool IsGuideStarted() const { return bGuideStarted; }

protected:
	// 현재 진행해야 할 단계의 RowName. 전부 완료면 NAME_None (파생 구현)
	virtual FName GetActiveStageRowName() const { return NAME_None; }

	// 지정 단계(RowName)에 표시할 체크리스트 항목을 채움 (파생 구현)
	virtual void BuildStageEntries(FName RowName, TArray<FNSGuideChecklistEntry>& OutEntries) const {}

	// 마커 등 월드 고유 시각요소 갱신 (아웃런만 오버라이드, 인런은 no-op)
	virtual void RefreshVisuals() {}

	// 세이브/DT 준비 확인 → 시각요소 + 체크리스트 갱신
	void RefreshGuide();

	// 현재 활성 단계에 맞는 체크리스트 표시/숨김 (같은 단계면 재스폰 안 함)
	void UpdateGuideChecklist();

	// HUD의 해당 줄 완료 애니메이션 재생 (HUD 없으면 조용히 무시)
	void CompleteItem(FName ItemId);

	// 명시적 완료: 해당 항목의 완료 애니메이션이 끝나면 다른 미완료 항목과 무관하게
	// 컨테이너 전체를 숨김 (Phase 2 카드선택용 — 리롤을 건너뛰어도 확실히 종료)
	void CompleteItemAndHide(FName ItemId);

	// 캐시된 영구 세이브 (로딩 전이면 nullptr)
	UNSPermanentSaveGame* GetPermanentData() const;

	// 안내 진행 상태를 영구 저장
	void SaveGuideState();

	// StartGuide 호출 여부 → 다른 월드/완료 후의 Notify를 무시하는 게이트
	bool bGuideStarted = false;

	// 현재 체크리스트로 표시 중인 단계 RowName (같은 단계 재스폰 방지)
	FName CurrentChecklistStage = NAME_None;

private:
	// 세이브 캐시가 아직 없을 때 로드 완료를 기다렸다가 갱신
	void HandlePermanentDataLoaded(UNSPermanentSaveGame* Data);

	// 공용 DataTable이 아직 로드 전일 때 로드 완료를 기다렸다가 갱신
	UFUNCTION()
	void HandleCommonDataReady();

	// 세이브 로드 대기 델리게이트 핸들
	FDelegateHandle DataLoadedHandle;
};
